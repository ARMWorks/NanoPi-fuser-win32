/*
* Copyright(c) 2015 Jeff Kent <jeff@jkent.net>
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met :
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* 2. The name of the author may not be used to endorse or promote products
* derived from this software withough specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC.AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include "stdafx.h"
#include "resource.h"

#include "NanoPi FuserView.h"

#define SECTOR_SIZE 512
#define BUFFER_SIZE (256*1024)

enum {
    FILE_DISK_IMAGE = 0,
    FILE_BOOTLOADER,
    FILE_ENVIRONMENT,
    FILE_KERNEL,
};

const DWORD FILE_MAX_SIZES[] = {
    ~0U,
    256 * 1024,
    16 * 1024,
    6 * 1024 * 1024,
};

const CString FILE_TYPES[] = {
    _T("disk image"),
    _T("bootloader"),
    _T("environment"),
    _T("kernel"),
};

const TCHAR *FILE_FILTERS[] = {
    _T("Image Files (*.img)\0*.img\0All Files (*.*)\0*.*\0"),
    _T("Bin Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0"),
    _T("Raw Files (*.raw)\0*.raw\0All Files (*.*)\0*.*\0"),
    _T("All Files (*.*)\0*.*\0"),
};

#define RESERVED_SIZE 1024
#define BL1_SIZE (8*1024)
#define ENV_SIZE (FILE_MAX_SIZES[FILE_ENVIRONMENT])
#define BL2_SIZE (FILE_MAX_SIZES[FILE_BOOTLOADER])
#define KERNEL_SIZE (FILE_MAX_SIZES[FILE_KERNEL])

#define SDHC_OFFSET (512*1024)
#define BL1_OFFSET (RESERVED_SIZE+BL1_SIZE)
#define ENV_OFFSET (BL1_OFFSET+ENV_SIZE)
#define BL2_OFFSET (ENV_OFFSET+BL2_SIZE)
#define KERNEL_OFFSET (BL2_OFFSET+KERNEL_SIZE)

CNanoPiFuserView::CNanoPiFuserView()
    :m_wndPhysicalDriveComboBox(this, 0), m_wndSdFuseCheck(this, 0), m_wndSdhcFuseCheck(this, 0),
    m_wndFuseProgress(this, 1), m_wndMessageLabel(this, 0),
    m_wndStartFuseBtn(this, 0), m_wndCancelBtn(this, 0)
{
    for (int i = 0; i < NUM_FILES; i++) {
        m_wndFileCheck[i] = new CContainedWindowT<CButton>(this, 0);
        m_wndFileEdit[i] = new CContainedWindowT<CEdit>(this, 0);
        m_wndFileBrowseBtn[i] = new CContainedWindowT<CButton>(this, 0);
    }
}

CNanoPiFuserView::~CNanoPiFuserView()
{
    for (int i = 0; i < NUM_FILES; i++) {
        delete m_wndFileCheck[i];
        delete m_wndFileEdit[i];
        delete m_wndFileBrowseBtn[i];
    }
}

BOOL CNanoPiFuserView::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

LRESULT CNanoPiFuserView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    for (int i = 0; i < NUM_FILES; i++) {
        m_wndFileCheck[i]->SubclassWindow(GetDlgItem(IDC_FILE1_CHECK + i));
        m_wndFileEdit[i]->SubclassWindow(GetDlgItem(IDC_FILE1_EDIT + i));
        m_wndFileBrowseBtn[i]->SubclassWindow(GetDlgItem(IDC_FILE1_BROWSE_BUTTON + i));

        m_wndFileCheck[i]->SetCheck(i < 2 ? BST_CHECKED : BST_UNCHECKED);
        m_wndFileEdit[i]->EnableWindow(i < 2 ? TRUE : FALSE);
        m_wndFileBrowseBtn[i]->EnableWindow(i < 2 ? TRUE : FALSE);
    }

    m_wndPhysicalDriveComboBox.SubclassWindow(GetDlgItem(IDC_PHYSICAL_DRIVE_COMBOBOX));

    m_wndSdFuseCheck.SubclassWindow(GetDlgItem(IDC_SD_FUSE_RADIO));
    m_wndSdhcFuseCheck.SubclassWindow(GetDlgItem(IDC_SDHC_FUSE_RADIO));

    m_wndFuseProgress.SubclassWindow(GetDlgItem(IDC_FUSE_PROGRESS));
    m_wndMessageLabel.SubclassWindow(GetDlgItem(IDC_MESSAGE_LABEL));

    m_wndStartFuseBtn.SubclassWindow(GetDlgItem(IDC_START_FUSE_BUTTON));
    m_wndCancelBtn.SubclassWindow(GetDlgItem(IDC_CANCEL_BUTTON));

    BOOL temp = TRUE;
    OnBnClickedRefreshButton(0, 0, NULL, temp);

    m_wndMessageLabel.SetWindowTextW(_T(""));

    m_fusing = false;
    m_timer = SetTimer(1, 125, NULL);

    return TRUE;
}

LRESULT CNanoPiFuserView::OnCloseDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (m_fusing) {
        int nRet = MessageBox(_T("A fuse operation is currently in progress.\n\nDo you want to cancel and exit?"), _T("NanoPi Fuser"), MB_ICONQUESTION | MB_YESNO);
        if (nRet == IDYES) {
            m_cancel = true;
            m_quit = true;
        }
        return 0;
    }

    GetParent().DestroyWindow();

    return 0;
}

LRESULT CNanoPiFuserView::OnRefreshDevices(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    PhysicalDrive::Enumerate(m_physicalDrives);
    m_wndPhysicalDriveComboBox.ResetContent();

    bool selection = false;
    for (size_t i = 0; i < m_physicalDrives.size(); i++) {
        m_wndPhysicalDriveComboBox.AddString(m_physicalDrives[i].GetDescription());
        if (!selection) {
            if (m_physicalDrives[i].GetPath() == m_lastSelectedPath && m_physicalDrives[i].GetSizeInBytes() == m_lastSelectedSize) {
                m_wndPhysicalDriveComboBox.SetCurSel(i);
                selection = true;
            }
        }
    }

    return 0;
}

LRESULT CNanoPiFuserView::OnBnClickedFileCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    for (int i = 0; i < NUM_FILES; i++) {
        if (m_wndFileCheck[i]->GetCheck() > 0) {
            m_wndFileEdit[i]->EnableWindow(TRUE);
            m_wndFileBrowseBtn[i]->EnableWindow(TRUE);
        }
        else {
            m_wndFileEdit[i]->EnableWindow(FALSE);
            m_wndFileBrowseBtn[i]->EnableWindow(FALSE);
        }
    }

    return 0;
}

LRESULT CNanoPiFuserView::OnBnClickedFileBrowseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
{
    int i;
    TCHAR path[MAX_PATH];

    for (i = 0; i < NUM_FILES; i++) {
        if (m_wndFileBrowseBtn[i]->m_hWnd == hWndCtl) {
            break;
        }
    }

    m_wndFileEdit[i]->GetWindowText(path, MAX_PATH);
    CFileDialog dialog(TRUE, NULL, path, OFN_FILEMUSTEXIST, FILE_FILTERS[i]);
    if (dialog.DoModal() == IDOK) {
        m_wndFileEdit[i]->SetWindowTextW(dialog.m_szFileName);
    }

    return 0;
}

LRESULT CNanoPiFuserView::OnBnClickedRefreshButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    PhysicalDrive::Enumerate(m_physicalDrives);

    m_wndPhysicalDriveComboBox.ResetContent();
    for (size_t i = 0; i < m_physicalDrives.size(); i++) {
        m_wndPhysicalDriveComboBox.AddString(m_physicalDrives[i].GetDescription());
    }

    return 0;
}

LRESULT CNanoPiFuserView::OnCbnSelchangePhysicalDriveCombobox(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    bHandled = FALSE;

    if (m_wndPhysicalDriveComboBox.GetCurSel() < 0) {
        return 0;
    }

    PhysicalDrive pd = m_physicalDrives[m_wndPhysicalDriveComboBox.GetCurSel()];

    m_lastSelectedPath = pd.GetPath();
    m_lastSelectedSize = pd.GetSizeInBytes();

    if (m_lastSelectedSize > 2147483647) {
        m_wndSdFuseCheck.SetCheck(BST_UNCHECKED);
        m_wndSdhcFuseCheck.SetCheck(BST_CHECKED);
    }
    else {
        m_wndSdFuseCheck.SetCheck(BST_CHECKED);
        m_wndSdhcFuseCheck.SetCheck(BST_UNCHECKED);
    }

    return 0;
}

LRESULT CNanoPiFuserView::OnCbnDropdownPhysicalDriveCombobox(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    KillTimer(m_timer);

    return 0;
}

LRESULT CNanoPiFuserView::OnCbnCloseupPhysicalDriveCombobox(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_timer = SetTimer(1, 125, NULL);

    return 0;
}

static LONGLONG GetFileSize64(HANDLE hFile)
{
    LARGE_INTEGER li;

    if (!GetFileSizeEx(hFile, &li)) {
        li.QuadPart = -1;
    }

    return li.QuadPart;
}

static void Poll(void)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static CString Capitalize(const CString s)
{
    CString copy(s);
    if (copy.GetLength() > 0) {
        copy.SetAt(0, _totupper(s[0]));
    }
    return copy;
}

LRESULT CNanoPiFuserView::OnBnClickedStartFuseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    TCHAR filename[MAX_PATH];
    CString message;
    int fileCount = 0;
    PhysicalDrive pd;
    HANDLE outputDrive = INVALID_HANDLE_VALUE;
    HANDLE inputFile = INVALID_HANDLE_VALUE;
    char buffer[BUFFER_SIZE];
    DWORD bufferLen, writtenLen;
    DWORD remainder;

    m_fusing = true;
    m_quit = false;
    m_cancel = false;

    /* Lock down the GUI */
    for (size_t i = 0; i < NUM_FILES; i++) {
        m_wndFileCheck[i]->EnableWindow(FALSE);
        m_wndFileEdit[i]->EnableWindow(FALSE);
        m_wndFileBrowseBtn[i]->EnableWindow(FALSE);
    }
    m_wndPhysicalDriveComboBox.EnableWindow(FALSE);
    m_wndSdFuseCheck.EnableWindow(FALSE);
    m_wndSdhcFuseCheck.EnableWindow(FALSE);
    m_wndStartFuseBtn.ShowWindow(SW_HIDE);
    m_wndCancelBtn.ShowWindow(SW_SHOW);

    KillTimer(m_timer);

    Poll();

    /* Check if files exist */
    for (size_t i = 0; i < NUM_FILES; i++) {
        if (m_wndFileCheck[i]->GetCheck() != BST_CHECKED) {
            continue;
        }

        fileCount++;

        m_wndFileEdit[i]->GetWindowText(filename, MAX_PATH);
        if (!PathFileExists(filename)) {
            message.Format(_T("%s file does not exist!"), Capitalize(FILE_TYPES[i]));
            MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
            goto Exit;
        }
    }

    if (fileCount == 0) {
        MessageBox(_T("No files selected to fuse!"), _T("Error"), MB_ICONERROR | MB_OK);
        goto Exit;
    }

    int upperRange = 0;
    for (size_t i = 0; i < NUM_FILES; i++) {
        if (m_wndFileCheck[i]->GetCheck() != BST_CHECKED) {
            continue;
        }

        m_wndFileEdit[i]->GetWindowText(filename, MAX_PATH);
        inputFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (inputFile == INVALID_HANDLE_VALUE) {
            message.Format(_T("%s could not be opened."), Capitalize(FILE_TYPES[i]));
            MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
            goto Exit;
        }

        LONGLONG fileSize = GetFileSize64(inputFile);
        if (i == FILE_DISK_IMAGE && fileSize < SECTOR_SIZE) {
            message.Format(_T("%s must be at least %d bytes"), Capitalize(FILE_TYPES[i]), SECTOR_SIZE);
        }

        if (FILE_MAX_SIZES[i] != ~0U && fileSize > FILE_MAX_SIZES[i]) {
            message.Format(_T("%s is too big.  Maximum of %u bytes."), Capitalize(FILE_TYPES[i]), FILE_MAX_SIZES[i]);
            MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
            goto Exit;
        }

        upperRange += (DWORD)((GetFileSize64(inputFile) + SECTOR_SIZE - 1) / SECTOR_SIZE);
        CloseHandle(inputFile);
        inputFile = INVALID_HANDLE_VALUE;
    }

    if (m_wndFileCheck[FILE_BOOTLOADER]->GetCheck() == BST_CHECKED) {
        upperRange += BL1_SIZE / SECTOR_SIZE;
    }

    m_wndFuseProgress.SetRange32(0, upperRange);
    m_wndFuseProgress.SetPos(0);

    {
        int i = m_wndPhysicalDriveComboBox.GetCurSel();
        if (i == CB_ERR) {
            MessageBox(_T("No physical drive selected!"), _T("Error"), MB_ICONERROR | MB_OK);
            goto Exit;
        }

        pd = m_physicalDrives[i];

        outputDrive = CreateFile(pd.GetPath(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (outputDrive == INVALID_HANDLE_VALUE) {
            MessageBox(_T("Selected physical drive inaccessable!"), _T("Error"), MB_ICONERROR | MB_OK);
            goto Exit;
        }

        if (!pd.Unmount()) {
            MessageBox(_T("Could not unmount volumes!"), _T("Error"), MB_ICONERROR | MB_OK);
            goto Exit;
        }
    }

    for (size_t i = 0; i < NUM_FILES; i++) {
        if (m_wndFileCheck[i]->GetCheck() != BST_CHECKED) {
            continue;
        }

        m_wndFileEdit[i]->GetWindowText(filename, MAX_PATH);
        inputFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (inputFile == INVALID_HANDLE_VALUE) {
            message.Format(_T("%s could not be opened.  Fusing incomplete!"), Capitalize(FILE_TYPES[i]));
            MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
            goto Exit;
        }

        LARGE_INTEGER offset;

        message.Format(_T("Writing %s..."), FILE_TYPES[i]);
        m_wndMessageLabel.SetWindowText(message);

        switch (i) {
        case FILE_DISK_IMAGE:
            /* Zero the partition info area and re-open the drive */
            memset(buffer, 0, SECTOR_SIZE);
			if (!WriteFile(outputDrive, buffer, SECTOR_SIZE, &writtenLen, NULL) || writtenLen < SECTOR_SIZE) {
				message.Format(_T("Error while writing %s.  Fusing incomplete!"), FILE_TYPES[i]);
				MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
				goto Exit;
			}
            CloseHandle(outputDrive);
            outputDrive = CreateFile(pd.GetPath(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (outputDrive == INVALID_HANDLE_VALUE) {
                MessageBox(_T("Could not reopen drive!"), _T("Error"), MB_ICONERROR | MB_OK);
                goto Exit;
            }

            /* Skip the partition info area */
            offset.QuadPart = SECTOR_SIZE;
            SetFilePointerEx(outputDrive, offset, NULL, FILE_BEGIN);
            SetFilePointerEx(inputFile, offset, NULL, FILE_BEGIN);

            while (true) {
                ReadFile(inputFile, buffer, sizeof(buffer), &bufferLen, NULL);
                if (bufferLen == 0) {
                    break;
                }

                if (remainder = bufferLen % SECTOR_SIZE) {
                    memset(buffer + bufferLen, 0, SECTOR_SIZE - remainder);
                    bufferLen += SECTOR_SIZE - remainder;
                }

				if (!WriteFile(outputDrive, buffer, bufferLen, &writtenLen, NULL) || writtenLen < bufferLen) {
                    message.Format(_T("Error while writing %s.  Fusing incomplete!"), FILE_TYPES[i]);
                    MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
                    goto Exit;
                }

                m_wndFuseProgress.SetPos(m_wndFuseProgress.GetPos() + bufferLen / SECTOR_SIZE);
                Poll();

                if (bufferLen < sizeof(buffer)) {
                    break;
                }

                if (m_cancel) {
                    goto Exit;
                }
            }

            /* Finally write the partition info area */
            offset.QuadPart = 0;
            SetFilePointerEx(outputDrive, offset, NULL, FILE_BEGIN);
            SetFilePointerEx(inputFile, offset, NULL, FILE_BEGIN);

            ReadFile(inputFile, buffer, SECTOR_SIZE, &bufferLen, NULL);

            if (remainder = bufferLen % SECTOR_SIZE) {
                memset(buffer + bufferLen, 0, SECTOR_SIZE - remainder);
                bufferLen += SECTOR_SIZE - remainder;
            }

            if (!WriteFile(outputDrive, buffer, bufferLen, &writtenLen, NULL) || writtenLen < bufferLen) {
                message.Format(_T("Error while writing %s.  Fusing incomplete! (BL1)"), FILE_TYPES[i]);
                MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
                goto Exit;
            }

            m_wndFuseProgress.SetPos(m_wndFuseProgress.GetPos() + bufferLen / SECTOR_SIZE);
            Poll();

            break;

        case FILE_BOOTLOADER:
            offset.QuadPart = pd.GetSizeInBytes() - BL2_OFFSET;
            if (m_wndSdhcFuseCheck.GetCheck() == BST_CHECKED) {
                offset.QuadPart -= SDHC_OFFSET;
            }
            SetFilePointerEx(outputDrive, offset, NULL, FILE_BEGIN);

            while (true) {
                ReadFile(inputFile, buffer, sizeof(buffer), &bufferLen, NULL);
                if (bufferLen == 0) {
                    break;
                }

                if (remainder = bufferLen % SECTOR_SIZE) {
                    memset(buffer + bufferLen, 0, SECTOR_SIZE - remainder);
                    bufferLen += SECTOR_SIZE - remainder;
                }

				if (!WriteFile(outputDrive, buffer, bufferLen, &writtenLen, NULL) || writtenLen < bufferLen) {
                    message.Format(_T("Error while writing %s.  Fusing incomplete! (BL2)"), FILE_TYPES[i]);
                    MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
                    goto Exit;
                }

                m_wndFuseProgress.SetPos(m_wndFuseProgress.GetPos() + bufferLen / SECTOR_SIZE);
                Poll();

                if (bufferLen < sizeof(buffer)) {
                    break;
                }

                if (m_cancel) {
                    goto Exit;
                }
            }

            offset.QuadPart = pd.GetSizeInBytes() - BL1_OFFSET;
            if (m_wndSdhcFuseCheck.GetCheck() == BST_CHECKED) {
                offset.QuadPart -= SDHC_OFFSET;
            }
            SetFilePointerEx(outputDrive, offset, NULL, FILE_BEGIN);

            offset.QuadPart = 0;
            SetFilePointerEx(inputFile, offset, NULL, FILE_BEGIN);

            ReadFile(inputFile, buffer, BL1_SIZE, &bufferLen, NULL);

            if (remainder = bufferLen % SECTOR_SIZE) {
                memset(buffer + bufferLen, 0, SECTOR_SIZE - remainder);
                bufferLen += SECTOR_SIZE - remainder;
            }

			if (!WriteFile(outputDrive, buffer, bufferLen, &writtenLen, NULL) || writtenLen < bufferLen) {
                message.Format(_T("Error while writing %s.  Fusing incomplete! (BL1)"), FILE_TYPES[i]);
                MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
                goto Exit;
            }

            m_wndFuseProgress.SetPos(m_wndFuseProgress.GetPos() + bufferLen / SECTOR_SIZE);
            Poll();

            break;

        case FILE_ENVIRONMENT:
            offset.QuadPart = pd.GetSizeInBytes() - ENV_OFFSET;
            if (m_wndSdhcFuseCheck.GetCheck() == BST_CHECKED) {
                offset.QuadPart -= SDHC_OFFSET;
            }
            SetFilePointerEx(outputDrive, offset, NULL, FILE_BEGIN);

            while (true) {
                ReadFile(inputFile, buffer, sizeof(buffer), &bufferLen, NULL);
                if (bufferLen == 0) {
                    break;
                }

                if (remainder = bufferLen % SECTOR_SIZE) {
                    memset(buffer + bufferLen, 0, SECTOR_SIZE - remainder);
                    bufferLen += SECTOR_SIZE - remainder;
                }

				if (!WriteFile(outputDrive, buffer, bufferLen, &writtenLen, NULL) || writtenLen < bufferLen) {
                    message.Format(_T("Error while writing %s.  Fusing incomplete!"), FILE_TYPES[i]);
                    MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
                    goto Exit;
                }

                m_wndFuseProgress.SetPos(m_wndFuseProgress.GetPos() + bufferLen / SECTOR_SIZE);
                Poll();

                if (bufferLen < sizeof(buffer)) {
                    break;
                }

                if (m_cancel) {
                    goto Exit;
                }
            }
            break;

        case FILE_KERNEL:
            offset.QuadPart = pd.GetSizeInBytes() - KERNEL_OFFSET;
            if (m_wndSdhcFuseCheck.GetCheck() == BST_CHECKED) {
                offset.QuadPart -= SDHC_OFFSET;
            }
            SetFilePointerEx(outputDrive, offset, NULL, FILE_BEGIN);

            while (true) {
                ReadFile(inputFile, buffer, sizeof(buffer), &bufferLen, NULL);
                if (bufferLen == 0) {
                    break;
                }

                if (remainder = bufferLen % SECTOR_SIZE) {
                    memset(buffer + bufferLen, 0, SECTOR_SIZE - remainder);
                    bufferLen += SECTOR_SIZE - remainder;
                }

				if (!WriteFile(outputDrive, buffer, bufferLen, &writtenLen, NULL) || writtenLen < bufferLen) {
                    message.Format(_T("Error while writing %s.  Fusing incomplete!"), FILE_TYPES[i]);
                    MessageBox(message, _T("Error"), MB_ICONERROR | MB_OK);
                    goto Exit;
                }

                m_wndFuseProgress.SetPos(m_wndFuseProgress.GetPos() + bufferLen / SECTOR_SIZE);
                Poll();

                if (bufferLen < sizeof(buffer)) {
                    break;
                }

                if (m_cancel) {
                    goto Exit;
                }
            }
            break;

        default:
            /* do nothing */
            break;
        }

        if (inputFile != INVALID_HANDLE_VALUE) {
            CloseHandle(inputFile);
            inputFile = INVALID_HANDLE_VALUE;
        }
    }

Exit:
    m_wndFuseProgress.SetPos(0);
    m_wndMessageLabel.SetWindowText(_T(""));

    /* Unlock the GUI */
    for (size_t i = 0; i < NUM_FILES; i++) {
        m_wndFileCheck[i]->EnableWindow(TRUE);
        if (m_wndFileCheck[i]->GetCheck() == BST_CHECKED) {
            m_wndFileEdit[i]->EnableWindow(TRUE);
            m_wndFileBrowseBtn[i]->EnableWindow(TRUE);
        }
    }
    m_wndPhysicalDriveComboBox.EnableWindow(TRUE);
    m_wndSdFuseCheck.EnableWindow(TRUE);
    m_wndSdhcFuseCheck.EnableWindow(TRUE);
    m_wndStartFuseBtn.ShowWindow(SW_SHOW);
    m_wndCancelBtn.ShowWindow(SW_HIDE);

    m_timer = SetTimer(1, 125, NULL);

    if (inputFile != INVALID_HANDLE_VALUE) {
        CloseHandle(inputFile);
    }

    if (outputDrive != INVALID_HANDLE_VALUE) {
        DWORD bytes;
        DeviceIoControl(outputDrive, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &bytes, NULL);
        CloseHandle(outputDrive);
    }

    m_fusing = false;

    if (m_quit) {
        GetParent().DestroyWindow();
    }

    return 0;
}

LRESULT CNanoPiFuserView::OnBnClickedCancelButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_cancel = true;

    return 0;
}