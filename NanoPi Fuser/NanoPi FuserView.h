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

#pragma once

#include "PhysicalDrive.h"

#define WM_REFRESH_DEVICE  (WM_USER + 789)
#define NUM_FILES 4

class CNanoPiFuserView : public CDialogImpl<CNanoPiFuserView>
{
public:
	enum { IDD = IDD_NANOPIFUSER_FORM };

    CNanoPiFuserView();
    ~CNanoPiFuserView();

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CNanoPiFuserView)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnCloseDialog)
        MESSAGE_HANDLER(WM_REFRESH_DEVICE, OnRefreshDevices)
        MESSAGE_HANDLER(WM_TIMER, OnRefreshDevices)
        COMMAND_HANDLER(IDC_FILE1_CHECK, BN_CLICKED, OnBnClickedFileCheck)
        COMMAND_HANDLER(IDC_FILE2_CHECK, BN_CLICKED, OnBnClickedFileCheck)
        COMMAND_HANDLER(IDC_FILE3_CHECK, BN_CLICKED, OnBnClickedFileCheck)
        COMMAND_HANDLER(IDC_FILE4_CHECK, BN_CLICKED, OnBnClickedFileCheck)
        COMMAND_HANDLER(IDC_FILE1_BROWSE_BUTTON, BN_CLICKED, OnBnClickedFileBrowseButton)
        COMMAND_HANDLER(IDC_FILE2_BROWSE_BUTTON, BN_CLICKED, OnBnClickedFileBrowseButton)
        COMMAND_HANDLER(IDC_FILE3_BROWSE_BUTTON, BN_CLICKED, OnBnClickedFileBrowseButton)
        COMMAND_HANDLER(IDC_FILE4_BROWSE_BUTTON, BN_CLICKED, OnBnClickedFileBrowseButton)
        COMMAND_HANDLER(IDC_PHYSICAL_DRIVE_COMBOBOX, CBN_SELCHANGE, OnCbnSelchangePhysicalDriveCombobox)
        COMMAND_HANDLER(IDC_PHYSICAL_DRIVE_COMBOBOX, CBN_DROPDOWN, OnCbnDropdownPhysicalDriveCombobox)
        COMMAND_HANDLER(IDC_PHYSICAL_DRIVE_COMBOBOX, CBN_CLOSEUP, OnCbnCloseupPhysicalDriveCombobox)
        COMMAND_HANDLER(IDC_START_FUSE_BUTTON, BN_CLICKED, OnBnClickedStartFuseButton)
        COMMAND_HANDLER(IDC_CANCEL_BUTTON, BN_CLICKED, OnBnClickedCancelButton)
        ALT_MSG_MAP(1)
    END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnCloseDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnRefreshDevices(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedFileCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedFileBrowseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/);
    LRESULT OnBnClickedRefreshButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCbnSelchangePhysicalDriveCombobox(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCbnDropdownPhysicalDriveCombobox(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCbnCloseupPhysicalDriveCombobox(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedBrowseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedStartFuseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedCancelButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

protected:
    CContainedWindowT<CButton> *m_wndFileCheck[NUM_FILES];
    CContainedWindowT<CEdit> *m_wndFileEdit[NUM_FILES];
    CContainedWindowT<CButton> *m_wndFileBrowseBtn[NUM_FILES];
    CContainedWindowT<CComboBox> m_wndPhysicalDriveComboBox;
    CContainedWindowT<CButton> m_wndSdFuseCheck, m_wndSdhcFuseCheck;
    CContainedWindowT<CProgressBarCtrl> m_wndFuseProgress;
    CContainedWindowT<CStatic> m_wndMessageLabel;
    CContainedWindowT<CButton> m_wndStartFuseBtn;
    CContainedWindowT<CButton> m_wndCancelBtn;

    UINT m_timer;

    vector<PhysicalDrive> m_physicalDrives;
    bool m_fusing, m_quit, m_cancel;
    CString m_lastSelectedPath;
    LONGLONG m_lastSelectedSize;
};