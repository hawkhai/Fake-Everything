#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <Winioctl.h>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
//#include <Windows.h>
//#define TEST
#define MAXVOL 3

using namespace std;

typedef struct _name_cur {
	CString filename;
	DWORDLONG pfrn;
}Name_Cur;

typedef struct _pfrn_name {
	DWORDLONG pfrn;
	CString filename;
}Pfrn_Name;
typedef map<DWORDLONG, Pfrn_Name> Frn_Pfrn_Name_Map;

class cmpStrStr {
public:
	cmpStrStr(bool uplow, bool inorder) {
		this->uplow = uplow;
		this->isOrder = inorder;
	}
	~cmpStrStr() {};
	bool cmpStrFilename(CString str, CString filename);
	bool infilename(CString &strtmp, CString &filenametmp);
private:
	bool uplow;
	bool isOrder;
};


int AdjustPrivileges(TCHAR* lpszPrivilege)
{
	HANDLE token_handle;
	int ret=0;

	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token_handle))
	{
		LUID luid;
		if(LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
		{
			TOKEN_PRIVILEGES tk_priv;

			tk_priv.PrivilegeCount=1;
			tk_priv.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
			tk_priv.Privileges[0].Luid=luid;

			if(AdjustTokenPrivileges(token_handle,FALSE,&tk_priv,0,NULL,NULL)) 
			{
				ret=1;
			}
		}
		CloseHandle(token_handle);
	}
	return ret;
}

class Volume {
public:
	Volume(char vol) {
		this->vol = vol;
		hVol = NULL;
		path = "";

		AdjustPrivileges(SE_MANAGE_VOLUME_NAME);

		WCHAR szSystemDrive[MAX_PATH] = {0};
		::GetSystemDirectory(szSystemDrive, MAX_PATH);

		szSystemDrive[0] = 0; // +		szSystemDrive	0x04a8fb88 "C:\WINDOWS\system32"	wchar_t [260]
	}
	~Volume() {
//		CloseHandle(hVol);
	}

	bool initVolume() {
		// 2.��ȡ�����̾��
		if (!getHandle()) return false;

		// 3.����USN��־
		if (!createUSN()) return false;

		// 4.��ȡUSN��־��Ϣ
		if (!getUSNInfo()) return false;

		// 5.��ȡ USN Journal �ļ��Ļ�����Ϣ
		if (!getUSNJournal()) return false;

		// 06. ɾ�� USN ��־�ļ� ( Ҳ���Բ�ɾ�� ) 
		if (!deleteUSN()) return false;

		return false;
	}

	bool isIgnore( vector<string>* pignorelist ) {
		string tmp = CW2A(path);
		for ( vector<string>::iterator it = pignorelist->begin();
			it != pignorelist->end(); ++it ) {
				size_t i = it->length();
				if ( !tmp.compare(0, i, *it,0, i) ) {
					return true;
				}
		}

		return false;
	}

	bool getHandle();
	bool createUSN();
	bool getUSNInfo();
	bool getUSNJournal();
	bool deleteUSN();

	vector<CString> findFile( CString str, cmpStrStr& cmpstrstr, vector<string>* pignorelist );
	
	CString getPath(DWORDLONG frn, CString &path);

	vector<CString> rightFile;	 // ���

private:
	char vol;
	HANDLE hVol;
	vector<Name_Cur> VecNameCur;		// ����1
	Name_Cur nameCur;
	Pfrn_Name pfrnName;
	Frn_Pfrn_Name_Map frnPfrnNameMap;	// ����2

	
	CString path;

	USN_JOURNAL_DATA ujd;
	CREATE_USN_JOURNAL_DATA cujd;
};
	
CString Volume::getPath(DWORDLONG frn, CString &path) {
	// ����2
	Frn_Pfrn_Name_Map::iterator it = frnPfrnNameMap.find(frn);
	if (it != frnPfrnNameMap.end()) {
		if ( 0 != it->second.pfrn ) {
			getPath(it->second.pfrn, path);
		}
		path += it->second.filename;
		path += ( _T("\\") );
	}

	return path;
}

