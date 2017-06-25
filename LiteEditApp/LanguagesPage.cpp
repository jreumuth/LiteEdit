// LanguagesPage.cpp : implementation file
//

#include "stdafx.h"
#include "liteedit.h"
#include "LanguagesPage.h"
#include "ConfigData.h"
#include "PathMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CString GetLangElemDisplayString(LangElemKinds langElemKind, TokenKinds tokenKind, const StringVector& strings) {
	CString res = TokenToString(tokenKind, tsoSeparateWords);
	if (langElemKind == leWords)
		res += 's';
	if (langElemKind == leRestOfLine || langElemKind == leRange)
		res = strings[0] + ' ' + res;
	if (langElemKind == leRange)
		res += ' ' + strings[1];
	return res;
}

LangElemData::LangElemData(LangElemPtr langElem) {
	mTokenKind = langElem->GetTokenKind();
	mLangElemKind = langElem->GetLangElemKind();
	mOptions = langElem->GetOptions();

	for (long i = 0; i < langElem->GetNumStrings(); ++i)
		mStrings.push_back(langElem->GetNthString(i));

	//GetNumStrings()/GetNthString() only returns start marker for ranges
	if (mLangElemKind == leRange) {
		Range* range = (Range*)langElem.GetPtr();
		mStrings.push_back(range->GetEndMarker());
		mStrings.push_back(range->GetEscapeChar());
	}
}

LangElemPtr LangElemData::CreateLangElem() const {
	if (mLangElemKind == leWords) {
		WordsPtr words = new Words(mTokenKind, mOptions);
		for (size_t i = 0; i < mStrings.size(); ++i)
			words->AddWord(mStrings[i]);

		return words;
	} else if (mLangElemKind == leRestOfLine)
		return new RestOfLine(mTokenKind, mOptions, mStrings[0]);
	else if (mLangElemKind == leRange)
		return new Range(mTokenKind, mOptions, mStrings[0], mStrings[1], mStrings[2]);

	ASSERT(false);
	return NULL;
}

CString LangElemData::ToString() const {
	return GetLangElemDisplayString(mLangElemKind, mTokenKind, mStrings);
}

ValidateLangElemData LangElemData::Validate(bool matchCase) {
	bool wholeWord = (mOptions & WholeWord) != 0;

	//remove empty lines from word lists
	if (mLangElemKind == leWords) {
		mStrings.RemoveAllOccurrencesOf(EmptyStr, matchCase);
		mStrings.RemoveDuplicates(matchCase);

		if (wholeWord)//don't sort symbols
			sort(mStrings.begin(), mStrings.end());
	}

	size_t i;
	for (i = 0; i < mStrings.size(); ++i) {
		mStrings[i].TrimLeft();
		mStrings[i].TrimRight();

		if (mStrings[i].FindOneOf(TEXT(" \t")) >= 0) {
			CString message;
			message.Format(TEXT("'%s' contains white space. Language element strings cannot contain white space."), mStrings[i]);
			return ValidateLangElemData(message);
		}
	}

	if (mLangElemKind == leRestOfLine || mLangElemKind == leRange)
		if (mStrings[0].IsEmpty())
			return ValidateLangElemData(TEXT("Start marker cannot be empty."));

	if (mLangElemKind == leRange) {
		if (mStrings[1].IsEmpty())
			return ValidateLangElemData(TEXT("End marker cannot be empty."));

		const CString& escapeChar = mStrings[2];
		//don't validate escape char if whole word is specified
		//because the escape char will not be used
		if (!wholeWord && !escapeChar.IsEmpty()) {
			if (StrCmpEx(escapeChar, mStrings[0], matchCase) == 0)
				return ValidateLangElemData(TEXT("Escape char cannot be the same as the start marker."));
			if (StrCmpEx(escapeChar, mStrings[1], matchCase) == 0)
				return ValidateLangElemData(TEXT("Escape char cannot be the same as the end marker."));
		}

		//if not spanning lines, then we cannot nest
		if ((mOptions & SpanLines) == 0)//if not spanning lines
			mOptions &= ~Nested;//remove nested

		if ((mOptions & Nested) != 0 && StrCmpEx(mStrings[0], mStrings[1], matchCase) == 0)
			return ValidateLangElemData(TEXT("Start and End marker cannot be the same if the Allow Nesting option is selected."));
	} else
		//only ranges can nest or span lines
		mOptions &= ~(Nested|SpanLines);

	for (i = 0; i < mStrings.size(); ++i)
		if (!mStrings[i].IsEmpty() && !BoolEq(wholeWord, IsValidIdentifier(mStrings[i]))) {
			CString message;
			message.Format(TEXT("'%s' is not consistent with Whole Word option. Language element strings should be valid identifiers if and only if the Whole Word option is selected. Valid identifiers begin with a letter or underscore and consist of only letters, numbers, and underscores."), mStrings[i]);
			return ValidateLangElemData(message);
		}

	return ValidateLangElemData();
}

