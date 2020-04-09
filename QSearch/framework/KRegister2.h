#ifndef __KRegister2_h__
#define __KRegister2_h__

#include <atlstr.h>
#include <list>

class KRegister2
{
private:
	HKEY m_hKey;

public:
	KRegister2() : m_hKey(NULL){}
	~KRegister2()
	{
		Close();
	}

	void Attach(HKEY hKey)
	{
		Close();
		m_hKey = hKey;
	}

	HKEY Detach(void)
	{
		HKEY hKey = m_hKey;
		m_hKey = NULL;
		return hKey;
	}
	
	BOOL Open(HKEY hRootKey, LPCTSTR szSubKey, BOOL bReadOnly = TRUE)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode;

		Close();

		if (bReadOnly)
		{
			lRetCode = RegOpenKey(hRootKey, szSubKey, &m_hKey);
		}
		else
		{
			lRetCode = RegCreateKeyEx(hRootKey, szSubKey, 0L, NULL,
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
				&m_hKey, NULL);
		}

		if (lRetCode != ERROR_SUCCESS)
			goto Exit0;

		bReturn = TRUE;
Exit0:
		return bReturn;
	}

    BOOL Open32(HKEY hRootKey, LPCTSTR szSubKey, BOOL bReadOnly = TRUE)
    {
        BOOL bReturn = FALSE;
        LONG lRetCode;

        Close();

        if (bReadOnly)
        {
            lRetCode = RegOpenKeyEx(hRootKey, szSubKey, 0, KEY_READ | KEY_WOW64_32KEY, &m_hKey);
            //lRetCode = RegOpenKey(hRootKey, szSubKey, &m_hKey);
        }
        else
        {
            lRetCode = RegCreateKeyEx(hRootKey, szSubKey, 0L, NULL,
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_32KEY, NULL, 
                &m_hKey, NULL);
        }

        if (lRetCode != ERROR_SUCCESS)
            goto Exit0;

        bReturn = TRUE;
Exit0:
        return bReturn;
    }

    BOOL Open64(HKEY hRootKey, LPCTSTR szSubKey, BOOL bReadOnly = TRUE)
    {
        BOOL bReturn = FALSE;
        LONG lRetCode;

        Close();

        if (bReadOnly)
        {
            lRetCode = RegOpenKeyEx(hRootKey, szSubKey, 0, KEY_READ | KEY_WOW64_64KEY, &m_hKey);
            //lRetCode = RegOpenKey(hRootKey, szSubKey, &m_hKey);
        }
        else
        {
            lRetCode = RegCreateKeyEx(hRootKey, szSubKey, 0L, NULL,
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, 
                &m_hKey, NULL);
        }

        if (lRetCode != ERROR_SUCCESS)
            goto Exit0;

        bReturn = TRUE;
Exit0:
        return bReturn;
    }

	BOOL CreateVolatileReg(HKEY hRootKey, LPCTSTR szSubKey)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode;

		Close();

		lRetCode = RegCreateKeyEx(hRootKey, szSubKey, 0L, NULL,
			REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&m_hKey, NULL);

		if (lRetCode != ERROR_SUCCESS)
			goto Exit0;

		bReturn = TRUE;
Exit0:
		return bReturn;
	}

	BOOL CreateNonVolatileReg(HKEY hRootKey, LPCTSTR szSubKey)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode;

		Close();

		lRetCode = RegCreateKeyEx(hRootKey, szSubKey, 0L, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			&m_hKey, NULL);

		if (lRetCode != ERROR_SUCCESS)
			goto Exit0;

		bReturn = TRUE;
Exit0:
		return bReturn;
	}

	BOOL DeleteKey(LPCTSTR szName)
	{
		BOOL bRet = FALSE;

		if (ERROR_SUCCESS == RegDeleteKey(m_hKey, szName))
			bRet = TRUE;

		return bRet;
	}

	BOOL DeleteValue(LPCTSTR szName)
	{
		BOOL bRet = FALSE;

		if (ERROR_SUCCESS == RegDeleteValue(m_hKey, szName))
			bRet = TRUE;

		return bRet;
	}

	void Close(void)
	{
		if (m_hKey) RegCloseKey(m_hKey);
		m_hKey = NULL;
	}

	BOOL Read(LPCTSTR szValueName, DWORD& dwValue)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode = 0;

		DWORD dwType;
		DWORD dwSize = sizeof (DWORD);
		DWORD dwDest;

		if (!m_hKey)
			goto Exit0;

		lRetCode = RegQueryValueEx(m_hKey, szValueName, NULL, 
			&dwType, (BYTE *)&dwDest, &dwSize);
		if (lRetCode != ERROR_SUCCESS || dwType != REG_DWORD)
			goto Exit0;

		dwValue = dwDest;

		bReturn = TRUE;