vector<CString> Volume::findFile( CString str, cmpStrStr& cmpstrstr, vector<string>* pignorelist) {
	
	// ���� VecNameCur
	// ͨ��һ��ƥ�亯���� �õ����ϵ� filename
	// ���� frnPfrnNameMap���ݹ�õ�·��

	for ( vector<Name_Cur>::const_iterator cit = VecNameCur.begin();
		cit != VecNameCur.end(); ++cit) {
		if ( cmpstrstr.cmpStrFilename(str, cit->filename) ) {
			path.Empty();
			// ��ԭ ·��
			// vol:\  path \ cit->filename
			getPath(cit->pfrn, path);
			path += cit->filename;


			// path����ȫ·��
			
			if ( isIgnore(pignorelist) ) {
				continue;
			}	
			rightFile.push_back(path);
			//path.Empty();
		}
	}

	return rightFile;
}

bool cmpStrStr::cmpStrFilename(CString str, CString filename) {
	// TODO ������ƥ�亯��

	int pos = 0;
	int end = str.GetLength(); 
	while ( pos < end ) {
		// ����str��ȡ�� ÿ���ո�ֿ�Ϊ�Ĺؼ���
		pos = str.Find( _T(' ') );

		CString strtmp;
		if ( pos == -1 ) {
			// �޿ո�
			strtmp = str;
			pos = end;
		} else {
			strtmp = str.Mid(0, pos-0);
		}

		if ( !infilename(strtmp, filename) ) {
			return false;
		}
		str.Delete(0, pos);
		str.TrimLeft(' ');
	}
	
	return true;
}

bool cmpStrStr::infilename(CString &strtmp, CString &filename) {
	CString filenametmp(filename);
	int pos;
	if ( !uplow ) {
		// ��Сд����
		filenametmp.MakeLower();
		pos = filenametmp.Find(strtmp.MakeLower());
	} else {
		pos = filenametmp.Find(strtmp);
	}
	
	if ( -1 == pos ) {
		return false;
	}
	if ( !isOrder ) {
		// ��˳��
		filename.Delete(0, pos+1);
	}

	return true;
}

bool Volume::getHandle() {
	// Ϊ\\.\C:����ʽ
	CString lpFileName( _T("\\\\.\\c:\\")); // "\\.\C:\"
	lpFileName.SetAt(4, vol);


	hVol = CreateFile(lpFileName,
		GENERIC_READ | GENERIC_WRITE, // ����Ϊ0
		FILE_SHARE_READ | FILE_SHARE_WRITE, // ���������FILE_SHARE_WRITE
		NULL,
		OPEN_EXISTING, // �������OPEN_EXISTING, CREATE_ALWAYS���ܻᵼ�´���
		FILE_FLAG_BACKUP_SEMANTICS, //FILE_ATTRIBUTE_READONLY, // FILE_ATTRIBUTE_NORMAL���ܻᵼ�´���
		NULL);

	if (INVALID_HANDLE_VALUE!=hVol){
		return true;
	}else{
		int code = GetLastError();
		code = 0;
				//MessageBox(NULL, _T("USN����"), _T("����"), MB_OK);
		return false;
//		exit(1);
	}
}

bool Volume::createUSN() {
	cujd.MaximumSize = 0; // 0��ʾʹ��Ĭ��ֵ  
	cujd.AllocationDelta = 0; // 0��ʾʹ��Ĭ��ֵ

	DWORD br = 0;

	if (
		DeviceIoControl( hVol,// handle to volume
		FSCTL_CREATE_USN_JOURNAL,      // dwIoControlCode
		&cujd,           // input buffer
		sizeof(cujd),         // size of input buffer
		NULL,                          // lpOutBuffer
		0,                             // nOutBufferSize
		&br,     // number of bytes returned
		NULL ) // OVERLAPPED structure	
		){	
			return true;
	} else {
		int code = GetLastError();
		code = 0;
		return false;
	}
}


bool Volume::getUSNInfo() {
	DWORD br;
	if (
		DeviceIoControl( hVol, // handle to volume
		FSCTL_QUERY_USN_JOURNAL,// dwIoControlCode
		NULL,            // lpInBuffer
		0,               // nInBufferSize
		&ujd,     // output buffer
		sizeof(ujd),  // size of output buffer
		&br, // number of bytes returned
		NULL ) // OVERLAPPED structure
		) {
			return true;
	} else {
		return false;
	}
}

