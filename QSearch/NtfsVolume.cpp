#pragma once
#include "stdafx.h"
#include "NtfsVolume.h"

int NtfsVolume::AdjustPrivileges(TCHAR* lpszPrivilege)
{
	HANDLE tokenHandle = 0;
	int ret = 0;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
	{
		LUID luid;
		if(LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
		{
			TOKEN_PRIVILEGES tkPriv;

			tkPriv.PrivilegeCount = 1;
			tkPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			tkPriv.Privileges[0].Luid = luid;

			if (AdjustTokenPrivileges(tokenHandle, FALSE, &tkPriv, 0, NULL, NULL)) 
			{
				ret = 1;
			}
		}
		CloseHandle(tokenHandle);
	}
	return ret;
}

BOOL StringCompare::compareStrFilename(CStringA str, CStringA filename)
{
	int pos = 0;
	int end = str.GetLength(); 
	while ( pos < end )
	{
		pos = str.Find( (' ') );

		CStringA strtmp;
		if ( pos == -1 ) { // �޿ո�
			strtmp = str;
			pos = end;
		} else {
			strtmp = str.Mid(0, pos-0);
		}

		if ( !infilename(strtmp, filename) ) {
			return false;
		}
		str.Delete(0, pos);
		str.TrimLeft((' '));
	}

	return true;
}

BOOL StringCompare::infilename(CStringA &strtmp, CStringA &filename)
{
	CStringA filenametmp(filename);
	int pos;
	if ( !uplow ) { // ��Сд����
		//filenametmp.MakeLower(); // .MakeLower()
		pos = filenametmp.Find(strtmp);
	} else {
		pos = filenametmp.Find(strtmp);
	}

	if ( -1 == pos ) {
		return false;
	}
	if ( !inorder ) { // ��˳��
		filename.Delete(0, pos+1);
	}

	return true;
}

BOOL NtfsVolume::getHandle()
{
	CString lpFileName( _T("\\\\.\\C:\\"));
	lpFileName.SetAt(4, m_chVol);

	m_hVol = CreateFile(lpFileName,
		GENERIC_READ | GENERIC_WRITE, // ����Ϊ0
		FILE_SHARE_READ | FILE_SHARE_WRITE, // ���������FILE_SHARE_WRITE
		NULL,
		OPEN_EXISTING, // �������OPEN_EXISTING, CREATE_ALWAYS���ܻᵼ�´���
		FILE_FLAG_BACKUP_SEMANTICS, //FILE_ATTRIBUTE_READONLY, // FILE_ATTRIBUTE_NORMAL���ܻᵼ�´���
		NULL);

	if (INVALID_HANDLE_VALUE != m_hVol) {
		return true;
	}else{
		int code = GetLastError();
		code = 0;
		return false;
	}
}

BOOL NtfsVolume::createUSN()
{
	CREATE_USN_JOURNAL_DATA cujd;
	cujd.MaximumSize = 0; // 0��ʾʹ��Ĭ��ֵ  
	cujd.AllocationDelta = 0; // 0��ʾʹ��Ĭ��ֵ

	DWORD br = 0;
	if (
		DeviceIoControl( m_hVol,	// handle to volume
		FSCTL_CREATE_USN_JOURNAL,   // dwIoControlCode
		&cujd,						// input buffer
		sizeof(cujd),				// size of input buffer
		NULL,                       // lpOutBuffer
		0,                          // nOutBufferSize
		&br,						// number of bytes returned
		NULL )						// OVERLAPPED structure	
		){	
		return true;
	} else {
		int code = GetLastError();
		code = 0;
		return false;
	}
}


BOOL NtfsVolume::getUSNInfo()
{
	DWORD br = 0;

	if (
		DeviceIoControl( m_hVol, // handle to volume
		FSCTL_QUERY_USN_JOURNAL, // dwIoControlCode
		NULL,					 // lpInBuffer
		0,						 // nInBufferSize
		&m_UsnJournalData,		 // output buffer
		sizeof(m_UsnJournalData), // size of output buffer
		&br,					// number of bytes returned
		NULL )					// OVERLAPPED structure
		) {
		return true;
	} else {
		return false;
	}
}

BOOL NtfsVolume::getUSNJournal()
{
	MFT_ENUM_DATA med;
	med.StartFileReferenceNumber = 0;
	med.LowUsn = m_UsnJournalData.FirstUsn;
	med.HighUsn = m_UsnJournalData.NextUsn;

	CHAR* szBuffer = new CHAR[USN_PAGE_SIZE];
	if (szBuffer == NULL)
	{
		return FALSE;
	}

	DWORD usnDataSize = 0;
	DWORD index = 0;

	while (0 != DeviceIoControl(m_hVol,  
		FSCTL_ENUM_USN_DATA,  
		&med,  
		sizeof(med),  
		szBuffer,  
		USN_PAGE_SIZE,  
		&usnDataSize,  
		NULL))  
	{  
		DWORD dwRetBytes = usnDataSize - sizeof(USN);  
		// �ҵ���һ�� USN ��¼  
		PUSN_RECORD pUsnRecord = (PUSN_RECORD)(((PCHAR)szBuffer) + sizeof(USN));  

		while (dwRetBytes > 0) {
	
			CString _fileName(pUsnRecord->FileName, pUsnRecord->FileNameLength / 2);

			CStringA fileName = CW2A(_fileName, CP_UTF8);

#ifdef TEST
			if (TRUE)
			{
				DWORDLONG FileReferenceNumber = pUsnRecord->FileReferenceNumber;
				DWORDLONG ParentFileReferenceNumber = pUsnRecord->ParentFileReferenceNumber;
				DWORD FileAttributes = pUsnRecord->FileAttributes;
				CStringA FileName = fileName;

				CStringA logstr;
				logstr.Format("[%llx] [%llx] [%02x] [%s]\n",
					FileReferenceNumber,
					ParentFileReferenceNumber,
					FileAttributes, FileName.GetString());

				{
					_wsetlocale(LC_ALL,L"chs"); // ���ó�����
					FILE *fp = NULL;
					static BOOL first = TRUE;
					fp = fopen("D:\\volumez.txt", first ? "w" : "a");
					first = FALSE;
					if (fp)
					{
						fprintf(fp, logstr.GetString());
						fclose(fp);
					}
				}
			}
#endif

			if (fileName.Find("$") == -1 || TRUE)
			{
				KFileNode* fileNode = new KFileNode();
				fileNode->filename = fileName;
				fileNode->frn = pUsnRecord->FileReferenceNumber;
				fileNode->pfrn = pUsnRecord->ParentFileReferenceNumber;
				fileNode->fileAttributes = pUsnRecord->FileAttributes;
				m_vecFileNode.push_back(fileNode);
			}

			// ��ȡ��һ����¼  
			DWORD recordLen = pUsnRecord->RecordLength;  
			dwRetBytes -= recordLen;  
			pUsnRecord = (PUSN_RECORD)(((PCHAR)pUsnRecord) + recordLen);  
		} 

		// ��ȡ��һҳ���� 
		med.StartFileReferenceNumber = *(USN *)szBuffer;  		
	}

	::qsort(&m_vecFileNode[0], m_vecFileNode.size(), sizeof(m_vecFileNode[0]), vecCompare);

	delete[] szBuffer;
	return true;
}

int NtfsVolume::vecCompare( const void * v1, const void * v2 )
{
	KFileNode* p1 = *(KFileNode**)v1;
	KFileNode* p2 = *(KFileNode**)v2;
	if (p1->frn == p2->frn)
		return 0;
	return p1->frn > p2->frn ? 1 : -1;
}

BOOL NtfsVolume::deleteUSN()
{
	DELETE_USN_JOURNAL_DATA dujd;  
	dujd.UsnJournalID = m_UsnJournalData.UsnJournalID;  
	dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;  
	DWORD br = 0;

	if ( DeviceIoControl(m_hVol,  
		FSCTL_DELETE_USN_JOURNAL,  
		&dujd,  
		sizeof(dujd),  
		NULL,  
		0,  
		&br,  
		NULL)
		) {
		return true;
	} else {
		return false;
	}
}

BOOL InitData::isNTFS(char ch)
{
	char lpRootPathName[] = ("C:\\");
	lpRootPathName[0] = ch;

	char lpVolumeNameBuffer[MAX_PATH];
	DWORD lpVolumeSerialNumber = 0;
	DWORD lpMaximumComponentLength = 0;
	DWORD lpFileSystemFlags = 0;
	char lpFileSystemNameBuffer[MAX_PATH];

	if ( GetVolumeInformationA(
		lpRootPathName,
		lpVolumeNameBuffer,
		MAX_PATH,
		&lpVolumeSerialNumber,
		&lpMaximumComponentLength,
		&lpFileSystemFlags,
		lpFileSystemNameBuffer,
		MAX_PATH
		)) {
		if (!strcmp(lpFileSystemNameBuffer, "NTFS")) {
			return true;
		} 
	}
	return false;
}

InitData initdata;
