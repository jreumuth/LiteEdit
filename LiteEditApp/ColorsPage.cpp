// ColorsPage.cpp : implementation file
//

#include "stdafx.h"
#include "liteedit.h"
#include "ColorsPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ColorsPage dialog


ColorsPage::ColorsPage()
	: CPropertyPage(IDD_COLORS_PAGE)
{
	//{{AFX_DATA_INIT(ColorsPage)
	//}}AFX_DATA_INIT
}


void ColorsPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ColorsPage)
	DDX_Control(pDX, IDC_EDIT_NAME, mEditName);
	DDX_Control(pDX, IDC_LIST_SCHEMAS, mListSchemes);
	DDX_Control(pDX, IDC_RADIO_USE_SYSTEM_SELECTION_COLORS, mUseSystemSelectionColors);
	DDX_Control(pDX, IDC_RADIO_INVERT_COLORS, mInvertColors);
	DDX_Control(pDX, IDC_COMBO_ITEMS, mComboItems);
	DDX_Control(pDX, IDC_STATIC_COLOR, mStaticColor);
	//}}AFX_DATA_MAP
}

void ColorsPage::StoreData() {
	//persisted within an instance to make it easier to experiment with changing colors
	gConfigData.mCurrentTokenForConfiguringColor = mComboItems.GetCurSel();

	gConfigData.mColorSchemes = mColorSchemes;
}

void ColorsPage::InitEditCtrl() {
	CWnd* edit = GetDlgItem(IDC_EDIT_PREVIEW);
	DWORD styles = edit->GetStyle();
	DWORD exStyles = edit->GetExStyle();
	RECT rc;
	edit->GetWindowRect(&rc);
	ScreenToClient(&rc);

	edit->DestroyWindow();

	mEdit.Create(m_hWnd, rc, styles, exStyles, IDC_EDIT_PREVIEW, new CLanguage());
	mEdit.SetShowGreyedWhenReadOnly(false);
	mEdit.SetReadOnly(true);
	mEdit.SetShowBookmarks(false);
	gConfigData.SetEdit(mEdit);
	mEdit.SetShowLineNumbers(false);

	LPCTSTR initText = TEXT("\
// This is a preview of your currently selected colors.\n\
\n\
// Converts a LF, CR, or CRLF string to a LF string.\n\
void ConvertAnyToLF(LPTSTR dest, LPCTSTR src) {\n\
\twhile (true) {\n\
\t\tif (*src == '\\r' && src[1] != '\\n')//found just CR\n\
\t\t\t*dest = '\\n';\n\
\t\telse {\n\
\t\t\tif (*src == '\\r')//found CRLF, so skip the CR\n\
\t\t\t\t++src;\n\
\n\
\t\t\tATLASSERT(*src != '\\r');//should never happen\n\
\n\
\t\t\t*dest = *src;\n\
\n\
\t\t\tif (*dest == NULL)\n\
\t\t\t\tbreak;\n\
\t\t}\n\
\n\
\t\t++dest;\n\
\t\t++src;\n\
\t}\n\
}\
");

/*
void AccelTable::DecorateMenu(HMENU hMenu) {\n\
\tCMenuHandle menu = hMenu;\n\
\n\
\tfor (size_t i = 0; i < m_accels.size(); ++i) {\n\
\t\tCString string;\n\
\t\tmenu.GetMenuString(m_accels[i].cmd, string, MF_BYCOMMAND);\n\
\n\
\t\t// remove old shortcut\n\
\t\tlong pos = string.Find('\\t');\n\
\t\tif (pos != -1)\n\
\t\t\tstring.Delete(pos, string.GetLength() - pos);\n\
\n\
\t\tstring += \"\\t\" + KeyAndModifiersToString(m_accels[i].GetVirtualKey(), m_accels[i].GetModifiers());\n\
\t\t// ModifyMenu may fail if the command isn't in the menu which is OK.\n\
\t\tmenu.ModifyMenu(m_accels[i].cmd, MF_BYCOMMAND|MF_STRING, m_accels[i].cmd, string);\n\
\t}\n\
}\
";*/

	mEdit.SetText(initText);
}

