#if !defined(AFX_TIMEDATEFORMATDLG_H__0F1547A0_7939_4EF9_B5B4_B6C953679B9B__INCLUDED_)
#define AFX_TIMEDATEFORMATDLG_H__0F1547A0_7939_4EF9_B5B4_B6C953679B9B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TimeDateFormatDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// TimeDateFormatDlg dialog

class TimeDateFormatDlg : public CDialog
{
// Construction
public:
	TimeDateFormatDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(TimeDateFormatDlg)
	enum { IDD = IDD_TIMEDATE_FORMAT_DLG };
	CButton	mButtonMacro;
	CEdit	mEditTimeDate;
	CEdit	mEditTimeDateFormat;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TimeDateFormatDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void ScrollTimeDate();

protected:

	// Generated message map functions
	//{{AFX_MSG(TimeDateFormatDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditTimeDateFormat();
	afx_msg void OnVscrollEditTimeDateFormat();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnMacroButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIMEDATEFORMATDLG_H__0F1547A0_7939_4EF9_B5B4_B6C953679B9B__INCLUDED_)
