#if !defined(AFX_EDITOROPTIONSDLG_H__0BF3A21A_D803_4146_90AE_3557C7A698CB__INCLUDED_)
#define AFX_EDITOROPTIONSDLG_H__0BF3A21A_D803_4146_90AE_3557C7A698CB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditorOptionsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// EditorOptionsDlg dialog

class EditorOptionsDlg : public CDialog
{
// Construction
public:
	EditorOptionsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(EditorOptionsDlg)
	enum { IDD = IDD_EDITOR_OPTIONS_DLG };
	CComboBox	mComboEncoding;
	CComboBox	mComboNewLineType;
	CButton	mCheckTabInsertsSpaces;
	CButton	mCheckShowWhitespace;
	CButton	mCheckShowLineNumbers;
	CEdit	mEditDefaultExt;
	CEdit	mEditTabWidth;
	CEdit	mEditMaxRecent;
	CButton	mCheckRemoveTrailingWhiteSpace;
	CButton	mCheckAutoIndent;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(EditorOptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	bool Validate();

protected:

	// Generated message map functions
	//{{AFX_MSG(EditorOptionsDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITOROPTIONSDLG_H__0BF3A21A_D803_4146_90AE_3557C7A698CB__INCLUDED_)