Exit0:
		return bReturn;
	}

	BOOL Read(LPCTSTR szValueName, CString& strValue)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode;
		DWORD dwType;
		DWORD dwSize = 0;

		if (!m_hKey) goto Exit0;

		lRetCode = RegQueryValueEx(m_hKey, szValueName, NULL,
			&dwType, NULL, &dwSize);
		if (dwType != REG_SZ && dwType != REG_EXPAND_SZ) 
			goto Exit0;
		
		strValue.Empty();
		LPTSTR pBuffer = strValue.GetBuffer(dwSize / 2 + 1);

		lRetCode = RegQueryValueEx(m_hKey, szValueName, NULL,
			&dwType, (BYTE*)pBuffer, &dwSize);
		if (lRetCode == ERROR_SUCCESS)
        {
            pBuffer[dwSize / 2] = 0;
            bReturn = TRUE;
        }

        strValue.ReleaseBuffer();

Exit0:
		return bReturn;
	}

	BOOL Read(LPCTSTR szValueName, BYTE* pBuffer, DWORD& dwSize)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode;
		DWORD dwType;

		if (!m_hKey)
			goto Exit0;

		lRetCode = RegQueryValueEx(m_hKey, szValueName, NULL,
			&dwType, pBuffer, &dwSize);
		if (lRetCode != ERROR_SUCCESS)
			goto Exit0;

		bReturn = TRUE;

Exit0:
		return bReturn;
	}

	BOOL Write(LPCTSTR szValueName, DWORD dwValue)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode;

		if (!m_hKey)
			goto Exit0;

		lRetCode = RegSetValueEx(m_hKey, szValueName, 0L, REG_DWORD,
			(CONST BYTE*)&dwValue, sizeof(DWORD));
		if (lRetCode != ERROR_SUCCESS)
			goto Exit0;

		bReturn = TRUE;

Exit0:
		return bReturn;
	}

	BOOL Write(LPCTSTR szValueName, LPCTSTR szValue)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode;

		if (!m_hKey)
			goto Exit0;

		lRetCode = RegSetValueEx(m_hKey, szValueName, 0L, REG_SZ,
			(CONST BYTE*) szValue, ((int)_tcslen(szValue) + 1) * sizeof(TCHAR));	
		if (lRetCode != ERROR_SUCCESS)
			goto Exit0;

		bReturn = TRUE;

Exit0:
		return bReturn;
	}

	BOOL WriteExpandString(LPCTSTR szValueName, LPCTSTR szValue)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode;

		if (!m_hKey)
			goto Exit0;

		lRetCode = RegSetValueEx(m_hKey, szValueName, 0L, REG_EXPAND_SZ,
			(CONST BYTE*) szValue, ((int)_tcslen(szValue) + 1) * sizeof(TCHAR));	
		if (lRetCode != ERROR_SUCCESS)
			goto Exit0;

		bReturn = TRUE;

Exit0:
		return bReturn;
	}

	BOOL Write(LPCTSTR szValueName, BYTE* pBuffer, DWORD dwSize)
	{
		BOOL bReturn = FALSE;
		LONG lRetCode;

		if (!m_hKey)
			goto Exit0;

		lRetCode = RegSetValueEx(m_hKey, szValueName, 0L, REG_BINARY,
			pBuffer, dwSize);
		if (lRetCode != ERROR_SUCCESS)
			goto Exit0;

		bReturn = TRUE;

Exit0:
		return bReturn;
	}

	static LONG DeleteSubKeyTree(HKEY hKey, LPCTSTR lpSubKey)
	{
		LONG lResult;
		HKEY hSubKey;
		DWORD dwIndex, cbName;
		TCHAR szSubKey[512];
		FILETIME ft;

		lResult = RegOpenKeyEx(hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hSubKey);
		if (lResult != ERROR_SUCCESS)
		{
			return lResult;
		}

		dwIndex = 0;
		cbName = sizeof(szSubKey)/sizeof(szSubKey[0]);
		while (ERROR_SUCCESS == (lResult = RegEnumKeyEx(hSubKey, dwIndex, szSubKey, &cbName, NULL, NULL, NULL, &ft)))
		{
			lResult = DeleteSubKeyTree(hSubKey, szSubKey);
			if (lResult != ERROR_SUCCESS)
			{
				RegCloseKey(hSubKey);
				return lResult;
			}

			cbName = sizeof(szSubKey)/sizeof(szSubKey[0]);
		}

		RegCloseKey(hSubKey);
		lResult = RegDeleteKey(hKey, lpSubKey);

		return lResult;
	}


    BOOL GetKeyList(std::list<CString>& lstKeyList)
    {
        TCHAR szSubKey[1024] = {0};
        DWORD dwIndex = 0;
        DWORD cbName = sizeof(szSubKey) / sizeof(szSubKey[0]);
        FILETIME ft;

        lstKeyList.clear();

        do 
        {
            LONG lResult = RegEnumKeyEx(m_hKey, dwIndex, szSubKey, &cbName, NULL, NULL, NULL, &ft);
            if (ERROR_SUCCESS != lResult) break;

            lstKeyList.push_back(szSubKey);

            cbName = sizeof(szSubKey) / sizeof(szSubKey[0]);

            dwIndex++;

        } while (TRUE);

        return TRUE;
    }
};

#endif