#if !defined(AFX_FINDREPLACEDLG_H__BDD44928_23A2_4BD5_B4DF_C50120F74AD0__INCLUDED_)
#define AFX_FINDREPLACEDLG_H__BDD44928_23A2_4BD5_B4DF_C50120F74AD0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FindReplaceDlg.h : header file
//

#include "Utility.h"

extern UINT WM_FINDREPLACE;

/////////////////////////////////////////////////////////////////////////////
// FindReplaceDlg dialog

class FindReplaceDlg : public CDialog
{
// Construction
public:
	FindReplaceDlg(bool find, const CString& initFindText, bool readOnly, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(FindReplaceDlg)
	enum { IDD = IDD_FINDREPLACE_DLG };
	CButton	mCheckUp;
	CButton	mCheckSelectionOnly;
	CButton	mBtnCancel;
	CStatic	mStaticReplaceWith;
	CButton	mCheckEscapedChars;
	CComboBox	mComboReplace;
	CComboBox	mComboFind;
	CButton	mCheckWholeWord;
	CButton	mCheckClose;
	CButton	mCheckCase;
	CButton	mBtnReplaceAll;
	CButton	mBtnReplace;
	CButton	mBtnFind;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(FindReplaceDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL PreTranslateMessage( MSG* pMsg );

// Implementation
public:
	void Show(CWnd* parent);

	// can call when this is NULL
	bool SafeIsVisible() const {
		return ::IsWindow(GetSafeHwnd()) && IsWindowVisible();
	}

	void SetReadOnly(bool readOnly) {
		mReadOnly = readOnly;
		UpdateEnable();
	}

private:
	enum FindKind {fkFind, fkReplace, fkReplaceAll};

	const bool mFind;
	CString mInitFindText;

	VirtualKey mSwapClipKey, mLastClipKey;
	KeyModifiers mSwapClipMods, mLastClipMods;

	bool mReadOnly;

	bool mNewSearch;

	void UpdateEnable();
	void AddStringToRecentStrings(const CString& string, StringVector& vec);
	void UpdateData(FindKind fk);
	void SendFindReplaceMsg();

protected:

	// Generated message map functions
	//{{AFX_MSG(FindReplaceDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonFind();
	afx_msg void OnButtonReplace();
	afx_msg void OnButtonReplaceAll();
	afx_msg void OnEditchangeComboFind();
	afx_msg void OnCheckSelectionOnly();
	afx_msg void OnSelendokComboFind();
	afx_msg void OnSelchangeComboFind();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINDREPLACEDLG_H__BDD44928_23A2_4BD5_B4DF_C50120F74AD0__INCLUDED_)