bool Volume::getUSNJournal() {
	MFT_ENUM_DATA med;
	med.StartFileReferenceNumber = 0;
	med.LowUsn = ujd.FirstUsn;
	med.HighUsn = ujd.NextUsn;

	// ��Ŀ¼
	CString tmp(_T("C:"));
	tmp.SetAt(0, vol);
	frnPfrnNameMap[0x5000000000005].filename = tmp;
	frnPfrnNameMap[0x5000000000005].pfrn = 0;

#define BUF_LEN 0x10000	// �����ܵش����Ч��

	CHAR Buffer[BUF_LEN];
	DWORD usnDataSize;
	PUSN_RECORD UsnRecord;
	int USN_counter = 0;

	// ͳ���ļ���...

	while (0!=DeviceIoControl(hVol,  
		FSCTL_ENUM_USN_DATA,  
		&med,  
		sizeof (med),  
		Buffer,  
		BUF_LEN,  
		&usnDataSize,  
		NULL))  
	{  

		DWORD dwRetBytes = usnDataSize - sizeof (USN);  
		// �ҵ���һ�� USN ��¼  
		UsnRecord = (PUSN_RECORD)(((PCHAR)Buffer)+sizeof (USN));  

		while (dwRetBytes>0){  
			// ��ȡ������Ϣ  	
			CString CfileName(UsnRecord->FileName, UsnRecord->FileNameLength/2);

			pfrnName.filename = nameCur.filename = CfileName;
			pfrnName.pfrn = nameCur.pfrn = UsnRecord->ParentFileReferenceNumber;

			// Vector
			VecNameCur.push_back(nameCur);

			// ����hash...
			frnPfrnNameMap[UsnRecord->FileReferenceNumber] = pfrnName;
			// ��ȡ��һ����¼  
			DWORD recordLen = UsnRecord->RecordLength;  
			dwRetBytes -= recordLen;  
			UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord)+recordLen);  

		} 
		// ��ȡ��һҳ���� 
		med.StartFileReferenceNumber = *(USN *)&Buffer;  		
	}

	return true;
}

bool Volume::deleteUSN() {
	DELETE_USN_JOURNAL_DATA dujd;  
	dujd.UsnJournalID = ujd.UsnJournalID;  
	dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;  
	DWORD br;

	if ( DeviceIoControl(hVol,  
		FSCTL_DELETE_USN_JOURNAL,  
		&dujd,  
		sizeof (dujd),  
		NULL,  
		0,  
		&br,  
		NULL)
		) {
			CloseHandle(hVol);
			return true;
	} else {
		CloseHandle(hVol);
		return false;
	}
}

/*
 *	����һ���м����̷���NTFS��ʽ
 */
class InitData {
public:	

	bool isNTFS(char c);

	list<Volume> volumelist;
	UINT initvolumelist(LPVOID vol) {
		char c = (char)vol;
		Volume volume(c);
		volume.initVolume();
		volumelist.push_back(volume);

		return 1;
	}
/*
	static UINT initThread(LPVOID pParam) {
		InitData * pObj = (InitData*)pParam;
		if ( pObj ) {
			return pObj->init(NULL);
		}
		return false;
	}
*/
	UINT init(LPVOID lp) {

#ifdef TEST
		for ( i=j=0; i<MAXVOL; ++i ) {
#else
		for ( i=j=0; i<26; ++i ) {
#endif
			cvol = i+'A';
			if ( isNTFS(cvol) ) {
				vol[j++] = cvol;
			}
		}
		/*
		CString showpro(_T("����ͳ��"));
		for ( i=0; i<j; ++i ) {
			initvolumelist((LPVOID)vol[i]);
			GetDlgItem(IDC_SHOWPRO)->SetWindowText(showpro + _T(vol[i]));
		}
		*/
		return true;
	}

	int getJ() {
		return j;
	}
	char * getVol() {
		return vol;
	}

	vector<string>* getIgnorePath() {
		ignorepath.clear();
		ifstream fin("config.ini");
		string tmp;
		while ( getline( fin,tmp ) ) {
			ignorepath.push_back(tmp);
		}
		return &ignorepath;
	}

private:
	char vol[26];
	char cvol;
	int i, j;

	vector<string> ignorepath;
};

bool InitData::isNTFS(char c) {
	char lpRootPathName[] = ("c:\\");
	lpRootPathName[0] = c;
	char lpVolumeNameBuffer[MAX_PATH];
	DWORD lpVolumeSerialNumber;
	DWORD lpMaximumComponentLength;
	DWORD lpFileSystemFlags;
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