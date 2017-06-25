#if !defined(AFX_LANGUAGESPAGE_H__3B670953_6FF9_4657_9A2E_13BD0A402C9E__INCLUDED_)
#define AFX_LANGUAGESPAGE_H__3B670953_6FF9_4657_9A2E_13BD0A402C9E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LanguagesPage.h : header file
//

#include "CodeColoring.h"

struct ValidateLangElemData {
	bool valid;
	CString message;

	ValidateLangElemData(const CString& msg) : valid(false), message(msg) {}
	ValidateLangElemData() : valid(true) {}
};

struct ValidateLangData : ValidateLangElemData {
	int elem;

	ValidateLangData(const ValidateLangElemData& data, int _elem) :
		ValidateLangElemData(data), elem(_elem) {}
	ValidateLangData(const CString& msg) :
		ValidateLangElemData(msg), elem(-1) {}
	ValidateLangData() :
		ValidateLangElemData(), elem(-1) {}
};

struct LangElemData {
	LangElemKinds mLangElemKind;
	TokenKinds mTokenKind;
	LangElemOptions mOptions;
	StringVector mStrings;

	LangElemData(LangElemPtr langElem);
	LangElemPtr CreateLangElem() const;
	CString ToString() const;
	ValidateLangElemData Validate(bool matchCase);
};

struct LangData {
	CString mName;
	bool mMatchCase;
	StringVector mExtensions;
	vector<LangElemData> mElemData;
	CString mDir;

	LangData(LanguagePtr lang);
	LanguagePtr CreateLanguage() const;
	ValidateLangData Validate();
};

/////////////////////////////////////////////////////////////////////////////
// LanguagesPage dialog

class LanguagesPage : public CPropertyPage
{
// Construction
public:
	LanguagesPage(const CString& initLang);   // standard constructor

// Dialog Data
	//{{AFX_DATA(LanguagesPage)
	enum { IDD = IDD_LANGUAGES_PAGE };
	CEdit	mEditEscapeChar;
	CStatic	mStaticEscapeChar;
	CButton	mBtnUp;
	CButton	mBtnDown;
	CStatic	mStaticFileExtensions;
	CStatic	mStaticLangName;
	CButton	mStaticLangOptions;
	CButton	mBtnRemoveLang;
	CButton	mBtnAddLang;
	CEdit	mEditLangName;
	CEdit	mEditExtensions;
	CStatic	mStaticKind;
	CStatic	mStaticElemDef;
	CStatic	mStaticStart;
	CStatic	mStaticEnd;
	CStatic	mStaticWordList;
	CButton	mStaticElem;
	CButton	mBtnRemoveElem;
	CButton	mBtnAddElem;
	CEdit	mEditWordList;
	CComboBox	mComboElemDef;
	CListBox	mListLang;
	CListBox	mListElem;
	CEdit	mEditStart;
	CEdit	mEditEnd;
	CComboBox	mComboKind;
	CButton	mCheckSpanLines;
	CButton	mCheckWholeWord;
	CButton	mCheckCaseSensitive;
	CButton	mCheckAllowNesting;
	//}}AFX_DATA

public:
	bool StoreLangData();

private:
	typedef CPropertyPage Base;

	vector<LangData> mLangData;
	int mCurLang;
	int mCurElem;
	bool mUpdatingElem;
	bool mUpdatingControls;
	CString mInitLang;

	void LoadLangData();
	void InitControls();
	void UpdateState();
	void RefreshLangList();
	void RefreshElemList();
	void UpdateElemText();
	int GetCurLang() const {return mCurLang;}
	void SetCurLang(int i);
	int GetCurElem() const {return mCurElem;}
	void SetCurElem(int i);
	void UpdateCurData();
	bool ValidateAllData();
	void SetModifiedIfNotUpdating();

	LangElemKinds GetLangElemKind() const {return (LangElemKinds)mComboElemDef.GetItemData(mComboElemDef.GetCurSel());}
	TokenKinds GetTokenKind() const {return (TokenKinds)mComboKind.GetItemData(mComboKind.GetCurSel());}
	LangElemOptions GetOptions() const;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(LanguagesPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(LanguagesPage)
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnSelendokComboElemDef();
	afx_msg void OnSelendokComboKind();
	afx_msg void OnCheckSpanLines();
	afx_msg void OnSelchangeListLang();
	afx_msg void OnSelchangeListElem();
	afx_msg void OnChangeEditLang();
	afx_msg void OnChangeEditElem();
	afx_msg void OnButtonAddElem();
	afx_msg void OnButtonAddLang();
	afx_msg void OnButtonRemoveElem();
	afx_msg void OnButtonRemoveLang();
	afx_msg void OnChangeEditLangName();
	afx_msg void OnChangeEditStart();
	afx_msg void OnChangeEditEnd();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnHelp();
	afx_msg void OnCheckCaseSensitive();
	afx_msg void OnChangeEditExtensions();
	afx_msg void OnCheckAllowNesting();
	afx_msg void OnCheckWholeWord();
	afx_msg void OnChangeEditEscapeChar();
	afx_msg void OnChangeEditWordList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LANGUAGESPAGE_H__3B670953_6FF9_4657_9A2E_13BD0A402C9E__INCLUDED_)
