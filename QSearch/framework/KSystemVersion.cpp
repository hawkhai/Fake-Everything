#include "stdafx.h"
#include <windows.h>
#include "KSystemVersion.h"
#include "KRegister2.h"

#pragma comment(lib, "version.lib")

//-------------------------------------------------------------------------

typedef void (WINAPI *LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO);

//-------------------------------------------------------------------------

KSystemVersion::enumSystemVersion KSystemVersion::GetSystemVersion()
{
    static enumSystemVersion OsPlatform = enumSystemVersionNone;
    if (OsPlatform == enumSystemVersionNone)
        OsPlatform = GetSystemVersionImpl();

    return OsPlatform;
}

namespace
{
	int _GetPEFileVersion(LPCTSTR szFileName, DWORD* pdwHighVersion, DWORD* pdwLowVersion)
	{	
		BOOL bReturn = FALSE;
		DWORD dwHandle = 0;
		PBYTE pBuffer = NULL;

		*pdwHighVersion = *pdwLowVersion = 0;
		DWORD dwRet = ::GetFileVersionInfoSize(szFileName, &dwHandle);
		if (dwRet == 0) goto Exit0;

		pBuffer = new BYTE[dwRet];
		BOOL bRet = ::GetFileVersionInfo(szFileName, dwHandle, dwRet, pBuffer);
		if (!bRet) goto Exit0;

		UINT uLen = 0;
		VS_FIXEDFILEINFO *pFixFileInfo = NULL;
		bRet = ::VerQueryValue(pBuffer, L"\\", (LPVOID *)&pFixFileInfo, &uLen);
		if (!bRet || !uLen) goto Exit0;

		*pdwHighVersion = pFixFileInfo->dwFileVersionMS;
		*pdwLowVersion = pFixFileInfo->dwFileVersionLS;

		bReturn = TRUE;
Exit0:
		if (pBuffer) delete[] pBuffer;
		return bReturn;
	}

    BOOL _GetNtVersionNumbers(DWORD& dwMajorVer, DWORD& dwMinorVer, DWORD& dwBuildNumber)
    {
        HMODULE hModNtdll = ::GetModuleHandle(_T("ntdll.dll"));
        if (!hModNtdll)
            return FALSE;

        typedef void (WINAPI *PFN_RtlGetNtVersionNumbers)(DWORD*, DWORD*, DWORD*);
        PFN_RtlGetNtVersionNumbers pfnRtlGetNtVersionNumbers = (PFN_RtlGetNtVersionNumbers)::GetProcAddress(hModNtdll, "RtlGetNtVersionNumbers");
        if (!pfnRtlGetNtVersionNumbers)
            return FALSE;

        pfnRtlGetNtVersionNumbers(&dwMajorVer, &dwMinorVer, &dwBuildNumber);
        dwBuildNumber &= 0x0ffff;
        return TRUE;
    }

	void _PathAddBackslash(CString& strPath)
	{
		CString strTemp;
		strTemp = strPath.Right(1);

		if (strTemp != _T("\\") && strTemp != _T("/"))
			strPath.Append(_T("\\"));
	}

	CString _GetSystem32Path()
	{
		CString strPath;
		TCHAR szPath[MAX_PATH] = {0};

		GetSystemDirectory(szPath, MAX_PATH);
		strPath = szPath;

		_PathAddBackslash(strPath);
		return strPath;
	}
}

KSystemVersion::enumSystemVersion KSystemVersion::_GetSysTemVersionByPEVersionOf_ntoskrnl()
{
    DWORD dwMajorVersion = 0;
    DWORD dwMinVersion = 0;
    DWORD dwBuildNumber = 0;

    if (!_GetNtVersionNumbers(dwMajorVersion, dwMinVersion, dwBuildNumber))
    {
        DWORD dwHighVersion = 0;
        DWORD dwLowVersion = 0;

        if (_GetPEFileVersion(_GetSystem32Path() + L"ntoskrnl.exe", &dwHighVersion, &dwLowVersion)) 
        {
            dwMajorVersion = HIWORD(dwHighVersion);
            dwMinVersion = LOWORD(dwHighVersion);
        }
    }

    if (dwMajorVersion == 6 && dwMinVersion == 2)
        return enumSystemVersionWin8;
    else if (dwMajorVersion == 6 && dwMinVersion == 3 )
        return enumSystemVersionWin8_1;
    else if (dwMajorVersion == 6 && dwMinVersion == 4 )
        return enumSystemVersionWin10;
    else if (dwMajorVersion == 10 && dwMinVersion == 0)
        return enumSystemVersionWin10;

    return enumSystemVersionWin8;
}

