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

class PhysicalDrive
{
public:
    PhysicalDrive();
    ~PhysicalDrive();

    LONGLONG GetSizeInBytes(void) { return m_sizeInBytes; }
    DWORD GetSizeInSectors(void) { return (DWORD)(m_sizeInBytes / 512); }
    DWORD GetSizeInMegabytes(void) { return (DWORD)(m_sizeInBytes / 1000 / 1000); }
    const CString GetPath(void);
    const CString GetDescription(void);
    bool Unmount(void);

    static void Enumerate(vector<PhysicalDrive> &physicalDisks);

protected:
    vector<CString> m_volumes;
    DWORD m_driveNumber;
    CString m_internalFilename;
    CString m_description;
    LONGLONG m_sizeInBytes;
};