LangData::LangData(LanguagePtr lang) {
	mName = lang->GetName();
	mMatchCase = lang->GetMatchCase();
	mExtensions = lang->m_extensions;
	mDir = lang->GetFileDir();

	for (long i = 0; i < lang->GetLangElems().size(); ++i)
		mElemData.push_back(LangElemData(lang->GetLangElems()[i]));
}

LanguagePtr LangData::CreateLanguage() const {
	LanguagePtr lang = new Language(mName, mMatchCase);
	lang->m_extensions = mExtensions;
	lang->SetFileDir(mDir);

	for (size_t i = 0; i < mElemData.size(); ++i)
		lang->AddLangElem(mElemData[i].CreateLangElem());

	return lang;
}

ValidateLangData LangData::Validate() {
	if (mName.IsEmpty())
		return ValidateLangData(TEXT("Language name cannot be empty."));

	if (!IsValidFileName(mName)) {
		CString message;
		message.Format(TEXT("Language name '%s' is not valid. Language names cannot contain any of the following characters:\n\t%s"), mName, InvalidFileNameChars);
		return ValidateLangData(message);
	}

	long i;
	for (i = 0; i < mExtensions.size(); ++i) {
		CString& ext = mExtensions[i];
		ext.TrimLeft();
		ext.TrimRight();
		ext.TrimLeft('.');
		if (!IsValidFileExt(ext)) {
			CString message = GetInvalidExtErrorMessage(ext);
			return ValidateLangData(message);
		}
	}

	for (i = 0; i < mElemData.size(); ++i) {
		ValidateLangElemData data = mElemData[i].Validate(mMatchCase);
		if (!data.valid)
			return ValidateLangData(data, i);
	}

	return ValidateLangData();
}

/////////////////////////////////////////////////////////////////////////////
// LanguagesPage dialog

LanguagesPage::LanguagesPage(const CString& initLang)
	: CPropertyPage(IDD_LANGUAGES_PAGE), mInitLang(initLang)
{
	//{{AFX_DATA_INIT(LanguagesPage)
	//}}AFX_DATA_INIT
}

bool LanguagesPage::StoreLangData() {
	UpdateCurData();

	if (!ValidateAllData())
		return false;

	Languages& langs = gConfigData.mLangMgr.mLangs;

	// we delete and recreate all the existing languages to handling
	// deletes and renames
	size_t i;
	for (i = 0; i < langs.size(); ++i) {
		BOOL succ = DeleteFile(langs[i]->GetFilePath());
		DEBUG_DISPLAY_LAST_ERROR(succ, _T("deleting existing language file '") + langs[i]->GetFilePath() + _T("'"));
	}

	langs.clear();

	for (i = 0; i < mLangData.size(); ++i)
		langs.push_back(mLangData[i].CreateLanguage());	

	return true;
}

void LanguagesPage::LoadLangData() {
	mLangData.clear();

	Languages& langs = gConfigData.mLangMgr.mLangs;
	for (long i = 0; i < langs.size(); ++i)
		mLangData.push_back(LangData(langs[i]));
}

