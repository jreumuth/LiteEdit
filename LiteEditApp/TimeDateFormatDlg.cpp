// TimeDateFormatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "liteedit.h"
#include "TimeDateFormatDlg.h"
#include "ConfigData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TimeDateFormatDlg dialog

TimeDateFormatDlg::TimeDateFormatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(TimeDateFormatDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(TimeDateFormatDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void TimeDateFormatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TimeDateFormatDlg)
	DDX_Control(pDX, IDC_MACRO_BUTTON, mButtonMacro);
	DDX_Control(pDX, IDC_EDIT_TIME_DATE, mEditTimeDate);
	DDX_Control(pDX, IDC_EDIT_TIME_DATE_FORMAT, mEditTimeDateFormat);
	//}}AFX_DATA_MAP
}

// to keep mEditTimeDate at the same scroll position as mEditTimeDateFormat
// when mEditTimeDateFormat has multiple lines of text
void TimeDateFormatDlg::ScrollTimeDate() {
	int fvl = mEditTimeDateFormat.GetFirstVisibleLine();
	fvl -= mEditTimeDate.GetFirstVisibleLine();
	mEditTimeDate.LineScroll(fvl);
}

BEGIN_MESSAGE_MAP(TimeDateFormatDlg, CDialog)
	//{{AFX_MSG_MAP(TimeDateFormatDlg)
	ON_EN_CHANGE(IDC_EDIT_TIME_DATE_FORMAT, OnChangeEditTimeDateFormat)
	ON_EN_VSCROLL(IDC_EDIT_TIME_DATE_FORMAT, OnVscrollEditTimeDateFormat)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_MACRO_BUTTON, OnMacroButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TimeDateFormatDlg message handlers

BOOL TimeDateFormatDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	mButtonMacro.SetIcon(gConfigData.mMacroIcon);

	mEditTimeDateFormat.SetWindowText(gConfigData.mTimeDateFormat);
	OnChangeEditTimeDateFormat();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void TimeDateFormatDlg::OnChangeEditTimeDateFormat() 
{
	CString string;
	mEditTimeDateFormat.GetWindowText(string);
	string = GetTimeDateString(string);
	mEditTimeDate.SetWindowText(string);

	ScrollTimeDate();
}

void TimeDateFormatDlg::OnVscrollEditTimeDateFormat() 
{
	ScrollTimeDate();
}

void TimeDateFormatDlg::OnOK() 
{
	mEditTimeDateFormat.GetWindowText(gConfigData.mTimeDateFormat);
	gConfigData.Save();
	CDialog::OnOK();
}

BOOL TimeDateFormatDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return TRUE;
}

enum MacroId {midNone = 0, midDate, midLongDate, midTime, midLongTime};

void TimeDateFormatDlg::OnMacroButton() 
{
	CMenu menu;
	VERIFY(menu.CreatePopupMenu());
	VERIFY(menu.AppendMenu(MF_STRING, midDate, _T("Date")));
	VERIFY(menu.AppendMenu(MF_STRING, midLongDate, _T("Long Date")));
	VERIFY(menu.AppendMenu(MF_STRING, midTime, _T("Time")));
	VERIFY(menu.AppendMenu(MF_STRING, midLongTime, _T("Long Time")));

	CRect rc;
	mButtonMacro.GetWindowRect(&rc);
	CPoint pt = rc.CenterPoint();

	UINT flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD;
	MacroId id = (MacroId)menu.TrackPopupMenu(flags, pt.x, pt.y, this);

	CString macro;
	if (id == midDate)
		macro = DateMacro;
	else if (id == midLongDate)
		macro = LongDateMacro;
	else if (id == midTime)
		macro = TimeMacro;
	else if (id == midLongTime)
		macro = LongTimeMacro;

	if (id != midNone)
		mEditTimeDateFormat.ReplaceSel(macro, TRUE);
}
