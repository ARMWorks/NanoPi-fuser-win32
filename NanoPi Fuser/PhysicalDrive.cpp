/*
* Copyright (c) 2000-2003,2010 The NetBSD Foundation, Inc.
* All rights reserved.
*
* Copyright (c) 2000-2003,2010 Martin Husemann <martin@NetBSD.org>.
* Copyright (c) 2010 Izumi Tsutsui <tsutsui@NetBSD.org>
* Copyright (c) 2010 Quentin Garnier <cube@NetBSD.org>
* All rights reserved.
*
* Copyright (c) 2015 Jeff Kent <jeff@jkent.net>
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* 2. The name of the author may not be used to endorse or promote products
* derived from this software withough specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include "stdafx.h"
#include "PhysicalDrive.h"

#pragma comment( lib, "setupapi.lib" )

PhysicalDrive::PhysicalDrive()
    :m_driveNumber(0), m_sizeInBytes(0)
{
}

PhysicalDrive::~PhysicalDrive()
{
}

const CString PhysicalDrive::GetPath(void)
{
    return m_internalFilename;
}

const CString PhysicalDrive::GetDescription(void)
{
    return m_description;
}

/* This code was derived from CRawrite32Dlg::OnWriteImage() from the rawrite32 source */
bool PhysicalDrive::Unmount(void)
{
    for (size_t i = 0; i < m_volumes.size(); i++) {
        CString internalFilename;
        internalFilename.Format(_T("\\\\.\\%s"), m_volumes[i]);
        HANDLE hVolume = CreateFile(internalFilename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hVolume == INVALID_HANDLE_VALUE) {
            continue;
        }
        DWORD bytes;
        if (DeviceIoControl(hVolume, FSCTL_IS_VOLUME_MOUNTED, NULL, 0, NULL, 0, &bytes, NULL)) {
            if (!DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes, NULL)) {
                CloseHandle(hVolume);
                return false;
            }
        }
        CloseHandle(hVolume);
    }
    return true;
}

