// GotoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GotoDlg.h"
#include "Utility.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGotoDlg dialog


CGotoDlg::CGotoDlg(long lineCount, CWnd* pParent /*=NULL*/)
	: CDialog(CGotoDlg::IDD, pParent), mLineCount(lineCount)
{
	//{{AFX_DATA_INIT(CGotoDlg)
	//}}AFX_DATA_INIT
}

void CGotoDlg::UpdateEnable() {
	CString text;
	mEdit.GetWindowText(text);
	mOkBtn.EnableWindow(!text.IsEmpty());
}

void CGotoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGotoDlg)
	DDX_Control(pDX, IDC_STATIC_LINE_NUMBER, mLineNumberLabel);
	DDX_Control(pDX, IDOK, mOkBtn);
	DDX_Control(pDX, IDC_LINE_EDIT, mEdit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGotoDlg, CDialog)
	//{{AFX_MSG_MAP(CGotoDlg)
	ON_EN_CHANGE(IDC_LINE_EDIT, OnChangeLineEdit)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGotoDlg message handlers

void CGotoDlg::OnChangeLineEdit()
{
	UpdateEnable();
}

BOOL CGotoDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CString label;
	label.Format(TEXT("Line &number (1 - %d):"), mLineCount);
	mLineNumberLabel.SetWindowText(label);

	mLineNumber = -1;
	UpdateEnable();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGotoDlg::OnOK() 
{
	CString text;
	mEdit.GetWindowText(text);
	mLineNumber = _tstol(text);

	if (mLineNumber == 0 && text != _T("0"))
		MessageBox(_T("'") + text + _T("' is not a valid line number."), ErrorStr, MB_OK | MB_ICONERROR);
	else
		CDialog::OnOK();
}

BOOL CGotoDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return TRUE;
}
