// IgnoreDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "QSearch.h"
#include "IgnoreDlg.h"

#include <fstream>
#include <string>


// CIgnoreDlg �Ի���

IMPLEMENT_DYNAMIC(CIgnoreDlg, CDialog)

CIgnoreDlg::CIgnoreDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIgnoreDlg::IDD, pParent)
{
	
}

CIgnoreDlg::~CIgnoreDlg()
{

}

BOOL CIgnoreDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{

	}

	ReadConfig();
	// ˮƽ������
	::SendMessage(m_IgnoreList, LB_SETHORIZONTALEXTENT, 2000, 0);


	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CIgnoreDlg::DoDataExchange(CDataExchange* pDX)
{
//	ReadConfig();
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IGN_LIST, m_IgnoreList);
}


BEGIN_MESSAGE_MAP(CIgnoreDlg, CDialog)
	ON_BN_CLICKED(IDC_ADD, &CIgnoreDlg::OnBnClickedAdd)
	ON_BN_CLICKED(IDC_DEL, &CIgnoreDlg::OnBnClickedDel)
	ON_BN_CLICKED(IDC_CLEAN, &CIgnoreDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_OK, &CIgnoreDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CIgnoreDlg ��Ϣ�������

void CIgnoreDlg::OnBnClickedAdd()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������


	BROWSEINFO   bInfo; 
	ZeroMemory(&bInfo, sizeof(bInfo)); 
//	bInfo.hwndOwner   =   m_hWnd; 
	TCHAR   tchPath[255]; 
	tchPath[0] = '\0';
	bInfo.lpszTitle   =   _T( "��ѡ��·��:   "); 
	bInfo.ulFlags   =   BIF_RETURNONLYFSDIRS;         

	LPITEMIDLIST   lpDlist; 
	//�������淵����Ϣ��IDList��ʹ��SHGetPathFromIDList����ת��Ϊ�ַ��� 
	lpDlist   =   SHBrowseForFolder(&bInfo)   ;   //��ʾѡ��Ի��� 
	if( lpDlist != NULL ) { 
		SHGetPathFromIDList(lpDlist,   tchPath);//����Ŀ��ʶ�б�ת����Ŀ¼ 
//		TRACE(tchPath); 
	}
	
	if ( tchPath[0] != '\0' ) {
		m_IgnoreList.AddString(tchPath);
	}
}
void CIgnoreDlg::OnBnClickedDel()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	
	m_IgnoreList.DeleteString(m_IgnoreList.GetCurSel());
}

void CIgnoreDlg::OnBnClickedButton2()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_IgnoreList.ResetContent();
}


bool CIgnoreDlg::WriteConfig(void)
{
	// д�������ļ� config.ini
	using namespace std;
	ofstream fout("config.ini");

	CString Ctmp;
	int listcount = m_IgnoreList.GetCount();
	for ( int i=0; i<listcount; ++i ) {
		m_IgnoreList.GetText(i, Ctmp);
		string path(CW2A( (LPCTSTR)Ctmp), CP_UTF8 );
		fout << path << "\n";
	}	

	fout.close();

	return true;
}



void CIgnoreDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	WriteConfig();
	OnOK();
}

bool CIgnoreDlg::ReadConfig(void)
{
	using namespace std;
	ifstream fin("config.ini", ios::in);
	
	string tmp;
	while ( getline(fin, tmp) ) {
		m_IgnoreList.AddString(CA2W(tmp.c_str(), CP_UTF8) );
	}
	
	fin.close();

	return true;
}
