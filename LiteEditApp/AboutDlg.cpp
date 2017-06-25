// AboutDlg.cpp : implementation file
//

#include "stdafx.h"
#include "liteedit.h"
#include "AboutDlg.h"
#include "ConfigData.h"
#include "PathMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AboutDlg dialog


AboutDlg::AboutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(AboutDlg::IDD, pParent), mModuleLoader(TEXT("RICHED32.DLL"))
{
	//{{AFX_DATA_INIT(AboutDlg)
	//}}AFX_DATA_INIT
}


void AboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AboutDlg)
	DDX_Control(pDX, IDC_RICHEDIT, mEdit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(AboutDlg, CDialog)
	//{{AFX_MSG_MAP(AboutDlg)
	ON_WM_HELPINFO()
	ON_NOTIFY(EN_LINK, IDC_RICHEDIT, OnLinkRichedit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AboutDlg message handlers

BOOL AboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CString text = ProgramLongName;
	text += TEXT("\r\nOctober 2013\r\n\r\n");
	text += Publisher;
	text += TEXT("\r\n");
	text += PublisherEMail;
	text += TEXT("\r\n");
	text += AppUrl;
	text += TEXT("\r\n\r\n\r\nPrevious Versions:\r\n");
	text += TEXT("-----------------------------\r\n");
	text += TEXT("2.1 [Lithium] - July 2012\r\n");
	text += TEXT("2.0.1 [Red] - December 2010\r\n");
	text += TEXT("2.0 [Red] - October 2006\r\n");
	text += TEXT("1.1 [Green] - October 2004\r\n");
	text += TEXT("1.0 [Blue] - March 2004");

	mEdit.SetWindowText(text);

	LinkifyText(mEdit, PublisherEMail);
	LinkifyText(mEdit, AppUrl);

	DWORD mask = mEdit.GetEventMask();
	mask |= ENM_LINK;
	mEdit.SetEventMask(mask);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL AboutDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return TRUE;
}

void AboutDlg::OnLinkRichedit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	ENLINK* link = (ENLINK*)pNMHDR;

	if (link->msg == WM_LBUTTONDOWN && link->wParam == MK_LBUTTON) {
		CString text;
		TEXTRANGE tr;
		MemClear(tr);
		tr.chrg = link->chrg;
		char buffer[100];
		tr.lpstrText = buffer;
		mEdit.SendMessage(EM_GETTEXTRANGE, 0, (WPARAM)&tr);
		text = buffer;

		//prefix the link with mailto: so ShellExecute() will be able to execute it
		if (text == PublisherEMail) {
			CString temp = TEXT("mailto:") + text;
			text = temp;
		}

		gConfigData.LaunchURL(text, m_hWnd);
	}

	*pResult = 0;
}
