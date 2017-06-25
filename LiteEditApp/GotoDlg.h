#if !defined(AFX_GOTODLG_H__4A37C95B_8D7C_4007_8238_78E8EDF96E15__INCLUDED_)
#define AFX_GOTODLG_H__4A37C95B_8D7C_4007_8238_78E8EDF96E15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GotoDlg.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CGotoDlg dialog

class CGotoDlg : public CDialog
{
// Construction
public:
	CGotoDlg(long lineCount, CWnd* pParent = NULL);   // standard constructor

	long GetLineNumber() const {return mLineNumber;}

private:
	const long mLineCount;
	long mLineNumber;

	void UpdateEnable();

// Dialog Data
	//{{AFX_DATA(CGotoDlg)
	enum { IDD = IDD_GOTO_DLG };
	CStatic	mLineNumberLabel;
	CButton	mOkBtn;
	CEdit	mEdit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGotoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGotoDlg)
	afx_msg void OnChangeLineEdit();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GOTODLG_H__4A37C95B_8D7C_4007_8238_78E8EDF96E15__INCLUDED_)