void ColorsPage::UpdateCurScheme() {
	ColorSchemePtr curScheme = mColorSchemes.GetCurColorScheme();

	if (curScheme != NULL) {
		mEditName.SetWindowText(curScheme->GetName());

		bool useSystem = curScheme->GetSelColor() == scSystem;
		mUseSystemSelectionColors.SetCheck(useSystem ? BST_CHECKED : BST_UNCHECKED);
		mInvertColors.SetCheck(!useSystem ? BST_CHECKED : BST_UNCHECKED);

		SetCurColor(curScheme->Item(GetCurTokenKind()));

		mEdit.SetUseSystemSelectionColors(useSystem);
		mEdit.SetTokenColors(curScheme.GetPtr());
	}
}

void ColorsPage::FillSchemeList() {
	long curScheme = GetCurScheme();

	mListSchemes.SetRedraw(FALSE);
	mListSchemes.ResetContent();
	for (size_t i = 0; i < mColorSchemes.size(); ++i)
		mListSchemes.AddString(mColorSchemes[i]->GetName());

	mListSchemes.SetCurSel(curScheme);
	mListSchemes.SetRedraw(TRUE);
	mListSchemes.Invalidate();
}

void ColorsPage::UpdateEnable() {
	bool hasItems = !mColorSchemes.empty();

	GetDlgItem(IDC_BUTTON_DELETE)->EnableWindow(hasItems);

	GetDlgItem(IDC_STATIC_SCHEME_NAME)->EnableWindow(hasItems);
	GetDlgItem(IDC_EDIT_NAME)->EnableWindow(hasItems);

	GetDlgItem(IDC_STATIC_ITEM)->EnableWindow(hasItems);
	GetDlgItem(IDC_COMBO_ITEMS)->EnableWindow(hasItems);

	GetDlgItem(IDC_STATIC_CURRENT_COLOR)->EnableWindow(hasItems);
	GetDlgItem(IDC_STATIC_COLOR)->EnableWindow(hasItems);
	GetDlgItem(IDC_BUTTON_PICK_COLOR)->EnableWindow(hasItems);

	GetDlgItem(IDC_STATIC_SELECTION_COLORS)->EnableWindow(hasItems);
	GetDlgItem(IDC_RADIO_USE_SYSTEM_SELECTION_COLORS)->EnableWindow(hasItems);
	GetDlgItem(IDC_RADIO_INVERT_COLORS)->EnableWindow(hasItems);

	GetDlgItem(IDC_STATIC_PREVIEW)->EnableWindow(hasItems);
	GetDlgItem(IDC_EDIT_PREVIEW)->EnableWindow(hasItems);

	long i = GetCurScheme();
	GetDlgItem(IDC_BUTTON_MOVE_UP)->EnableWindow(i > 0);
	GetDlgItem(IDC_BUTTON_MOVE_DOWN)->EnableWindow(i < (long)mColorSchemes.size() - 1L);
}

void ColorsPage::SetCurColor(COLORREF clr) {
	if (mCurColor != clr) {
		mCurColor = clr;

		ColorSchemePtr curScheme = mColorSchemes.GetCurColorScheme();
		ATLASSERT(curScheme != NULL);
		if (curScheme->Item(GetCurTokenKind()) != mCurColor) {
			curScheme->Item(GetCurTokenKind()) = mCurColor;
			mEdit.RecolorAll();
		}

		if ((HBRUSH)mBrushCurColor != NULL)
			VERIFY(mBrushCurColor.DeleteObject());

		mBrushCurColor.CreateSolidBrush(mCurColor);
		mStaticColor.Invalidate();

		if (!mUpdatingControls)
			SetModified();
	}
}

void ColorsPage::SetCurScheme(long i, bool alwaysUpdate/* = false*/) {
	if (i != mColorSchemes.GetCurScheme() || alwaysUpdate) {
		mColorSchemes.SetCurScheme(i);
		mListSchemes.SetCurSel(i);
		UpdateCurScheme();
		UpdateEnable();
		SetModified();
	}
}

long ColorsPage::GetCurScheme() {
	return mColorSchemes.GetCurScheme();
}

BEGIN_MESSAGE_MAP(ColorsPage, CDialog)
	//{{AFX_MSG_MAP(ColorsPage)
	ON_BN_CLICKED(IDC_BUTTON_PICK_COLOR, OnButtonPickColor)
	ON_CBN_SELENDOK(IDC_COMBO_ITEMS, OnSelendokComboItems)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_RADIO_INVERT_COLORS, OnRadioInvertColors)
	ON_BN_CLICKED(IDC_RADIO_USE_SYSTEM_SELECTION_COLORS, OnRadioUseSystemSelectionColors)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_LBN_SELCHANGE(IDC_LIST_SCHEMAS, OnSelchangeListSchemes)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_UP, OnButtonMoveUp)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_DOWN, OnButtonMoveDown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ColorsPage message handlers