void LanguagesPage::InitControls() {
	int i;
	int index;

	for (i = tkStart; i < tkEnd; ++i)
		if (i != tkPlainText && i != tkBackground) {
			index = mComboKind.AddString(TokenToString((TokenKinds)i, tsoSeparateWords));
			mComboKind.SetItemData(index, i);
		}

	i = mComboElemDef.AddString(TEXT("Words"));
	mComboElemDef.SetItemData(i, leWords);
		
	i = mComboElemDef.AddString(TEXT("Rest Of Line"));
	mComboElemDef.SetItemData(i, leRestOfLine);

	i = mComboElemDef.AddString(TEXT("Range"));
	mComboElemDef.SetItemData(i, leRange);

	RefreshLangList();
}

void LanguagesPage::UpdateState() {
	bool hasLang = GetCurLang() >= 0;
	bool hasElem = hasLang && GetCurElem() >= 0;
	bool wholeWord = mCheckWholeWord.GetCheck() == BST_CHECKED;

	// update enabled

	mBtnRemoveLang.EnableWindow(hasLang);

	mStaticLangOptions.EnableWindow(hasLang);
	mStaticLangName.EnableWindow(hasLang);
	mEditLangName.EnableWindow(hasLang);
	mStaticFileExtensions.EnableWindow(hasLang);
	mEditExtensions.EnableWindow(hasLang);
	mCheckCaseSensitive.EnableWindow(hasLang);

	mStaticElem.EnableWindow(hasLang);
	mListElem.EnableWindow(hasLang);
	mBtnAddElem.EnableWindow(hasLang);
	mBtnRemoveElem.EnableWindow(hasElem);

	mStaticElemDef.EnableWindow(hasElem);
	mComboElemDef.EnableWindow(hasElem);
	mStaticKind.EnableWindow(hasElem);
	mComboKind.EnableWindow(hasElem);
	mCheckWholeWord.EnableWindow(hasElem);
	mStaticStart.EnableWindow(hasElem);
	mEditStart.EnableWindow(hasElem);
	mStaticEnd.EnableWindow(hasElem);
	mEditEnd.EnableWindow(hasElem);
	mStaticEscapeChar.EnableWindow(hasElem && !wholeWord);
	mEditEscapeChar.EnableWindow(hasElem && !wholeWord);
	mStaticWordList.EnableWindow(hasElem);
	mEditWordList.EnableWindow(hasElem);
	mCheckSpanLines.EnableWindow(hasElem);
	mCheckAllowNesting.EnableWindow(hasElem && mCheckSpanLines.GetCheck() == BST_CHECKED);

	mBtnUp.EnableWindow(hasElem && GetCurElem() > 0);
	mBtnDown.EnableWindow(hasElem && GetCurElem() < (int)mLangData[GetCurLang()].mElemData.size() - 1);

	// update visiblity

	LangElemKinds le = (LangElemKinds)mComboElemDef.GetItemData(mComboElemDef.GetCurSel());
	mStaticStart.ShowWindow(le == leRestOfLine || le == leRange ? SW_SHOWNA : SW_HIDE);
	mEditStart.ShowWindow(le == leRestOfLine || le == leRange ? SW_SHOWNA : SW_HIDE);
	mStaticEnd.ShowWindow(le == leRange ? SW_SHOWNA : SW_HIDE);
	mEditEnd.ShowWindow(le == leRange ? SW_SHOWNA : SW_HIDE);
	mStaticEscapeChar.ShowWindow(le == leRange ? SW_SHOWNA : SW_HIDE);
	mEditEscapeChar.ShowWindow(le == leRange ? SW_SHOWNA : SW_HIDE);
	mStaticWordList.ShowWindow(le == leWords ? SW_SHOWNA : SW_HIDE);
	mEditWordList.ShowWindow(le == leWords ? SW_SHOWNA : SW_HIDE);
	mCheckSpanLines.ShowWindow(le == leRange ? SW_SHOWNA : SW_HIDE);
	mCheckAllowNesting.ShowWindow(le == leRange ? SW_SHOWNA : SW_HIDE);
}

void LanguagesPage::RefreshLangList() {
	mListLang.ResetContent();
	for (size_t i = 0; i < mLangData.size(); ++i)
		mListLang.AddString(mLangData[i].mName);
}

