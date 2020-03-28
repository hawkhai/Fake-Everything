// QSearchDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "QSearch.h"
#include "QSearchDlg.h"
#include "IgnoreDlg.h"
#include "NtfsVolume.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define MY_WM_NOTIFYICON (WM_USER+1001)

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CQSearchDlg �Ի���


extern InitData initdata;

CQSearchDlg::CQSearchDlg(CWnd* pParent /*=NULL*/)
: CDialog(CQSearchDlg::IDD, pParent)
, m_bMin(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CQSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STR, m_str);
	DDX_Control(pDX, IDC_LIST1, m_FileName);
	DDX_Control(pDX, IDC_PROGRESS1, m_pro);
}

BEGIN_MESSAGE_MAP(CQSearchDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CQSearchDlg::OnBnClickedOk)
	ON_LBN_SELCHANGE(IDC_LIST1, &CQSearchDlg::OnLbnSelchangeList1)
	ON_LBN_DBLCLK(IDC_LIST1, &CQSearchDlg::OnLbnDblclkList1)
	ON_COMMAND(ID_UPLOW, &CQSearchDlg::OnUplow)
	ON_COMMAND(ID_UNORDER, &CQSearchDlg::OnUnorder)
	ON_COMMAND(ID_ABOUT, &CQSearchDlg::OnAbout)
	ON_BN_CLICKED(ID_FIND, &CQSearchDlg::OnBnClickedFind)
	ON_COMMAND(ID_TIP, &CQSearchDlg::OnTip)

	//	ON_COMMAND(ID_32783, &CQSearchDlg::OnIgnore)
	ON_COMMAND(ID_IGNORE, &CQSearchDlg::OnIgnore)

	// OnNotifyIcon ������Ϣ������������
	ON_MESSAGE(MY_WM_NOTIFYICON, &CQSearchDlg::OnNotifyIcon)   

	ON_COMMAND(ID_32788, &CQSearchDlg::OnPopupShow)
	ON_COMMAND(ID_32786, &CQSearchDlg::OnPopupAbout)
	ON_COMMAND(ID_32787, &CQSearchDlg::OnPopupQuit)

	// �˳�
	ON_WM_CLOSE() // ����ط�һ��Ҫд

	ON_WM_DESTROY()

END_MESSAGE_MAP()


// CQSearchDlg ��Ϣ�������

BOOL CQSearchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	//m_str.SetWindowText(_T("������Ҫ���ҵ��ļ�����"));

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// ��ʼ��ϵͳ����ͼ��
	OnInitalIcon();

	// ��ť��ͼ
	HICON m_hicn1=AfxGetApp()->LoadIcon(IDI_ICON1);
	CWnd *pWnd = GetDlgItem(ID_FIND);
	CButton *Button= (CButton *) pWnd;
	Button->SetIcon(m_hicn1);

	// ˮƽ������
	::SendMessage(m_FileName, LB_SETHORIZONTALEXTENT, 2000, 0);

	// �˵�
	m_bUnOrder = true;
	m_bIsUpLow = false;
	menu.LoadMenu(IDR_MENU);
	SetMenu(&menu);

	// �߳�
	initdata.init();
	num = initdata.getNum();
	m_pro.SetRange(0,num*100);
	pThread = AfxBeginThread(initThread, (LPVOID)this);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

UINT CQSearchDlg::initThread(LPVOID pParam) {
	CQSearchDlg * pObj = (CQSearchDlg*)pParam;
	if ( pObj ) {
		return pObj->realThread(NULL);
	}
	return false;
}

UINT CQSearchDlg::realThread(LPVOID pParam) {

	char *pvol = initdata.getVol();
	for ( int i=0; i<num; ++i ) {
		initdata.initvolumelist(pvol[i]);
		CString showpro(_T("����ͳ��"));
		showpro += pvol[i];
		showpro += _T("���ļ�...");
		GetDlgItem(IDC_SHOWPRO)->SetWindowText(showpro);
		//m_pro.SetPos(i+1);
		showProsess((i+1)*100);
	}

	GetDlgItem(IDC_SHOWPRO)->SetWindowText(_T("ͳ�����^_^"));
	m_pro.ShowWindow(SW_HIDE);

	return 0;
}