void ColorsPage::OnButtonPickColor() 
{
	CColorDialog dlg(GetCurColor(), CC_FULLOPEN, this);
	dlg.m_cc.lpCustColors = gConfigData.mCustomColors;

	if (dlg.DoModal() == IDOK)
		SetCurColor(dlg.GetColor());
}

BOOL ColorsPage::OnInitDialog() 
{
	CDialog::OnInitDialog();

	mUpdatingControls = false;

	mColorSchemes = gConfigData.mColorSchemes;

	mCurColor = -1;

	InitEditCtrl();

	int i = 0;
	for (long tokenKind = tkStart; tokenKind < tkEnd; ++tokenKind) {
		i = mComboItems.AddString(TokenToString((TokenKinds)tokenKind, tsoSeparateWords));
		mComboItems.SetItemData(i, tokenKind);
	}

	mComboItems.SetCurSel(gConfigData.mCurrentTokenForConfiguringColor);
	OnSelendokComboItems();

	FillSchemeList();	
	UpdateCurScheme();
	UpdateEnable();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL ColorsPage::OnApply() {
	StoreData();
	gConfigData.Save();
	return TRUE;
}

void ColorsPage::OnSelendokComboItems() 
{
	AutoBool autoBool(mUpdatingControls, true);

	ColorSchemePtr cs = mColorSchemes.GetCurColorScheme();
	if (cs != NULL) {
		COLORREF clr = cs->Item(GetCurTokenKind());
		SetCurColor(clr);
	}
}

HBRUSH ColorsPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_COLOR && (HBRUSH)mBrushCurColor != NULL)
		return mBrushCurColor;
	else
		return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void ColorsPage::OnRadioInvertColors() 
{
	mColorSchemes.GetCurColorScheme()->SetSelColor(scInvert);
	mEdit.SetUseSystemSelectionColors(mInvertColors.GetCheck() == BST_UNCHECKED);
	SetModified();
}

void ColorsPage::OnRadioUseSystemSelectionColors() 
{
	mColorSchemes.GetCurColorScheme()->SetSelColor(scSystem);
	mEdit.SetUseSystemSelectionColors(mUseSystemSelectionColors.GetCheck() == BST_CHECKED);
	SetModified();
}

void ColorsPage::OnHelp() {
	gConfigData.ExecuteHelp(hfSyntaxColoring, m_hWnd);
}

void ColorsPage::OnSelchangeListSchemes() 
{
	AutoBool autoBool(mUpdatingControls, true);
	SetCurScheme(mListSchemes.GetCurSel());	
}

void ColorsPage::OnChangeEditName() 
{
	CString text;
	mEditName.GetWindowText(text);
	SetListBoxItemText(mListSchemes, mListSchemes.GetCurSel(), text);
	mColorSchemes.GetCurColorScheme()->SetName(text);

	if (!mUpdatingControls)
		SetModified();
}

void ColorsPage::OnButtonNew() 
{
	long i = GetCurScheme();
	ColorSchemePtr cs = new ColorScheme();
	//copy selected color scheme
	if (i >= 0)
		*cs = *mColorSchemes[i];

	cs->SetName(TEXT("New Color Scheme"));

	mColorSchemes.push_back(cs);
	FillSchemeList();
	SetCurScheme(mColorSchemes.size() - 1);
	UpdateEnable();
	SetModified();
}

void ColorsPage::OnButtonDelete() 
{
	long i = GetCurScheme();
	ASSERT(i >= 0);//button should be disabled
	mColorSchemes.erase(mColorSchemes.begin() + i);
	i = Min(i, (long)mColorSchemes.size() - 1L);
	FillSchemeList();
	SetCurScheme(i, true);
	UpdateEnable();
	SetModified();
}

void ColorsPage::OnButtonMoveUp() 
{
	long i = GetCurScheme();
	ASSERT(i > 0);//button should be disabled
	swap(mColorSchemes[i], mColorSchemes[i-1]);
	FillSchemeList();
	SetCurScheme(i - 1);
	SetModified();
}

void ColorsPage::OnButtonMoveDown() 
{
	long i = GetCurScheme();
	ASSERT(i < mColorSchemes.size() - 1);//button should be disabled
	swap(mColorSchemes[i], mColorSchemes[i+1]);
	FillSchemeList();	
	SetCurScheme(i + 1);
	SetModified();
}