void LanguagesPage::RefreshElemList() {
	mListElem.ResetContent();

	if (GetCurLang() >= 0) {
		LangData& curLang = mLangData[GetCurLang()];
		for (size_t i = 0; i < curLang.mElemData.size(); ++i)
			mListElem.AddString(curLang.mElemData[i].ToString());
	}
}

void LanguagesPage::UpdateElemText() {
	if (!mUpdatingElem) {
		int curElemKind = mComboElemDef.GetCurSel();
		int curTokenKind = mComboKind.GetCurSel();

		if (curElemKind >= 0 && curTokenKind >= 0 && GetCurLang() >= 0 && GetCurElem() >= 0) {
			LangElemKinds langElemKind = (LangElemKinds)mComboElemDef.GetItemData(curElemKind);
			TokenKinds tokenKind = (TokenKinds)mComboKind.GetItemData(curTokenKind);
			StringVector strings;
			CString temp;
			mEditStart.GetWindowText(temp);
			strings.push_back(temp);
			mEditEnd.GetWindowText(temp);
			strings.push_back(temp);

			CString langElemString = GetLangElemDisplayString(langElemKind, tokenKind, strings);
			SetListBoxItemText(mListElem, GetCurElem(), langElemString);
		}
	}
}

void LanguagesPage::SetCurLang(int i) {
	if (i != mCurLang) {
		AutoBool autoBool(mUpdatingControls, true);

		if (mCurLang >= 0)
			UpdateCurData();

		SetCurElem(-1);
		mCurLang = i;

		if (mCurLang >= 0) {
			LangData& curLang = mLangData[mCurLang];
			mEditLangName.SetWindowText(curLang.mName);
			mCheckCaseSensitive.SetCheck(curLang.mMatchCase ? BST_CHECKED : BST_UNCHECKED);
			mEditExtensions.SetWindowText(curLang.mExtensions.Concat(TEXT("; ")));

			RefreshElemList();

			SetCurElem(curLang.mElemData.empty() ? -1 : 0);
		}

		UpdateState();
		mListLang.SetCurSel(mCurLang);
	}
}

void LanguagesPage::SetCurElem(int i) {
	if (i != mCurElem) {
		AutoBool autoBool(mUpdatingControls, true);
		mUpdatingElem = true;

		if (mCurElem >= 0)
			UpdateCurData();

		mCurElem = i;

		mEditStart.SetWindowText(EmptyStr);
		mEditEnd.SetWindowText(EmptyStr);
		mEditEscapeChar.SetWindowText(EmptyStr);
		mEditWordList.SetWindowText(EmptyStr);

		if (mCurElem >= 0) {
			ASSERT(mCurLang >= 0);
			LangElemData& curElem = mLangData[mCurLang].mElemData[mCurElem];

			VERIFY(FindItemByData(mComboElemDef, curElem.mLangElemKind, i));
			mComboElemDef.SetCurSel(i);

			VERIFY(FindItemByData(mComboKind, curElem.mTokenKind, i));
			mComboKind.SetCurSel(i);

			if (curElem.mLangElemKind == leWords)
				mEditWordList.SetWindowText(curElem.mStrings.Concat(TEXT("\r\n")));
			if (curElem.mLangElemKind == leRestOfLine || curElem.mLangElemKind == leRange)
				mEditStart.SetWindowText(curElem.mStrings[0]);
			if (curElem.mLangElemKind == leRange) {
				mEditEnd.SetWindowText(curElem.mStrings[1]);
				mEditEscapeChar.SetWindowText(curElem.mStrings[2]);
			}

			mCheckWholeWord.SetCheck((curElem.mOptions & WholeWord) != 0 ? BST_CHECKED : BST_UNCHECKED);
			mCheckSpanLines.SetCheck((curElem.mOptions & SpanLines) != 0 ? BST_CHECKED : BST_UNCHECKED);
			mCheckAllowNesting.SetCheck((curElem.mOptions & Nested) != 0 ? BST_CHECKED : BST_UNCHECKED);
		}

		mListElem.SetCurSel(mCurElem);
		mUpdatingElem = false;
		UpdateElemText();
		UpdateState();
	}
}

