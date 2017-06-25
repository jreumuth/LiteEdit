#if !defined(AFX_TOOLSDLG_H__C05BC518_4D88_47E4_9BCA_CE7B3643B6AE__INCLUDED_)
#define AFX_TOOLSDLG_H__C05BC518_4D88_47E4_9BCA_CE7B3643B6AE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ToolsDlg.h : header file
//

#include "ConfigData.h"

/////////////////////////////////////////////////////////////////////////////
// ToolsDlg dialog

class ToolsDlg : public CDialog
{
// Construction
public:
	ToolsDlg(const CString& fileName, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(ToolsDlg)
	enum { IDD = IDD_TOOLS_DLG };
	CButton	mCheckIsSubmenu;
	CEdit	mEditVisibleExts;
	CButton	mBtnInSubMenu;
	CButton	mBtnOutSubMenu;
	CButton	mCheckIsSeparator;
	CButton	mBtnParametersMacro;
	CButton	mBtnInitDirMacro;
	CButton	mBtnCommandMacro;
	CButton	mCheckHideCommandWindow;
	CButton	mCheckIsFilter;
	CHotKeyCtrl	mHotKey;
	CEdit	mEditShowCommand;
	CEdit	mEditMenuItemName;
	CButton	mBtnInsert;
	CListBox	mListMenuItems;
	CEdit	mEditShowParameters;
	CEdit	mEditShowInitDir;
	CEdit	mEditParameters;
	CEdit	mEditInitDir;
	CButton	mCheckSaveFile;
	CEdit	mEditCommand;
	CEdit	mEditExts;
	CButton	mBtnMoveUp;
	CButton	mBtnMoveDown;
	CButton	mBtnDelete;
	CButton	mBtnBrowse;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ToolsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	const CString mFileName;
	Commands mCommands;
	int mCurItem;
	bool mProgrammicChange;

	static CString GetMenuString(long indent, bool isSubmenu, bool isSeparator, const CString& itemName);
	static CString GetMenuString(const Command& command);

	void RefreshList();
	bool SaveCurItem();
	bool SaveExtList(CEdit& editExts, StringVector& extList);
	bool SetCurItem(int item, bool save = true);
	void UpdateEnable();
	long FindConflictingHotkey() const;
	bool ValidateHotkeys();
	void OnMacro(CButton& button, CEdit& edit);
	void UpdateCurListItemText();
	bool HasDuplicateImpl(int index, CString& msg, int count);

protected:

	// Generated message map functions
	//{{AFX_MSG(ToolsDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeListMenuItems();
	afx_msg void OnChangeEditMenuItemName();
	afx_msg void OnButtonBrowse();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonInsert();
	afx_msg void OnButtonMoveDown();
	afx_msg void OnButtonMoveUp();
	afx_msg void OnChangeEditCommand();
	afx_msg void OnChangeEditParameters();
	afx_msg void OnChangeEditInitDir();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonCheckMnemonics();
	afx_msg void OnButtonCheckHotkey();
	afx_msg void OnButtonCommandMacro();
	afx_msg void OnButtonInitDirMacro();
	afx_msg void OnButtonParametersMacro();
	afx_msg void OnCheckIsFilter();
	afx_msg void OnButtonInSubmenu();
	afx_msg void OnButtonOutSubmenu();
	afx_msg void OnCheckIsSeparator();
	afx_msg void OnCheckIsSubmenu();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TOOLSDLG_H__C05BC518_4D88_47E4_9BCA_CE7B3643B6AE__INCLUDED_)
