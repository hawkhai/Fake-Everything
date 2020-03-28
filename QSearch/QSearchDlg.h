// QSearchDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CQSearchDlg �Ի���
class CQSearchDlg : public CDialog
{
// ����
public:
	CQSearchDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_QSEARCH_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_str;
public:
	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnDestroy();
	afx_msg void OnClose();

public:
	CListBox m_FileName;


public:
	afx_msg void OnLbnDblclkList1();
public:
	afx_msg void OnUplow();

	// �˵�����
public:
	static UINT initThread(LPVOID pParam);
	UINT realThread(LPVOID pParam);
	int horizontalLen;	// ������
//	CProgressCtrl m_pro;	// ������
	void showProsess(int i);
	CMenu menu;
	bool m_bIsUpLow;
	bool m_bUnOrder;
	int num;
public:
	afx_msg void OnUnorder();
public:
	afx_msg void OnAbout();
public:
	CBitmapButton m_btnBitmap; // ��ť��ͼ
public:
	CWinThread* pThread;
public:
	afx_msg void OnBnClickedFind();
public:
	afx_msg void OnTip();
public:
	CProgressCtrl m_pro;
public:
	afx_msg void OnIgnore();
public:
	NOTIFYICONDATA m_ntIcon;
public:
	BOOL OnInitalIcon(void);


public:
	afx_msg LRESULT OnNotifyIcon(WPARAM, LPARAM); // ��С��ͼ��
public:
	bool m_bMin;
public:
	afx_msg void OnPopupShow();
public:
	afx_msg void OnPopupAbout();
public:
	afx_msg void OnPopupQuit();
};