/* This code was derived from CRawrite32Dlg::EnumPhysicalDrives() from the rawrite32 source */
void PhysicalDrive::Enumerate(vector<PhysicalDrive> &physicalDrives)
{
    TCHAR allDrives[32 * 4 + 10];
    GetLogicalDriveStrings(sizeof(allDrives) / sizeof(allDrives[0]), allDrives);
    LPCTSTR drive;

    physicalDrives.clear();

    for (drive = allDrives; *drive; drive += _tcslen(drive) + 1) {
        DWORD type = GetDriveType(drive);
        if (type != DRIVE_REMOVABLE) {
            continue;
        }

        CString volName(drive, 2);
        PhysicalDrive pd;
        pd.m_volumes.push_back(volName);
        pd.m_driveNumber = ~0U;
        pd.m_internalFilename.Format(_T("\\\\.\\%s"), volName);
        HANDLE hVolume = CreateFile(pd.m_internalFilename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hVolume == INVALID_HANDLE_VALUE) {
            continue;
        }

        DISK_GEOMETRY_EX diskGeometryEx;
        DWORD bytes;
        if (!DeviceIoControl(hVolume, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskGeometryEx, sizeof(DISK_GEOMETRY_EX), &bytes, NULL)) {
            // No medium in drive
            CloseHandle(hVolume);
            continue;
        }

        if (diskGeometryEx.DiskSize.QuadPart > 68719476736LL) {
            // Drive too big
            CloseHandle(hVolume);
            continue;
        }

        if (!DeviceIoControl(hVolume, IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, &bytes, NULL)) {
            // Drive is readonly
            CloseHandle(hVolume);
            continue;
        }

        CloseHandle(hVolume);

        pd.m_sizeInBytes = diskGeometryEx.DiskSize.QuadPart;

        CString logName, fsName;
        GetVolumeInformation(drive, logName.GetBuffer(200), 200, NULL, NULL, NULL, fsName.GetBuffer(200), 200);
        logName.ReleaseBuffer(); fsName.ReleaseBuffer();

        if (logName.IsEmpty()) {
            pd.m_description.Format(_T("%s %s (%u MB)"), volName, fsName, pd.GetSizeInMegabytes());
        }
        else {
            pd.m_description.Format(_T("%s %s, %s (%u MB)"), volName, logName, fsName, pd.GetSizeInMegabytes());
        }

        physicalDrives.push_back(pd);
    }

    size_t errCnt = 0;
    for (DWORD i = 0;; i++) {
        CString internalFilename;
        internalFilename.Format(_T("\\\\.\\PhysicalDrive%u"), i);
        HANDLE hVolume = CreateFile(internalFilename, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hVolume == INVALID_HANDLE_VALUE) {
            errCnt++;
            if (errCnt > 48) {
                break;
            }
            continue;
        }
        errCnt = 0;

        DISK_GEOMETRY_EX diskGeometryEx;
        DWORD bytes;
        if (!DeviceIoControl(hVolume, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskGeometryEx, sizeof(DISK_GEOMETRY_EX), &bytes, NULL)) {
            // No medium in drive
            CloseHandle(hVolume);
            continue;
        }

        if (diskGeometryEx.DiskSize.QuadPart > 68719476736LL) {
            // Drive too big
            CloseHandle(hVolume);
            continue;
        }

        if (!DeviceIoControl(hVolume, IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, &bytes, NULL)) {
            // Drive is readonly
            CloseHandle(hVolume);
            continue;
        }

        PhysicalDrive pd;
        pd.m_driveNumber = i;
        pd.m_internalFilename = internalFilename;
        pd.m_sizeInBytes = diskGeometryEx.DiskSize.QuadPart;

        union {
            STORAGE_DEVICE_DESCRIPTOR header;
            TCHAR buf[1024];
        } desc;
        STORAGE_PROPERTY_QUERY qry;
        memset(&qry, 0, sizeof(qry));
        qry.PropertyId = StorageDeviceProperty;
        qry.QueryType = PropertyStandardQuery;
        CString info;
        if (DeviceIoControl(hVolume, IOCTL_STORAGE_QUERY_PROPERTY, &qry, sizeof(qry), &desc.header, sizeof(desc), &bytes, NULL) && bytes >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
            const char *strings = (const char *)&desc;
            if (desc.header.VendorIdOffset || desc.header.ProductIdOffset) {
                if (desc.header.VendorIdOffset) {
                    CString t(strings + desc.header.VendorIdOffset);
                    t.Trim();
                    info = t;
                }
                if (desc.header.ProductIdOffset) {
                    CString t(strings + desc.header.ProductIdOffset);
                    t.Trim();
                    if (!info.IsEmpty()) {
                        info += " ";
                    }
                    info += t;
                }
                pd.m_description = info;
            }
        }

        if (!info.IsEmpty()) {
            pd.m_description.Format(_T("PhysicalDrive%u: %s (%u MB)"), pd.m_driveNumber, info, pd.GetSizeInMegabytes());
        }
        else {
            pd.m_description.Format(_T("PhysicalDrive%u: (%u MB)"), pd.m_driveNumber, pd.GetSizeInMegabytes());
        }

        CloseHandle(hVolume);
        physicalDrives.push_back(pd);
    }

    for (drive = allDrives; *drive; drive += _tcslen(drive) + 1) {
        CString internalFilename, vol(drive, 2);
        DWORD type = GetDriveType(drive);
        if (type == DRIVE_CDROM || type == DRIVE_REMOTE || type == DRIVE_RAMDISK) {
            continue;
        }

        internalFilename.Format(_T("\\\\.\\%s"), vol);
        HANDLE hVolume = CreateFile(internalFilename, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hVolume == INVALID_HANDLE_VALUE) {
            continue;
        }

        STORAGE_DEVICE_NUMBER sdn;
        DWORD bytes = 0;
        if (!DeviceIoControl(hVolume, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &bytes, NULL)) {
            CloseHandle(hVolume);
            continue;
        }

        for (size_t i = 0; i < physicalDrives.size(); i++) {
            if (physicalDrives[i].m_driveNumber == sdn.DeviceNumber) {
                physicalDrives[i].m_volumes.push_back(vol);
                // check if this volume has already been enumerated before (unpartitioned removable disks)
                for (size_t j = 0; j < physicalDrives.size(); j++) {
                    if (physicalDrives[j].m_driveNumber != ~0U) {
                        continue;
                    }
                    if (i == j) {
                        continue;
                    }
                    if (physicalDrives[j].m_volumes.size() == 1 && physicalDrives[j].m_volumes[0].CompareNoCase(vol) == 0) {
                        physicalDrives.erase(physicalDrives.begin() + j);
                        break;
                    }
                }
            }
        }
        CloseHandle(hVolume);
    }
}