void CQSearchDlg::showProsess(int end) {
	for ( int i=0; i <= end; i+=5 ) {
		m_pro.SetPos(i);
	}
}

void CQSearchDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CQSearchDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR CQSearchDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CQSearchDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	// OnOK();

}

void CQSearchDlg::OnLbnSelchangeList1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}

void CQSearchDlg::OnLbnDblclkList1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString CSpath;
	m_FileName.GetText(m_FileName.GetCurSel(), CSpath);
	CStringA path(CW2A( (LPCTSTR)CSpath) , CP_UTF8);
	ShellExecute(NULL,   NULL, CSpath,   NULL,   NULL,   SW_SHOWNORMAL);
}

void CQSearchDlg::OnUplow()
{
	// TODO: �ڴ���������������
	CMenu* pUplowMenu = menu.GetSubMenu(0);
	if ( m_bIsUpLow ) {
		pUplowMenu->CheckMenuItem(ID_UPLOW, MF_BYCOMMAND|MF_UNCHECKED );
	} else {
		pUplowMenu->CheckMenuItem(ID_UPLOW, MF_BYCOMMAND|MF_CHECKED );
	}
	m_bIsUpLow = !m_bIsUpLow;
}

void CQSearchDlg::OnUnorder()
{
	// TODO: �ڴ���������������
	CMenu* pUnorder = menu.GetSubMenu(0);
	if ( m_bUnOrder ) {
		pUnorder->CheckMenuItem(ID_UNORDER, MF_BYCOMMAND|MF_UNCHECKED );
	} else {
		pUnorder->CheckMenuItem(ID_UNORDER, MF_BYCOMMAND|MF_CHECKED );
	}
	m_bUnOrder = !m_bUnOrder;
}

void CQSearchDlg::OnAbout()
{
	// TODO: �ڴ���������������
	CAboutDlg m_AboutDlg;
	m_AboutDlg.DoModal();
}

void CQSearchDlg::OnBnClickedFind()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//	WaitForSingleObject(pThread, INFINITE);
	//	delete pThread;

	// ���֮ǰ�Ľ��
	m_FileName.ResetContent();

	CString lpszStringBuf;
	// �õ�������ļ�����
	m_str.GetWindowText(lpszStringBuf);
	lpszStringBuf.TrimLeft();	// �Է�ȫ���ǿո�
	lpszStringBuf.TrimRight();
	if ( lpszStringBuf.GetLength() == 0 ) {
		return;
	} else if ( lpszStringBuf.GetLength() < 2 ) {
		MessageBox(_T("��Ч�ַ�С��2,���ܻ�����ʮW���ļ������������룺^_^"));
		return;
	}

	// CString -> string
	// string str(CW2A( (LPCTSTR)lpszStringBuf) );
	CStringA strbuf = CW2A(CString(lpszStringBuf), CP_UTF8);


	// TODO: �ڴ���� 
	// ����c d e��
	StringCompare cmpstrstr(m_bIsUpLow, m_bUnOrder);
	std::vector<CStringA>& pignorepath = initdata.vectorIgnorePath();

	for ( std::list<NtfsVolume*>::iterator lvolit = initdata.listVolume().begin();
		lvolit != initdata.listVolume().end(); ++lvolit ) {
			// c d e volumelist
			std::vector<CStringA> rightFile = (*lvolit)->findFile(strbuf, cmpstrstr, &pignorepath);

			// ��ListBox����ʾ
			for (std::vector<CStringA>::iterator vstrit = rightFile.begin();
				vstrit != rightFile.end(); ++vstrit) {
					CStringA temp = *vstrit;
					CString tempk = CA2W(temp, CP_UTF8);
					m_FileName.AddString( tempk );
			}
	}
}