KSystemVersion::enumSystemVersion KSystemVersion::GetSystemVersionImpl()
{
    int nRetCode = false;
    enumSystemVersion OsPlatform = enumSystemVersionNone;

    OSVERSIONINFOEX osvi;  
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    nRetCode = ::GetVersionEx((OSVERSIONINFO *)&osvi);
    if (FALSE == nRetCode)
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        nRetCode = ::GetVersionEx((OSVERSIONINFO *)&osvi);
    }

    if (nRetCode == FALSE)
        goto Exit0;

    switch(osvi.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
		{
        if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
            OsPlatform = (osvi.wProductType == VER_NT_WORKSTATION) ? enumSystemVersionWin7 : enumSystemVersionWin2008;
        else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
            OsPlatform = (osvi.wProductType == VER_NT_WORKSTATION) ? enumSystemVersionVista : enumSystemVersionWin2008;
        else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
            OsPlatform = enumSystemVersionWin2003;
        else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
            OsPlatform = enumSystemVersionWinXp;
        else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
            OsPlatform = enumSystemVersionWin2000;
        else if (osvi.dwMajorVersion <= 4)
            OsPlatform = enumSystemVersionWinNT;
		else if (((osvi.dwMajorVersion >= 6 && osvi.dwMinorVersion >= 2) || osvi.dwMajorVersion > 6) && osvi.wProductType == VER_NT_WORKSTATION)
		{
			OsPlatform = _GetSysTemVersionByPEVersionOf_ntoskrnl();
		}
		}
        break;
    case VER_PLATFORM_WIN32_WINDOWS:
        if (((osvi.dwBuildNumber >> 16) & 0x0000FFFF) < 0x045A)
            OsPlatform = enumSystemVersionWin9X;
        else 
            OsPlatform = enumSystemVersionWinMe;
        break;
    default:
        OsPlatform = enumSystemVersionNone;
        break;
    }

Exit0:
    return OsPlatform;
}

KSystemVersion::EM_PROCESSOR_ARCHITECTURE KSystemVersion::GetProcessorArchitecture()
{
    static EM_PROCESSOR_ARCHITECTURE emProcArch = enum_processor_architecture__none;
    if (enum_processor_architecture__none == emProcArch)
    {
        emProcArch = enum_processor_architecture__32_bit;
        LPFN_GetNativeSystemInfo pGNSI = (LPFN_GetNativeSystemInfo)::GetProcAddress(
            ::GetModuleHandleW(L"kernel32.dll"),
            "GetNativeSystemInfo");
        if (NULL != pGNSI)
        {
            SYSTEM_INFO si = {0};
            pGNSI(&si);
            if (PROCESSOR_ARCHITECTURE_IA64 == si.wProcessorArchitecture ||
                PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture ||
                PROCESSOR_ARCHITECTURE_ALPHA64 == si.wProcessorArchitecture)
            {
                emProcArch = enum_processor_architecture__64_bit;
            }
        }
    }

    return emProcArch;
}

BOOL KSystemVersion::IsVistaOrLater()
{
    BOOL bRet = FALSE;
    enumSystemVersion emSystemVersion = GetSystemVersion();

    if ( emSystemVersion == enumSystemVersionVista   ||
        emSystemVersion == enumSystemVersionWin7    ||
        emSystemVersion == enumSystemVersionWin2008 ||
        emSystemVersion == enumSystemVersionWin8 ||
        emSystemVersion == enumSystemVersionWin8_1 ||
        emSystemVersion == enumSystemVersionWin10)
    {
        bRet = TRUE;
    }

    return bRet;
}

BOOL KSystemVersion::GetSystemVersion(DWORD &dwMarjorVersion, DWORD &dwMinorVersion)
{
    BOOL bReturn = FALSE;

    //注册表方法在win6.2版本后不准确
//    bReturn = GetSystemVersionByReg(dwMarjorVersion, dwMinorVersion);
//    if (bReturn) goto Exit0;

    bReturn = GetSystemVersionByApi(dwMarjorVersion, dwMinorVersion);

//Exit0:
    return bReturn;
}

BOOL KSystemVersion::GetSystemServicePack(DWORD &dwServicePackMajor, DWORD &dwServicePackMinor)
{
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    BOOL bSuccess = ::GetVersionEx((OSVERSIONINFO *)&osvi);
    if (!bSuccess)
    {
        return FALSE;
    }

    dwServicePackMajor = osvi.wServicePackMajor;
    dwServicePackMinor = osvi.wServicePackMinor;

    return TRUE;
}

