// FindReplaceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "liteedit.h"
#include "FindReplaceDlg.h"
#include "ConfigData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT WM_FINDREPLACE = ::RegisterWindowMessage(TEXT("LiteEdit_FindReplaceDlg"));

/////////////////////////////////////////////////////////////////////////////
// FindReplaceDlg dialog


FindReplaceDlg::FindReplaceDlg(bool find, const CString& initFindText, bool readOnly, CWnd* pParent /*=NULL*/)
	: mFind(find), mInitFindText(initFindText), CDialog(FindReplaceDlg::IDD, pParent),
		mReadOnly(readOnly),
		mSwapClipKey(0), mSwapClipMods(kmNone),
		mLastClipKey(0), mLastClipMods(kmNone),
		mNewSearch(true)
{
	//{{AFX_DATA_INIT(FindReplaceDlg)
	//}}AFX_DATA_INIT
}


void FindReplaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(FindReplaceDlg)
	DDX_Control(pDX, IDC_CHECK_UP, mCheckUp);
	DDX_Control(pDX, IDC_CHECK_SELECTION_ONLY, mCheckSelectionOnly);
	DDX_Control(pDX, IDCANCEL, mBtnCancel);
	DDX_Control(pDX, IDC_STATIC_REPLACE_WITH, mStaticReplaceWith);
	DDX_Control(pDX, IDC_CHECK_ESCAPED_CHARS, mCheckEscapedChars);
	DDX_Control(pDX, IDC_COMBO_REPLACE, mComboReplace);
	DDX_Control(pDX, IDC_COMBO_FIND, mComboFind);
	DDX_Control(pDX, IDC_CHECK_WHOLE_WORD, mCheckWholeWord);
	DDX_Control(pDX, IDC_CHECK_CLOSE, mCheckClose);
	DDX_Control(pDX, IDC_CHECK_CASE, mCheckCase);
	DDX_Control(pDX, IDC_BUTTON_REPLACE_ALL, mBtnReplaceAll);
	DDX_Control(pDX, IDC_BUTTON_REPLACE, mBtnReplace);
	DDX_Control(pDX, IDC_BUTTON_FIND, mBtnFind);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(FindReplaceDlg, CDialog)
	//{{AFX_MSG_MAP(FindReplaceDlg)
	ON_BN_CLICKED(IDC_BUTTON_FIND, OnButtonFind)
	ON_BN_CLICKED(IDC_BUTTON_REPLACE, OnButtonReplace)
	ON_BN_CLICKED(IDC_BUTTON_REPLACE_ALL, OnButtonReplaceAll)
	ON_CBN_EDITCHANGE(IDC_COMBO_FIND, OnEditchangeComboFind)
	ON_BN_CLICKED(IDC_CHECK_SELECTION_ONLY, OnCheckSelectionOnly)
	ON_CBN_SELENDOK(IDC_COMBO_FIND, OnSelendokComboFind)
	ON_CBN_SELCHANGE(IDC_COMBO_FIND, OnSelchangeComboFind)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL FindReplaceDlg::PreTranslateMessage(MSG* pMsg) {
	//handle the switch clipboard hotkey
	if (pMsg->message == WM_KEYDOWN) {
		WPARAM keyCode = pMsg->wParam;
		KeyModifiers mods = GetKeyModifiers();

		if (keyCode == mSwapClipKey && mods == mSwapClipMods && gConfigData.CanSwitchClipboard()) {
			gConfigData.SwitchClipboard(m_hWnd);
			return TRUE;
		} else if (keyCode == mLastClipKey && mods == mLastClipMods && gConfigData.CanLastClipboard()) {
			gConfigData.LastClipboard(m_hWnd);
			return TRUE;
		} else if (keyCode == 'A' && mods == kmCtrl) {
			CWnd* focused = GetFocus();
			if (focused != NULL)
				focused = focused->GetParent();

			if (&mComboFind == focused) {
				mComboFind.SetEditSel(0, -1);
				return TRUE;
			} else if (&mComboReplace == focused) {
				mComboReplace.SetEditSel(0, -1);
				return TRUE;
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void FindReplaceDlg::Show(CWnd* parent) {
	Create(IDD, parent);

	// calculate position to center window within parent. size is already correct.
	CRect rcParent, rcCurr;
	CPoint ptInit;
	GetWindowRect(&rcCurr);
	parent->GetWindowRect(&rcParent);
	ptInit = rcParent.CenterPoint();
	ptInit.Offset(-rcCurr.Width() / 2, -rcCurr.Height() / 2);

	// move to center and show window
	SetWindowPos(NULL, ptInit.x, ptInit.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
}

void FindReplaceDlg::UpdateEnable() {
	bool hasFind = mComboFind.GetWindowTextLength() > 0;
	bool selOnly = !mFind && mCheckSelectionOnly.GetCheck() == BST_CHECKED;
	mBtnFind.EnableWindow(!selOnly && hasFind);
	mBtnReplace.EnableWindow(!selOnly && hasFind && !mReadOnly);
	mBtnReplaceAll.EnableWindow(hasFind && !mReadOnly);
}

void FindReplaceDlg::AddStringToRecentStrings(const CString& string, StringVector& vec) {
	if (!string.IsEmpty()) {
		long i = vec.FindString(string, true);
		if (i >= 0)
			vec.erase(vec.begin() + i);

		vec.insert(vec.begin(), string);
	}
}

void FindReplaceDlg::UpdateData(FindKind fk) {
	FindReplaceData& data = gConfigData.mFindReplaceData;

	CString string;
	mComboFind.GetWindowText(string);
	data.findStr = string;
	AddStringToRecentStrings(string, data.findStrings);

	if (!mFind) {
		mComboReplace.GetWindowText(string);
		data.replaceStr = string;
		AddStringToRecentStrings(string, data.replaceStrings);
	}

	data.flags = ffIgnoreCRs;
	if (mCheckCase.GetCheck() == BST_CHECKED)
		data.flags |= ffMatchCase;
	else
		data.flags |= ffIgnoreCase;
	if (mCheckWholeWord.GetCheck() == BST_CHECKED)
		data.flags |= ffWholeWord;
	if (mCheckUp.GetCheck() == BST_CHECKED)
		data.flags |= ffReverse;
	data.selectionOnly = mCheckSelectionOnly.GetCheck() == BST_CHECKED;
	data.closeOnFind = mCheckClose.GetCheck() == BST_CHECKED;
	data.escapedChars = mCheckEscapedChars.GetCheck() == BST_CHECKED;

	if (data.escapedChars) {
		data.findStr = UnescapeChars(data.findStr);
		data.replaceStr = UnescapeChars(data.replaceStr);
	}

	data.findNext = fk == fkFind || fk == fkReplace;
	data.replaceCurrent = fk == fkReplace;
	data.replaceAll = fk == fkReplaceAll;

	data.newSearch = mNewSearch;
	mNewSearch = false;

	bool doCloseOnFind = mFind && data.closeOnFind;
	if (doCloseOnFind)
		ShowWindow(SW_HIDE);

	SendFindReplaceMsg();

	if (doCloseOnFind)
		VERIFY(DestroyWindow());
}

void FindReplaceDlg::SendFindReplaceMsg() {
	GetParent()->SendMessage(WM_FINDREPLACE);
}

/////////////////////////////////////////////////////////////////////////////
// FindReplaceDlg message handlers

BOOL FindReplaceDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	gConfigData.mAccelTable.GetHotKey(ID_EDIT_ADVANCED_SWITCHCLIPBOARD, mSwapClipKey, mSwapClipMods);
	gConfigData.mAccelTable.GetHotKey(ID_EDIT_ADVANCED_LASTCLIPBOARD, mLastClipKey, mLastClipMods);

	mNewSearch = true;

	FindReplaceData& data = gConfigData.mFindReplaceData;

	if (mFind) {
		SetWindowText(TEXT("Find"));

		mCheckSelectionOnly.ShowWindow(SW_HIDE);

		CRect rc;
		mBtnReplace.GetWindowRect(&rc);
		ScreenToClient(&rc);
		mBtnCancel.MoveWindow(&rc);

		int dy = -24;
		MoveWindowBy(mCheckCase, 0, dy);
		MoveWindowBy(mCheckWholeWord, 0, dy);
		MoveWindowBy(mCheckUp, 0, dy);
		MoveWindowBy(mCheckClose, 0, dy);
		MoveWindowBy(mCheckEscapedChars, 0, dy);

		GetWindowRect(&rc);
		rc.bottom += dy;
		MoveWindow(&rc);

		mStaticReplaceWith.ShowWindow(SW_HIDE);
		mComboReplace.ShowWindow(SW_HIDE);
		mBtnReplace.ShowWindow(SW_HIDE);
		mBtnReplaceAll.ShowWindow(SW_HIDE);
		mCheckSelectionOnly.ShowWindow(SW_HIDE);
	} else {
		SetWindowText(TEXT("Replace"));

		mCheckClose.ShowWindow(SW_HIDE);

		StringVectorToComboBox(mComboReplace, data.replaceStrings);
	}

	StringVectorToComboBox(mComboFind, data.findStrings);

	mCheckCase.SetCheck((data.flags & ffMatchCase) != 0 ? BST_CHECKED : BST_UNCHECKED);
	mCheckWholeWord.SetCheck((data.flags & ffWholeWord) != 0 ? BST_CHECKED : BST_UNCHECKED);
	mCheckUp.SetCheck(mFind && (data.flags & ffReverse) != 0 ? BST_CHECKED : BST_UNCHECKED);
	mCheckSelectionOnly.SetCheck(data.selectionOnly ? BST_CHECKED : BST_UNCHECKED);
	mCheckClose.SetCheck(data.closeOnFind ? BST_CHECKED : BST_UNCHECKED);
	mCheckEscapedChars.SetCheck(data.escapedChars ? BST_CHECKED : BST_UNCHECKED);

	mComboFind.SetWindowText(mInitFindText);
	VERIFY(mComboFind.SetEditSel(0, -1));//select initial find text

	UpdateEnable();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void FindReplaceDlg::OnButtonFind() 
{
	UpdateData(fkFind);
}

void FindReplaceDlg::OnButtonReplace() 
{
	UpdateData(fkReplace);
}

void FindReplaceDlg::OnButtonReplaceAll() 
{
	UpdateData(fkReplaceAll);
}

void FindReplaceDlg::OnEditchangeComboFind() 
{
	UpdateEnable();
}

void FindReplaceDlg::OnCheckSelectionOnly() 
{
	UpdateEnable();
}

void FindReplaceDlg::OnSelendokComboFind() 
{
	UpdateEnable();
}

void FindReplaceDlg::OnSelchangeComboFind() 
{
	//Since the text doesn't update as soon as the selection changes,
	//I must update the text myself for UpdateEnable() to work properly.
	int i = mComboFind.GetCurSel();
	ASSERT(i >= 0);
	CString text;
	mComboFind.GetLBText(i, text);
	mComboFind.SetWindowText(text);

	UpdateEnable();	
}

BOOL FindReplaceDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return TRUE;
}