void CQSearchDlg::OnTip()
{
	// TODO: �ڴ���������������
	MessageBox(_T("�����̷���ʾ�����������ĵȴ�5s���ٴ���������^_^"));
}

void CQSearchDlg::OnIgnore()
{
	// TODO: �ڴ���������������
	CIgnoreDlg  Dlg;
	Dlg.DoModal();
	OnBnClickedFind();
}

BOOL CQSearchDlg::OnInitalIcon(void)
{
	m_bMin = false;

	m_ntIcon.cbSize = sizeof(NOTIFYICONDATA); // �ýṹ������Ĵ�С
	m_ntIcon.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME); // ͼ�꣬ͨ����ԴID�õ�
	m_ntIcon.hWnd = this->m_hWnd;	 // ��������ͼ��֪ͨ��Ϣ�Ĵ��ھ��
	CString atip = L"QSearch";
	wcscpy_s(m_ntIcon.szTip,128, atip.GetString());	// ���������ʱ��ʾ����ʾ
	m_ntIcon.uCallbackMessage = MY_WM_NOTIFYICON; // Ӧ�ó��������ϢID��
	m_ntIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;// ͼ������ԣ����ó�ԱuCallbackMessage��hIcon��szTip��Ч
	::Shell_NotifyIcon(NIM_ADD, &m_ntIcon); // ��ϵͳ֪ͨ�����������ͼ��

	return TRUE;
}

LRESULT CQSearchDlg::OnNotifyIcon(WPARAM wparam, LPARAM lparam)
{
	if(lparam == WM_LBUTTONDOWN) {
		//������Ӷ�����������Ĵ������崦�����4��
		if(m_bMin == true) {
			AfxGetMainWnd()->ShowWindow(SW_SHOW);
			AfxGetMainWnd()->ShowWindow(SW_RESTORE);
			//����ò��ֻ��д����������ܱ�֤�ָ����ں󣬸ô��ڴ��ڻ״̬������ǰ�棩
			m_bMin = false;
		} else {
			AfxGetMainWnd()->ShowWindow(SW_MINIMIZE);
			m_bMin = true;
		}  
	} else if(lparam == WM_RBUTTONDOWN)	{
		//������Ӷ�����Ҽ�����Ĵ������崦�����5��
		//��������˵�
		CMenu popMenu;

		//IDR_MENU_POPUP����ResourceView�д������༭��һ���˵�
		popMenu.LoadMenu(IDR_MENU_POPUP); 

		//�����Ĳ˵�ʵ������IDR_MENU_POPUP�˵���ĳ����Ӳ˵��������ǵ�һ��
		CMenu* pmenu = popMenu.GetSubMenu(0);

		CPoint pos;
		GetCursorPos(&pos);            //�����˵���λ�ã�����������ĵ�ǰλ��
		//��ʾ�ò˵�����һ������������ֵ�ֱ��ʾ�������ұ���ʾ����Ӧ����һ�
		pmenu->TrackPopupMenu(TPM_RIGHTALIGN|TPM_RIGHTBUTTON, pos.x, pos.y, AfxGetMainWnd(), 0);  
	}
	return 0;
}

void CQSearchDlg::OnDestroy() 
{
	// ɾ����ͼ��
	::Shell_NotifyIcon(NIM_DELETE, &m_ntIcon);
}
void CQSearchDlg::OnPopupShow()
{
	// TODO: �ڴ���������������
	AfxGetMainWnd()->ShowWindow(SW_SHOWNORMAL);
}

void CQSearchDlg::OnPopupAbout()
{
	// TODO: �ڴ���������������
	CAboutDlg abdlg;
	abdlg.DoModal();
}

void CQSearchDlg::OnPopupQuit()
{
	// TODO: �ڴ���������������
	// ����һ��WM_CLOSE��Ϣ���رմ���
	// SendMessage(WM_CLOSE);
	this->DestroyWindow();
}

void CQSearchDlg::OnClose() {
	//this->DestroyWindow();

	AfxGetMainWnd()->ShowWindow(SW_HIDE);
	m_bMin = true;
}
