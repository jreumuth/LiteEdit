#if !defined(AFX_OLORSPAGE_H__39BD1751_9C4B_4D65_B62F_2C4F97170C91__INCLUDED_)
#define AFX_OLORSPAGE_H__39BD1751_9C4B_4D65_B62F_2C4F97170C91__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ColorsPage.h : header file
//

#include "EditCtrl.h"
#include "ConfigData.h"

/////////////////////////////////////////////////////////////////////////////
// ColorsPage dialog

class ColorsPage : public CPropertyPage
{
// Construction
public:
	ColorsPage();   // standard constructor

// Dialog Data
	//{{AFX_DATA(ColorsPage)
	enum { IDD = IDD_COLORS_PAGE };
	CEdit	mEditName;
	CListBox	mListSchemes;
	CButton	mUseSystemSelectionColors;
	CButton	mInvertColors;
	CComboBox	mComboItems;
	CStatic	mStaticColor;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ColorsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	void StoreData();

private:
	typedef CPropertyPage Base;

	CBrush mBrushCurColor;
	COLORREF mCurColor;
	ColorSchemes mColorSchemes;
	EditCtrl mEdit;
	bool mUpdatingControls;

	TokenKinds GetCurTokenKind() const {return (TokenKinds)mComboItems.GetItemData(mComboItems.GetCurSel());}
	void InitEditCtrl();
	void UpdateCurScheme();
	void FillSchemeList();
	void UpdateEnable();
	COLORREF GetCurColor() const {return mCurColor;}
	void SetCurColor(COLORREF clr);
	void SetCurScheme(long i, bool alwaysUpdate = false);
	long GetCurScheme();

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(ColorsPage)
	afx_msg void OnButtonPickColor();
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnSelendokComboItems();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnRadioInvertColors();
	afx_msg void OnRadioUseSystemSelectionColors();
	afx_msg void OnHelp();
	afx_msg void OnSelchangeListSchemes();
	afx_msg void OnChangeEditName();
	afx_msg void OnButtonNew();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonMoveUp();
	afx_msg void OnButtonMoveDown();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OLORSPAGE_H__39BD1751_9C4B_4D65_B62F_2C4F97170C91__INCLUDED_)