BOOL KSystemVersion::GetNtVersionNumbers(DWORD &dwMarjorVersion, DWORD &dwMinorVersion, DWORD &dwBuildNumber)
{
    BOOL bRet = FALSE;

    bRet = _GetNtVersionNumbers(dwMarjorVersion, dwMinorVersion, dwBuildNumber);
    if(!bRet)
    {
        bRet = GetSystemBuildNumberByReg(dwBuildNumber);
    }

    return bRet;
}
BOOL KSystemVersion::GetSystemVersionByReg(DWORD &dwMarjorVersion, DWORD &dwMinorVersion)
{
    BOOL bReturn = FALSE;
    BOOL bRetCode = FALSE;
    KRegister2 regKey;
    CString	   strVer;
    CString strMarjorVer;
    CString strMinorVer;

    bRetCode = regKey.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"));
    if (!bRetCode) goto Exit0;

    bRetCode = regKey.Read(_T("CurrentVersion"), strVer);
    if (!bRetCode || strVer.IsEmpty()) goto Exit0;

    int nPos = strVer.Find(_T("."));
    if (nPos == -1) goto Exit0;

    strMarjorVer = strVer.Left(nPos);
    strMinorVer = strVer.Mid(nPos + 1);

    if (strMarjorVer.IsEmpty() || strMinorVer.IsEmpty())
        goto Exit0;

    dwMarjorVersion = _tcstol(strMarjorVer, NULL, 10);
    dwMinorVersion = _tcstol(strMinorVer, NULL, 10);
    bReturn = TRUE;

Exit0:
    return bReturn;
}

BOOL KSystemVersion::GetSystemBuildNumberByReg(DWORD &dwBuildNumber)
{
    BOOL bReturn = FALSE;
    BOOL bRetCode = FALSE;
    KRegister2 regKey;
    CString strBuildNumber;

    bRetCode = regKey.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"));
    if (!bRetCode) goto Exit0;

    bRetCode = regKey.Read(_T("CurrentBuildNumber"), strBuildNumber);
    if (!bRetCode || strBuildNumber.IsEmpty())
        goto Exit0;
    dwBuildNumber = _tcstol(strBuildNumber, NULL, 10);

    bReturn = TRUE;

Exit0:
    return bReturn;
}

BOOL KSystemVersion::GetSystemVersionByApi( DWORD &dwMarjorVersion, DWORD &dwMinorVersion )
{
    BOOL bReturn = FALSE;

    OSVERSIONINFOEX osvi;  
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    bReturn = ::GetVersionEx((OSVERSIONINFO *)&osvi);
    if (!bReturn)
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        bReturn = ::GetVersionEx((OSVERSIONINFO *)&osvi);
    }

    if (bReturn == FALSE)
        goto Exit0;

    dwMarjorVersion = osvi.dwMajorVersion;
    dwMinorVersion = osvi.dwMinorVersion;

    if (((osvi.dwMajorVersion >= 6 && osvi.dwMinorVersion >= 2) || osvi.dwMajorVersion > 6) && osvi.wProductType == VER_NT_WORKSTATION)
    {
        DWORD dwTempMajorVersion = 0;
        DWORD dwTempMinVersion = 0;
        DWORD dwBuildNumber = 0;

        if (!_GetNtVersionNumbers(dwTempMajorVersion, dwTempMinVersion, dwBuildNumber))
        {
            DWORD dwHighVersion = 0;
            DWORD dwLowVersion = 0;

            if (_GetPEFileVersion(_GetSystem32Path() + L"ntoskrnl.exe", &dwHighVersion, &dwLowVersion)) 
            {
                dwTempMajorVersion = HIWORD(dwHighVersion);
                dwTempMinVersion = LOWORD(dwHighVersion);
            }
        }
        dwMarjorVersion = dwTempMajorVersion;
        dwMinorVersion = dwTempMinVersion;
    }

Exit0:
    return bReturn;
}

KSystemVersion::enumSystemVersionEx KSystemVersion::GetSystemVersionEx()
{
	enumSystemVersionEx version = enumSystemVersionExNone;

	switch(GetSystemVersion())
	{
	case enumSystemVersionWinXp :   version = enumSystemVersionExXP_32;      break;
	case enumSystemVersionVista :   version = enumSystemVersionExVista_32;   break;
	case enumSystemVersionWin7 :    version = enumSystemVersionExWin7_32;    break;
	case enumSystemVersionWin2008 : version = enumSystemVersionExWin2008_32; break;
	case enumSystemVersionWin8 :    version = enumSystemVersionExWin8_32;    break;
	case enumSystemVersionWin8_1 :  version = enumSystemVersionExWin8_1_32;  break;
	case enumSystemVersionWin10 :   version = enumSystemVersionExWin10_32;   break;
	default: version = enumSystemVersionExNone;
	}

	switch(GetProcessorArchitecture())
	{
	case enum_processor_architecture__32_bit : break;
	case enum_processor_architecture__64_bit : version = static_cast<enumSystemVersionEx>(version + 1); break;
	default: version = enumSystemVersionExNone;
	}

	return version;
}

DWORD KSystemVersion::GetWin10VersionBuildNumber()
{
    KSystemVersion::enumSystemVersionEx version = KSystemVersion::GetSystemVersionEx();
    if(version == KSystemVersion::enumSystemVersionExWin10_32 || version == KSystemVersion::enumSystemVersionExWin10_64)
    {
        DWORD dwMajorVersion = 0;
        DWORD dwMinVersion = 0;
        DWORD dwBuildNumber = 0;

        if(_GetNtVersionNumbers(dwMajorVersion, dwMinVersion, dwBuildNumber))
        {
            return dwBuildNumber;
        }
    }

    return -1;	
}

