#ifndef _KSYSTEMVERSION_H_
#define _KSYSTEMVERSION_H_

class KSystemVersion
{
public:
    enum enumSystemVersion
    {
        enumSystemVersionNone       = 0,
        enumSystemVersionWin9X      = 1,
        enumSystemVersionWinMe      = 2,
        enumSystemVersionWin2000    = 3,
        enumSystemVersionWinNT      = 4,
        enumSystemVersionWin2003    = 5,
        enumSystemVersionWinXp      = 6,
        enumSystemVersionVista      = 7,
        enumSystemVersionWin7       = 8,
        enumSystemVersionWin2008    = 9,
        enumSystemVersionWin8       = 10,
        enumSystemVersionWin8_1     = 11,
		enumSystemVersionWin10      = 12,
    };
    static enumSystemVersion GetSystemVersion();

    enum EM_PROCESSOR_ARCHITECTURE
    {
        enum_processor_architecture__none,
        enum_processor_architecture__32_bit,    // 32位操作系统
        enum_processor_architecture__64_bit,    // 64位操作系统
    };
    static EM_PROCESSOR_ARCHITECTURE GetProcessorArchitecture();

	enum enumSystemVersionEx
	{
		enumSystemVersionExNone         = 0,
		enumSystemVersionExXP_32	    = 1,
		enumSystemVersionExXP_64		= 2,
		enumSystemVersionExVista_32		= 3,
		enumSystemVersionExVista_64		= 4,
		enumSystemVersionExWin7_32		= 5,
		enumSystemVersionExWin7_64		= 6,
		enumSystemVersionExWin2008_32   = 7,
		enumSystemVersionExWin2008_64	= 8,
		enumSystemVersionExWin8_32		= 9,
		enumSystemVersionExWin8_64		= 10,
		enumSystemVersionExWin8_1_32	= 11,
		enumSystemVersionExWin8_1_64	= 12,
		enumSystemVersionExWin10_32 	= 13,
		enumSystemVersionExWin10_64 	= 14,
	};
	static enumSystemVersionEx GetSystemVersionEx();

    static BOOL IsVistaOrLater();

    static BOOL GetSystemVersion(DWORD &dwMarjorVersion, DWORD &dwMinorVersion);
    static BOOL GetSystemServicePack(DWORD &dwServicePackMajor, DWORD &dwServicePackMinor);
    static BOOL GetNtVersionNumbers(DWORD &dwMarjorVersion, DWORD &dwMinorVersion, DWORD &dwBuildNumber);
    static DWORD GetWin10VersionBuildNumber();
private:
	static enumSystemVersion _GetSysTemVersionByPEVersionOf_ntoskrnl();
    static enumSystemVersion GetSystemVersionImpl();
    static BOOL GetSystemVersionByReg(DWORD &dwMarjorVersion, DWORD &dwMinorVersion);
    static BOOL GetSystemVersionByApi(DWORD &dwMarjorVersion, DWORD &dwMinorVersion);
    static BOOL GetSystemBuildNumberByReg(DWORD &dwBuildNumber);
};

#endif