void LanguagesPage::UpdateCurData() {
	if (mCurLang >= 0) {
		LangData& curLang = mLangData[mCurLang];
		mEditLangName.GetWindowText(curLang.mName);
		curLang.mMatchCase = mCheckCaseSensitive.GetCheck() == BST_CHECKED;
		CString exts;
		mEditExtensions.GetWindowText(exts);
		StringListToStringVector(exts, TEXT(";"), curLang.mExtensions);

		if (mCurElem >= 0) {
			LangElemData& curElem = curLang.mElemData[mCurElem];
			curElem.mTokenKind = GetTokenKind();
			LangElemKinds elemKind = curElem.mLangElemKind = GetLangElemKind();
			curElem.mOptions = GetOptions();

			curElem.mStrings.clear();
			CString string;

			if (elemKind == leWords) {
				mEditWordList.GetWindowText(string);
				ConvertCRLFtoLF(string);
				StringListToStringVector(string, TEXT("\n"), curElem.mStrings);
			}

			if (elemKind == leRestOfLine || elemKind == leRange) {
				mEditStart.GetWindowText(string);
				curElem.mStrings.push_back(string);
			}

			if (elemKind == leRange) {
				mEditEnd.GetWindowText(string);
				curElem.mStrings.push_back(string);
				mEditEscapeChar.GetWindowText(string);
				curElem.mStrings.push_back(string);
			}
		}
	}
}

bool LanguagesPage::ValidateAllData() {
	for (size_t i1 = 0; i1 < mLangData.size(); ++i1) {
		//check that the language name is unique
		for (size_t i2 = i1 + 1; i2 < mLangData.size(); ++i2)
			if (StrCmpI(mLangData[i1].mName, mLangData[i2].mName) == 0) {
				//the 2nd language with the same name is the one that was just added,
				//so select that one
				SetCurLang(i2);
				CString msg;
				msg.Format(TEXT("More than one language named '%s' exists. Language names are not case sensitive."), mLangData[i1].mName);
				MessageBox(msg, ErrorStr, MB_OK|MB_ICONERROR);
				return false;
			}

		//validate the language data
		ValidateLangData data = mLangData[i1].Validate();
		if (!data.valid) {
			SetCurLang(i1);
			SetCurElem(data.elem);
			MessageBox(data.message, ErrorStr, MB_OK|MB_ICONERROR);
			return false;
		}
	}

	return true;
}

void LanguagesPage::SetModifiedIfNotUpdating() {
	if (!mUpdatingControls)
		SetModified();
}

LangElemOptions LanguagesPage::GetOptions() const {
	LangElemOptions res = NoOptions;
	if (mCheckWholeWord.GetCheck() == BST_CHECKED)
		res |= WholeWord;
	if (mCheckSpanLines.GetCheck() == BST_CHECKED)
		res |= SpanLines;
	if (mCheckAllowNesting.GetCheck() == BST_CHECKED)
		res |= Nested;
	return res;
}

void LanguagesPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(LanguagesPage)
	DDX_Control(pDX, IDC_EDIT_ESCAPE_CHAR, mEditEscapeChar);
	DDX_Control(pDX, IDC_STATIC_ESCAPE_CHAR, mStaticEscapeChar);
	DDX_Control(pDX, IDC_BUTTON_UP, mBtnUp);
	DDX_Control(pDX, IDC_BUTTON_DOWN, mBtnDown);
	DDX_Control(pDX, IDC_STATIC_FILE_EXTENSIONS, mStaticFileExtensions);
	DDX_Control(pDX, IDC_STATIC_LANG_NAME, mStaticLangName);
	DDX_Control(pDX, IDC_STATIC_LANG_OPTIONS, mStaticLangOptions);
	DDX_Control(pDX, IDC_BUTTON_REMOVE_LANG, mBtnRemoveLang);
	DDX_Control(pDX, IDC_BUTTON_ADD_LANG, mBtnAddLang);
	DDX_Control(pDX, IDC_EDIT_LANG_NAME, mEditLangName);
	DDX_Control(pDX, IDC_EDIT_EXTENSIONS, mEditExtensions);
	DDX_Control(pDX, IDC_STATIC_KIND, mStaticKind);
	DDX_Control(pDX, IDC_STATIC_ELEM_DEF, mStaticElemDef);
	DDX_Control(pDX, IDC_STATIC_START, mStaticStart);
	DDX_Control(pDX, IDC_STATIC_END, mStaticEnd);
	DDX_Control(pDX, IDC_STATIC_WORD_LIST, mStaticWordList);
	DDX_Control(pDX, IDC_STATIC_ELEM, mStaticElem);
	DDX_Control(pDX, IDC_BUTTON_REMOVE_ELEM, mBtnRemoveElem);
	DDX_Control(pDX, IDC_BUTTON_ADD_ELEM, mBtnAddElem);
	DDX_Control(pDX, IDC_EDIT_WORD_LIST, mEditWordList);
	DDX_Control(pDX, IDC_COMBO_ELEM_DEF, mComboElemDef);
	DDX_Control(pDX, IDC_LIST_LANG, mListLang);
	DDX_Control(pDX, IDC_LIST_ELEM, mListElem);
	DDX_Control(pDX, IDC_EDIT_START, mEditStart);
	DDX_Control(pDX, IDC_EDIT_END, mEditEnd);
	DDX_Control(pDX, IDC_COMBO_KIND, mComboKind);
	DDX_Control(pDX, IDC_CHECK_SPAN_LINES, mCheckSpanLines);
	DDX_Control(pDX, IDC_CHECK_WHOLE_WORD, mCheckWholeWord);
	DDX_Control(pDX, IDC_CHECK_CASE_SENSITIVE, mCheckCaseSensitive);
	DDX_Control(pDX, IDC_CHECK_ALLOW_NESTING, mCheckAllowNesting);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(LanguagesPage, CDialog)
	//{{AFX_MSG_MAP(LanguagesPage)
	ON_CBN_SELENDOK(IDC_COMBO_ELEM_DEF, OnSelendokComboElemDef)
	ON_CBN_SELENDOK(IDC_COMBO_KIND, OnSelendokComboKind)
	ON_BN_CLICKED(IDC_CHECK_SPAN_LINES, OnCheckSpanLines)
	ON_LBN_SELCHANGE(IDC_LIST_LANG, OnSelchangeListLang)
	ON_LBN_SELCHANGE(IDC_LIST_ELEM, OnSelchangeListElem)
	ON_BN_CLICKED(IDC_BUTTON_ADD_ELEM, OnButtonAddElem)
	ON_BN_CLICKED(IDC_BUTTON_ADD_LANG, OnButtonAddLang)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_ELEM, OnButtonRemoveElem)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_LANG, OnButtonRemoveLang)
	ON_EN_CHANGE(IDC_EDIT_LANG_NAME, OnChangeEditLangName)
	ON_EN_CHANGE(IDC_EDIT_START, OnChangeEditStart)
	ON_EN_CHANGE(IDC_EDIT_END, OnChangeEditEnd)
	ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_CHECK_CASE_SENSITIVE, OnCheckCaseSensitive)
	ON_EN_CHANGE(IDC_EDIT_EXTENSIONS, OnChangeEditExtensions)
	ON_BN_CLICKED(IDC_CHECK_ALLOW_NESTING, OnCheckAllowNesting)
	ON_BN_CLICKED(IDC_CHECK_WHOLE_WORD, OnCheckWholeWord)
	ON_EN_CHANGE(IDC_EDIT_ESCAPE_CHAR, OnChangeEditEscapeChar)
	ON_WM_GETDLGCODE()
	ON_WM_SHOWWINDOW()
	ON_EN_CHANGE(IDC_EDIT_WORD_LIST, OnChangeEditWordList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// LanguagesPage message handlers

BOOL LanguagesPage::OnInitDialog() 
{
	CDialog::OnInitDialog();

	mCurLang = -1;
	mCurElem = -1;
	mUpdatingElem = false;
	mUpdatingControls = false;

	LoadLangData();
	InitControls();
	UpdateState();

	//find the index of the language with name mInitName
	long initCurLang = mLangData.empty() ? -1 : 0;
	for (size_t i = 0; i < mLangData.size(); ++i)
		if (mLangData[i].mName == mInitLang) {
			initCurLang = i;
			break;
		}

	SetCurLang(initCurLang);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL LanguagesPage::OnApply() {
	if (StoreLangData()) {
		gConfigData.mLangMgr.Save();
		return TRUE;
	} else
		return FALSE;
}

void LanguagesPage::OnSelendokComboElemDef() 
{
	UpdateState();
	UpdateElemText();
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnSelendokComboKind() 
{
	TokenKinds tokenKind = (TokenKinds)mComboKind.GetItemData(mComboKind.GetCurSel());
	bool wholeWord = tokenKind == tkKeyword;
	mCheckWholeWord.SetCheck(wholeWord ? BST_CHECKED : BST_UNCHECKED);

	UpdateState();
	UpdateElemText();
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnCheckSpanLines() 
{
	UpdateState();
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnSelchangeListLang() 
{
	SetCurLang(mListLang.GetCurSel());
}

void LanguagesPage::OnSelchangeListElem() 
{
	SetCurElem(mListElem.GetCurSel());	
}

void LanguagesPage::OnChangeEditElem() 
{
	UpdateState();
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnButtonAddElem() 
{
	SetCurElem(-1);
	LangData& curLang = mLangData[mCurLang];
	LangElemData newElemData(new Words(tkKeyword, WholeWord));
	curLang.mElemData.push_back(newElemData);
	RefreshElemList();
	SetCurElem(curLang.mElemData.size() - 1);
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnButtonAddLang() 
{
	CString langName = TEXT("New Language");

	SetCurLang(-1);
	LangData newLangData(new Language(langName));
	mLangData.push_back(newLangData);
	RefreshLangList();
	SetCurLang(mLangData.size() - 1);
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnButtonRemoveElem() 
{
	int curElem = GetCurElem();
	SetCurElem(-1);
	LangData& curLang = mLangData[mCurLang];
	curLang.mElemData.erase(curLang.mElemData.begin() + curElem);
	RefreshElemList();
	SetCurElem(Min(curElem, (int)curLang.mElemData.size() - 1));	
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnButtonUp() 
{
	int curElem = GetCurElem();
	ATLASSERT(curElem > 0);
	LangData& curLang = mLangData[mCurLang];
	SetCurElem(-1);
	swap(curLang.mElemData[curElem], curLang.mElemData[curElem-1]);
	RefreshElemList();
	SetCurElem(curElem-1);
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnButtonDown() 
{
	int curElem = GetCurElem();
	LangData& curLang = mLangData[mCurLang];
	ATLASSERT(curElem < (int)curLang.mElemData.size() - 1);
	SetCurElem(-1);
	swap(curLang.mElemData[curElem], curLang.mElemData[curElem+1]);
	RefreshElemList();
	SetCurElem(curElem+1);
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnButtonRemoveLang() 
{
	int curLang = GetCurLang();
	SetCurLang(-1);
	mLangData.erase(mLangData.begin() + curLang);
	RefreshLangList();
	SetCurLang(Min(curLang, (int)mLangData.size() - 1));
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnChangeEditLangName() 
{
	CString name;
	mEditLangName.GetWindowText(name);
	mLangData[GetCurLang()].mName = name;
	SetListBoxItemText(mListLang, GetCurLang(), name);
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnChangeEditStart() 
{
	UpdateElemText();
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnChangeEditEnd() 
{
	UpdateElemText();
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnHelp() {
	gConfigData.ExecuteHelp(hfSyntaxColoring, m_hWnd);
}

void LanguagesPage::OnCheckCaseSensitive() 
{
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnChangeEditExtensions() 
{
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnCheckAllowNesting() 
{
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnCheckWholeWord() 
{
	SetModifiedIfNotUpdating();
	UpdateState();
}

void LanguagesPage::OnChangeEditEscapeChar() 
{
	SetModifiedIfNotUpdating();
}

void LanguagesPage::OnChangeEditWordList() 
{
	SetModifiedIfNotUpdating();
}
