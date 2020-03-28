#pragma once

#include <vector>
#include <map>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <Winioctl.h>

#define ROOT_FILE_PREF_NUM  0
#define ROOT_FILE_REF_NUM	0x5000000000005
#define MAX_DISK_COUNT		26
#define MEMO_CHAR_SIZE_64K	0x10000

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
	//CStringA filename;
	DWORDLONG frn;
	DWORDLONG pfrn;
	UINT8 fileAttributes; // DWORD

public:
	//INT8 depth;
	//_FileNode* pfrnAddress;
	UINT8 fileNameLength;
	CHAR fileName[1];

public:
	static _FileNode* createFileNodeNull()
	{
		return new _FileNode();
	}

private:
	_FileNode()
	{
		//pfrnAddress = NULL;
		frn = pfrn = 0;
		fileAttributes = 0;
		fileNameLength = 0;
		fileName[0] = 0;
		//depth = -1;
	}
} KFileNode;

typedef struct _PageString
{
	CHAR* szBuffer;
	int nPos;
	int nEnd;

public:
	_PageString()
	{
		szBuffer = new CHAR[MEMO_CHAR_SIZE_64K];
		nPos = 0;
		nEnd = MEMO_CHAR_SIZE_64K;
	}

	virtual ~_PageString()
	{
		if (szBuffer)
		{
			delete[] szBuffer;
			szBuffer = NULL;
		}
	}

	KFileNode* locateFileNode(const CHAR* name)
	{
		if (name == NULL)
		{
			name = "";
		}

		UINT8 namelen = strlen(name);
		UINT8 length = sizeof(KFileNode) + namelen;
		if (nPos + length < nEnd)
		{
			KFileNode* retv = (KFileNode*)(&szBuffer[nPos]);
			retv->fileNameLength = namelen;
			strcpy(retv->fileName, name);
			nPos += length;
			return retv;
		}
		else
		{
			return NULL;
		}
	}

} KPageString;

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

struct NtfsFile
{
	KFileNode* m_fileNode;
	CStringA m_filePath; // utf8 编码的文件夹路径

	NtfsFile(CStringA filePath, KFileNode* fileNode)
	{
		this->m_filePath = filePath;
		this->m_fileNode = fileNode;
	}

	CString getFilePath() 
	{
		return CString(CA2W(m_filePath.GetString(), CP_UTF8));
	}
};

typedef std::vector<KFileNode*> KFileNodeVector;
typedef std::vector<KPageString*> KPageStringVector;

class NtfsVolume {

public:

	NtfsVolume(char vol)
	{
		m_chVol = vol;
		m_hVol = NULL;
		ZeroMemory(&m_UsnJournalData, sizeof(m_UsnJournalData));

		m_curPageString = NULL;

		// 根目录
		CStringA tmp("C:");
		tmp.SetAt(0, m_chVol);

		m_fileNodeDisk = createFileNode(tmp.GetString());
		if (m_fileNodeDisk) // 内存足够
		{
			m_fileNodeDisk->pfrn = ROOT_FILE_PREF_NUM;
			m_fileNodeDisk->frn = ROOT_FILE_REF_NUM;
			m_fileNodeDisk->fileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			//disk->depth = 0;
			m_vecFRNIndex.push_back(m_fileNodeDisk);
			m_vecParentFRNIndex.push_back(m_fileNodeDisk);
		}

		AdjustPrivileges(SE_MANAGE_VOLUME_NAME);
	}

	KFileNode* createFileNode(const CHAR* name)
	{
		if (m_curPageString == NULL)
		{
			m_curPageString = new KPageString();
			m_vecPageString.push_back(m_curPageString);
		}
		KFileNode* retv = m_curPageString->locateFileNode(name);
		if (retv != NULL)
		{
			return retv;
		}
		m_curPageString = new KPageString();
		m_vecPageString.push_back(m_curPageString);
		retv = m_curPageString->locateFileNode(name);
		return retv;
	}

	virtual ~NtfsVolume()
	{
		if (m_hVol)
		{
			CloseHandle(m_hVol);
			m_hVol = NULL;
		}
		KPageStringVector::iterator it = m_vecPageString.begin();
		while (it != m_vecPageString.end())
		{
			KPageString* temp = *it;
			delete temp;
			++it;
		}
		m_vecPageString.clear();
		m_vecFRNIndex.clear();
		m_vecParentFRNIndex.clear();
		m_curPageString = NULL;
	}

	BOOL getHandle();
	BOOL createUSN();
	BOOL getUSNInfo();
	BOOL getUSNJournal();
	BOOL deleteUSN();
	int AdjustPrivileges(TCHAR* lpszPrivilege);
	static int vecCompareFrn( const void * v1, const void * v2 );
	static int vecCompareParentFrn( const void * v1, const void * v2 );

	BOOL initVolume()
	{
		if (m_fileNodeDisk == NULL)
		{
			return false;
		}

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

		for (KFileNodeVector::const_iterator cit = m_vecFRNIndex.begin(); cit != m_vecFRNIndex.end(); ++cit)
		{
			if ( cmpstrstr.compareStrFilename(str, (*cit)->fileName) )
			{
				CStringA path;
				getParentPath(*cit, path);
				path += (*cit)->fileName;

				if ( isIgnore(pignorelist, path) ) {
					continue;
				}	
				rightFile.push_back(path);
			}
		}

#ifdef TEST
		{
			std::vector<CStringA> flist;
			CStringA path;
			deepSearch(flist, path, m_fileNodeDisk);

			_wsetlocale(LC_ALL,L"chs"); // 设置成中文
			FILE *fp = NULL;
			static BOOL first = TRUE;
			fp = fopen("D:\\volumezlist.txt", first ? "w" : "a");
			first = FALSE;
			if (fp)
			{
				for (int i = 0; i < flist.size(); i++)
				{
					fprintf(fp, flist[i].GetString());
					fprintf(fp, "\n");
				}
				fclose(fp);
			}
		}
#endif

		return rightFile;
	}

