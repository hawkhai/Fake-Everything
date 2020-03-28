#pragma once

#include <vector>
#include <map>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <Winioctl.h>

#define ROOT_FILE_REF_NUM 0x5000000000005
#define MAX_DISK_COUNT 26

// 和操作系统保持一致
//#define FILE_ATTRIBUTE_READONLY      0x00000001  
//#define FILE_ATTRIBUTE_HIDDEN        0x00000002  
//#define FILE_ATTRIBUTE_SYSTEM        0x00000004  
//#define FILE_ATTRIBUTE_DIRECTORY     0x00000010  
//#define FILE_ATTRIBUTE_ARCHIVE       0x00000020  

// 和操作系统保持一致
#define FILE_NODE_READONLY             0x00000001  
#define FILE_NODE_HIDDEN               0x00000002  
#define FILE_NODE_SYSTEM               0x00000004  
#define FILE_NODE_DIRECTORY            0x00000010  
#define FILE_NODE_ARCHIVE              0x00000020  

typedef struct _FileNode
{
	CStringA filename;
	DWORDLONG frn;
	DWORDLONG pfrn;
	UINT8 fileAttributes; // DWORD

public:
	INT8 depth;
	_FileNode* pfrnAddress;

public:
	_FileNode()
	{
		pfrnAddress = NULL;
		frn = pfrn = 0;
		fileAttributes = 0;
		depth = -1;
	}
} KFileNode;

typedef std::map<DWORDLONG, KFileNode*> KFileNodeMap;
typedef std::vector<KFileNode*> KFileNodeVector;

class StringCompare {
public:
	StringCompare(BOOL uplow, BOOL inorder) {
		this->uplow = uplow;
		this->inorder = inorder;
	}
	
	~StringCompare() {};
	BOOL compareStrFilename(CStringA str, CStringA filename);
	BOOL infilename(CStringA &strtmp, CStringA &filenametmp);

private:
	BOOL uplow;
	BOOL inorder;
};

class NtfsVolume {

public:

	NtfsVolume(char vol)
	{
		m_chVol = vol;
		m_hVol = NULL;
		ZeroMemory(&m_UsnJournalData, sizeof(m_UsnJournalData));

		KFileNode* disk = new KFileNode();

		// 根目录
		CStringA tmp("C:");
		tmp.SetAt(0, m_chVol);
		disk->filename = tmp;
		disk->pfrn = 0;
		disk->frn = ROOT_FILE_REF_NUM;
		disk->fileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		disk->depth = 0;
		m_vecFileNode.push_back(disk);

		AdjustPrivileges(SE_MANAGE_VOLUME_NAME);
	}

	virtual ~NtfsVolume()
	{
		if (m_hVol)
		{
			CloseHandle(m_hVol);
			m_hVol = NULL;
		}
		KFileNodeVector::iterator it = m_vecFileNode.begin();
		while (it != m_vecFileNode.end())
		{
			KFileNode* temp = *it;
			delete temp;
			++it;
		}
		m_vecFileNode.clear();
	}

	BOOL getHandle();
	BOOL createUSN();
	BOOL getUSNInfo();
	BOOL getUSNJournal();
	BOOL deleteUSN();
	int AdjustPrivileges(TCHAR* lpszPrivilege);
	static int vecCompare( const void * v1, const void * v2 );

	BOOL initVolume()
	{
		// 2.获取驱动盘句柄
		if (!getHandle()) return false;

		// 3.创建USN日志
		if (!createUSN()) return false;

		// 4.获取USN日志信息
		if (!getUSNInfo()) return false;

		// 5.获取 USN Journal 文件的基本信息
		if (!getUSNJournal()) return false;

		// 06. 删除 USN 日志文件 (也可以不删除) 
		if (!deleteUSN()) return false;

		return TRUE;
	}

	BOOL isIgnore( std::vector<CStringA>* pignorelist, CStringA& path)
	{
		if (pignorelist == NULL)
		{
			return false;
		}
		//CStringA tmp = CW2A(path.GetString(), CP_UTF8); // CW2A
		for ( std::vector<CStringA>::iterator it = pignorelist->begin(); it != pignorelist->end(); ++it )
		{
			if ( !path.Compare(*it) )
			{
				return true;
			}
		}
		return false;
	}

