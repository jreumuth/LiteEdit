#if !defined(AFX_PROMPTDLG_H__37102380_A836_4F5E_9DF4_483A78E3EC41__INCLUDED_)
#define AFX_PROMPTDLG_H__37102380_A836_4F5E_9DF4_483A78E3EC41__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PromptDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PromptDlg dialog

class PromptDlg : public CDialog
{
// Construction
public:
	PromptDlg(const CString& promptStr, const CString& title, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(PromptDlg)
	enum { IDD = IDD_PROMPT_DLG };
	CButton	mOk;
	CStatic	mStaticPrompt;
	CEdit	mEdit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PromptDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	CString mText;

private:
	CString mPromptStr, mTitle;

protected:

	// Generated message map functions
	//{{AFX_MSG(PromptDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROMPTDLG_H__37102380_A836_4F5E_9DF4_483A78E3EC41__INCLUDED_)