	NtfsFile getDiskRoot()
	{
		CStringA path;
		path.Append(m_fileNodeDisk->fileName);
		return NtfsFile(path, m_fileNodeDisk);
	}

	BOOL getNtfsFile(CStringA path, NtfsFile& ntfsFile)
	{
		return FALSE;
	}

	std::vector<NtfsFile> getChildFiles(NtfsFile ntfsFile)
	{
		std::vector<NtfsFile> retv;
		size_t leftIndex = 0;
		size_t rightIndex = 0;
		if (getChildNodes(ntfsFile.m_fileNode, leftIndex, rightIndex)) // 存在子文件夹
		{
			for (int i = leftIndex; i <= rightIndex; i++)
			{
				KFileNode* current = m_vecParentFRNIndex[i];
				CStringA temp = ntfsFile.m_filePath;
				temp.Append("\\");
				temp.Append(current->fileName);
				retv.push_back(NtfsFile(temp, current));
			}
		}
		return retv;
	}

	void deepSearch(std::vector<CStringA>& flist, CStringA path, KFileNode* root)
	{
		if (root == NULL)
		{
			return;
		}
		path.Append(root->fileName);
		if (root->fileAttributes & FILE_NODE_ARCHIVE)
		{
			flist.push_back(path);
			return;
		}

		path.Append("\\");
		size_t leftIndex = 0;
		size_t rightIndex = 0;
		if (getChildNodes(root, leftIndex, rightIndex)) // 存在子文件夹
		{
			for (int i = leftIndex; i <= rightIndex; i++)
			{
				KFileNode* current = m_vecParentFRNIndex[i];
				deepSearch(flist, path, current);
			}
		}
	}

	BOOL getChildNodes(KFileNode* pnode, size_t& leftIndex, size_t& rightIndex)
	{
		return getChildNodes(pnode, 0, m_vecParentFRNIndex.size() - 1, leftIndex, rightIndex);
	}

	BOOL getChildNodes(KFileNode* pnode, size_t indexL, size_t indexR, size_t& leftIndex, size_t& rightIndex)
	{
		if (pnode == NULL || indexL > indexR)
		{
			return FALSE;
		}

		int vecsize = m_vecParentFRNIndex.size();
		if (indexL >= 0 && indexL < vecsize && indexR >= 0 && indexR < vecsize)
		{
			// yes
		}
		else
		{
			return FALSE;
		}

		KFileNode* child = KFileNode::createFileNodeNull();
		child->pfrn = pnode->frn;
		
		KFileNode** result = (KFileNode**)::bsearch(
			&child,
			&m_vecParentFRNIndex[indexL],
			indexR - indexL + 1,
			sizeof(m_vecParentFRNIndex[0]),
			vecCompareParentFrn);
		delete child;

		if (result == NULL)
		{
			return FALSE;
		}

		int index = ((DWORD)result - (DWORD)&m_vecParentFRNIndex[0]) / sizeof(m_vecParentFRNIndex[0]);
		{ // 递归左边
			size_t LL = 0;
			size_t LR = 0;
			if (getChildNodes(pnode, indexL, index - 1, LL, LR))
			{
				leftIndex = LL;
			}
			else
			{
				leftIndex = index;
			}
		}
		{ // 递归右边
			size_t RL = 0;
			size_t RR = 0;
			if (getChildNodes(pnode, index + 1, indexR, RL, RR))
			{
				rightIndex = RR;
			}
			else
			{
				rightIndex = index;
			}
		}
		return TRUE;
	}

	KFileNode* getParentNode(KFileNode* pnode)
	{
		if (pnode == NULL || pnode->pfrn == 0)
		{
			return NULL;
		}
		//if (pnode->pfrnAddress != NULL)
		{
		//	KFileNode* pParent = pnode->pfrnAddress;
		//	return pParent;
		}

		KFileNode* parent = KFileNode::createFileNodeNull();
		parent->frn = pnode->pfrn;

		KFileNode** result = (KFileNode**)::bsearch(
			&parent,
			&m_vecFRNIndex[0], 
			m_vecFRNIndex.size(), 
			sizeof(m_vecFRNIndex[0]), 
			vecCompareFrn);
		delete parent;

		if (result == NULL)
		{
			return NULL;
		}

		//parent = *result;
		//pnode->pfrnAddress = pParent;
		return *result;
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

			//if (parent->depth >= 0)
			{
				//pnode->depth = parent->depth + 1;
			}

			path += parent->fileName;
			//path.AppendFormat("[%d]", parent->depth);
			path += "\\";
		}
		return path;
	}

private:
	char m_chVol;
	HANDLE m_hVol;
	KFileNode* m_fileNodeDisk;
	KFileNodeVector m_vecFRNIndex; // 查找1
	KFileNodeVector m_vecParentFRNIndex; // 查找2
	KPageStringVector m_vecPageString;
	KPageString* m_curPageString;
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
