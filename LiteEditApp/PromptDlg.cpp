// PromptDlg.cpp : implementation file
//

#include "stdafx.h"
#include "liteedit.h"
#include "PromptDlg.h"
#include "Utility.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PromptDlg dialog


PromptDlg::PromptDlg(const CString& promptStr, const CString& title = EmptyStr, CWnd* pParent /*=NULL*/)
	: CDialog(PromptDlg::IDD, pParent), mPromptStr(promptStr), mTitle(title)
{
	//{{AFX_DATA_INIT(PromptDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void PromptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PromptDlg)
	DDX_Control(pDX, IDOK, mOk);
	DDX_Control(pDX, IDC_STATIC_PROMPT, mStaticPrompt);
	DDX_Control(pDX, IDC_EDIT, mEdit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PromptDlg, CDialog)
	//{{AFX_MSG_MAP(PromptDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PromptDlg message handlers

BOOL PromptDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	//Use hueristic to add trailing colon if the user forgot it.
	//It debateably may have been better not to do this to give the user
	//complete control over this, but I can't just remove it now because
	//users could be relying on it.
	//I could remove this hueristic if I added a new file persistence
	//version and added the trailing colon to prompt messages of tools when
	//opening old config files but it's probably not worth the trouble.
	if (!mPromptStr.IsEmpty() && _istalnum(mPromptStr.GetAt(mPromptStr.GetLength() - 1)))
		mPromptStr += ':';

	mStaticPrompt.SetWindowText(mPromptStr);

	if (!mTitle.IsEmpty())
		SetWindowText(mTitle);

	// dynamically size dialog based on text label size
	CRect rc;
	mStaticPrompt.GetWindowRect(&rc);

	const long MinWidth = rc.Width();
	long reasonableWidth = Div(GetMonitorRect(m_hWnd).Width() * 5L, 10L, rkOff);
	const long MaxWidth = Max(reasonableWidth, MinWidth);

	CDC* dc = mStaticPrompt.GetDC();
	ASSERT(dc != NULL);

	if (dc != NULL) {
		CFont* oldFont = dc->SelectObject(mStaticPrompt.GetFont());

		UINT format = DT_CALCRECT | DT_EXPANDTABS | DT_LEFT;
		CRect rcCalc(0, 0, 0, 0);
		VERIFY(dc->DrawText(mPromptStr, &rcCalc, format));

		long width = Max(MinWidth, Min(MaxWidth, (long)rcCalc.Width()));

		rcCalc.SetRect(0, 0, width, 0);
		VERIFY(dc->DrawText(mPromptStr, &rcCalc, format | DT_WORDBREAK));

		dc->SelectObject(oldFont);
		VERIFY(mStaticPrompt.ReleaseDC(dc));

		long growX = Max(width - rc.Width(), 0);
		long growY = Max(rcCalc.Height() - rc.Height(), 0);

		mStaticPrompt.GetWindowRect(&rc);
		rc.InflateRect(0, 0, growX, growY);
		ScreenToClient(&rc);
		mStaticPrompt.MoveWindow(&rc);

		mEdit.GetWindowRect(&rc);
		rc.OffsetRect(0, growY);
		rc.InflateRect(0, 0, growX, 0);
		ScreenToClient(&rc);
		mEdit.MoveWindow(&rc);

		mOk.GetWindowRect(&rc);
		rc.OffsetRect(growX, growY);
		ScreenToClient(&rc);
		mOk.MoveWindow(&rc);

		// windows seems to center the dialog as long as I don't set top and left
		GetWindowRect(&rc);
		rc.InflateRect(0, 0, growX, growY);
		MoveWindow(&rc);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PromptDlg::OnOK() 
{
	mEdit.GetWindowText(mText);
		
	CDialog::OnOK();
}