	std::vector<CStringA> findFile( CStringA str, StringCompare& cmpstrstr, std::vector<CStringA>* pignorelist)
	{
		std::vector<CStringA> rightFile;	// 结果

		for (KFileNodeVector::const_iterator cit = m_vecFileNode.begin(); cit != m_vecFileNode.end(); ++cit)
		{
			if ( cmpstrstr.compareStrFilename(str, (*cit)->filename) )
			{
				CStringA path;
				getParentPath(*cit, path);
				path += (*cit)->filename;

				if ( isIgnore(pignorelist, path) ) {
					continue;
				}	
				rightFile.push_back(path);
			}
		}

		return rightFile;
	}

	KFileNode* getParentNode(KFileNode* pnode)
	{
		KFileNode parent;
		if (pnode == NULL || pnode->pfrn == 0)
		{
			return NULL;
		}
		if (pnode->pfrnAddress != NULL)
		{
			KFileNode* pParent = pnode->pfrnAddress;
			return pParent;
		}

		parent.frn = pnode->pfrn;
		KFileNode* pParent = &parent;

		KFileNode** result = (KFileNode**)::bsearch(&pParent, &m_vecFileNode[0], m_vecFileNode.size(), sizeof(m_vecFileNode[0]), vecCompare);
		if (result == NULL || *result == NULL)
		{
			return NULL;
		}

		pParent = *result;
		pnode->pfrnAddress = pParent;
		return pParent;
	}
	
	CStringA& getParentPath(KFileNode* pnode, CStringA &path)
	{
		if (pnode == NULL)
		{
			return path;
		}
		KFileNode* parent = getParentNode(pnode);
		
		if (parent != NULL)
		{
			if ( 0 != parent->pfrn )
			{
				getParentPath(parent, path);
			}

			if (parent->depth >= 0)
			{
				pnode->depth = parent->depth + 1;
			}

			path += parent->filename;
			path.AppendFormat("[%d]", parent->depth);
			path += "\\";
		}
		return path;
	}

private:
	char m_chVol;
	HANDLE m_hVol;
	KFileNodeVector m_vecFileNode; // 查找1
	// KFileNodeMap m_mapFileNode; // 查找2
	USN_JOURNAL_DATA m_UsnJournalData;
};
	

/*
 * 计算一共有几个盘符是NTFS格式
 */
class InitData {
public:	
	InitData()
	{
		m_nCount = 0;
		ZeroMemory(m_chDiskArray, MAX_DISK_COUNT);
		m_bInited = FALSE;
	}

	virtual ~InitData()
	{
		std::list<NtfsVolume*>::iterator it = m_listVolume.begin();
		while (it != m_listVolume.end())
		{
			NtfsVolume* temp = *it;
			delete temp;
			++it;
		}
		m_listVolume.clear();
	}

	BOOL isNTFS(char c);

	BOOL initvolumelist(char vol)
	{
		NtfsVolume* volume = new NtfsVolume(vol);
		volume->initVolume();
		m_listVolume.push_back(volume);
		return TRUE;
	}

	BOOL init()
	{

		if (m_bInited)
		{
			return TRUE;
		}
		
		std::ifstream fin("config.ini");
		std::string temp;
		while (getline(fin, temp))
		{
			if (temp.empty())
			{
				continue;
			}
			CStringA ctemp = temp.c_str();
			m_vecIgnorePath.push_back(ctemp);
		}

		m_chSystemDrive = 'C';
		char szSystemDrive[MAX_PATH] = {0};
		if (::GetSystemDirectoryA(szSystemDrive, MAX_PATH))
		{
			m_chSystemDrive = szSystemDrive[0];
		}
		else
		{
			return FALSE;
		}

		m_nCount = 0;
		for (int i = 0; i < MAX_DISK_COUNT; ++i)
		{
#ifdef TEST
			char cvol = i + 'F';
			if (isNTFS(cvol))
			{
				m_chDiskArray[m_nCount++] = cvol;
				break;
			}
#else
			char cvol = i + 'A';
			if (isNTFS(cvol))
			{
				m_chDiskArray[m_nCount++] = cvol;
				break;
			}
#endif
		}
		m_bInited = TRUE;
		return true;
	}

	int getNum()
	{
		return m_nCount;
	}
	char* getVol()
	{
		return m_chDiskArray;
	}

	std::vector<CStringA>& vectorIgnorePath()
	{
		return m_vecIgnorePath;
	}

	std::list<NtfsVolume*>& listVolume()
	{
		return m_listVolume;
	}

private:
	BOOL m_bInited;
	int m_nCount;
	char m_chSystemDrive;
	char m_chDiskArray[MAX_DISK_COUNT];
	std::list<NtfsVolume*> m_listVolume;
	std::vector<CStringA> m_vecIgnorePath;
};
