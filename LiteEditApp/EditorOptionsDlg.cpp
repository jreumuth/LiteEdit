// EditorOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "liteedit.h"
#include "EditorOptionsDlg.h"
#include "ConfigData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// EditorOptionsDlg dialog


EditorOptionsDlg::EditorOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(EditorOptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(EditorOptionsDlg)
	//}}AFX_DATA_INIT
}


void EditorOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(EditorOptionsDlg)
	DDX_Control(pDX, IDC_COMBO_ENCODING, mComboEncoding);
	DDX_Control(pDX, IDC_COMBO_NEW_LINE_TYPE, mComboNewLineType);
	DDX_Control(pDX, IDC_CHECK_TAB_INSERTS_SPACES, mCheckTabInsertsSpaces);
	DDX_Control(pDX, IDC_CHECK_SHOW_WHITESPACE, mCheckShowWhitespace);
	DDX_Control(pDX, IDC_CHECK_SHOW_LINE_NUMBERS, mCheckShowLineNumbers);
	DDX_Control(pDX, IDC_EDIT_DEFAULT_EXT, mEditDefaultExt);
	DDX_Control(pDX, IDC_EDIT_TAB_WIDTH, mEditTabWidth);
	DDX_Control(pDX, IDC_EDIT_MAX_RECENT, mEditMaxRecent);
	DDX_Control(pDX, IDC_CHECK_REMOVE_WHIETSPACE, mCheckRemoveTrailingWhiteSpace);
	DDX_Control(pDX, IDC_CHECK_AUTOINDENT, mCheckAutoIndent);
	//}}AFX_DATA_MAP
}

bool EditorOptionsDlg::Validate() {
	CString defExt;
	mEditDefaultExt.GetWindowText(defExt);
	defExt.TrimLeft('.');

	if (!IsValidFileExt(defExt)) {
		CString message = GetInvalidExtErrorMessage(defExt);
		MessageBox(message, ErrorStr, MB_OK|MB_ICONERROR);
		mEditDefaultExt.SetFocus();
		return false;
	}

	return true;
}

BEGIN_MESSAGE_MAP(EditorOptionsDlg, CDialog)
	//{{AFX_MSG_MAP(EditorOptionsDlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// EditorOptionsDlg message handlers

BOOL EditorOptionsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	mCheckRemoveTrailingWhiteSpace.SetCheck(gConfigData.mRemoveTrailingWhitespaceOnSave ? BST_CHECKED : BST_UNCHECKED);

	int i;
	i = mComboNewLineType.AddString(_T("CR + LF (Windows)"));
	mComboNewLineType.SetItemData(i, nlCRLF);
	i = mComboNewLineType.AddString(_T("LF (Unix)"));
	mComboNewLineType.SetItemData(i, nlLF);
	i = mComboNewLineType.AddString(_T("CR (Macintosh)"));
	mComboNewLineType.SetItemData(i, nlCR);

	VERIFY(FindItemByData(mComboNewLineType, gConfigData.mDefaultNewLineType, i));
	mComboNewLineType.SetCurSel(i);

	i = mComboEncoding.AddString(FileFormatToString(ffAscii));
	mComboEncoding.SetItemData(i, ffAscii);
	i = mComboEncoding.AddString(FileFormatToString(ffUTF16));
	mComboEncoding.SetItemData(i, ffUTF16);
	i = mComboEncoding.AddString(FileFormatToString(ffUTF8));
	mComboEncoding.SetItemData(i, ffUTF8);

	VERIFY(FindItemByData(mComboEncoding, gConfigData.mDefaultFileFormat, i));
	mComboEncoding.SetCurSel(i);

	mCheckAutoIndent.SetCheck(gConfigData.mAutoIndent ? BST_CHECKED : BST_UNCHECKED);
	mCheckShowLineNumbers.SetCheck(gConfigData.mShowLineNumbers ? BST_CHECKED : BST_UNCHECKED);
	mCheckShowWhitespace.SetCheck(gConfigData.mShowWhitespace ? BST_CHECKED : BST_UNCHECKED);
	mCheckTabInsertsSpaces.SetCheck(gConfigData.mTabInsertsSpaces ? BST_CHECKED : BST_UNCHECKED);

	CString num;
	num.Format(TEXT("%d"), gConfigData.mRecentFileList.GetMax());
	mEditMaxRecent.SetWindowText(num);

	mEditDefaultExt.SetWindowText(gConfigData.mDefExt);

	num.Format(TEXT("%d"), gConfigData.mTabWidth);
	mEditTabWidth.SetWindowText(num);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void EditorOptionsDlg::OnOK() 
{
	if (Validate()) {
		gConfigData.mRemoveTrailingWhitespaceOnSave = mCheckRemoveTrailingWhiteSpace.GetCheck() == BST_CHECKED;

		gConfigData.mDefaultNewLineType = (NewLineType)mComboNewLineType.GetItemData(mComboNewLineType.GetCurSel());
		gConfigData.mDefaultFileFormat = (FileFormat)mComboEncoding.GetItemData(mComboEncoding.GetCurSel());

		gConfigData.mAutoIndent = mCheckAutoIndent.GetCheck() == BST_CHECKED;
		gConfigData.mShowLineNumbers = mCheckShowLineNumbers.GetCheck() == BST_CHECKED;
		gConfigData.mShowWhitespace = mCheckShowWhitespace.GetCheck() == BST_CHECKED;
		gConfigData.mTabInsertsSpaces = mCheckTabInsertsSpaces.GetCheck() == BST_CHECKED;

		CString string;
		mEditTabWidth.GetWindowText(string);
		gConfigData.mTabWidth = _tstol(string);

		mEditDefaultExt.GetWindowText(string);
		string.TrimLeft('.');
		gConfigData.mDefExt = string;

		mEditMaxRecent.GetWindowText(string);
		gConfigData.mRecentFileList.SetMax(_tstol(string));

		gConfigData.Save();

		CDialog::OnOK();
	}
}

BOOL EditorOptionsDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return TRUE;
}
