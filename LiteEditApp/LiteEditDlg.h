// LiteEditDlg.h : header file
//

#if !defined(AFX_LITEEDITDLG_H__B0F985F6_15AC_4766_A519_8F6D842BCCE2__INCLUDED_)
#define AFX_LITEEDITDLG_H__B0F985F6_15AC_4766_A519_8F6D842BCCE2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EditCtrl.h"
#include <memory>
#include "ConfigData.h"
#include "FindReplaceDlg.h"

struct FileInfo {
	bool exists;
	FILETIME modifyTime;
	FileFormat fileFormatOnDisk, fileFormat;
	NewLineType newLineTypeOnDisk, newLineType;

	FileInfo() :
		exists(false),
		// can't initialize fileFormat with gConfigData.mFileFormat because
		// member variables of CLiteEditDlg are initialized before gConfigData,
		// but the initial value of fileFormat doesn't matter anyway
		fileFormatOnDisk(ffAscii),
		fileFormat(ffAscii),
		newLineTypeOnDisk(nlDefault),
		newLineType(nlDefault)
	{
		MemClear(modifyTime);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CLiteEditDlg dialog

class CLiteEditDlg : public CDialog
{
// Construction
public:
	CLiteEditDlg(const LiteEditCommandLineInfo& cmdLineInfo, CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CLiteEditDlg)
	enum { IDD = IDD_MAIN_DLG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLiteEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL PreTranslateMessage( MSG* pMsg );

private:
	typedef CDialog CBase;
	const LiteEditCommandLineInfo& mCmdLineInfo;
	EditCtrl mEdit;
	CStatusBarCtrl mStatusBar;
	bool mHasModifiedMarker;
	HWND mNextClipViewer;
	CString mFileName;
	FileInfo mFileInfo;
	CommandIdCreator mIdCreator;
	auto_ptr<FindReplaceDlg> mFindRelaceDlgPtr;
	//used to know when the user has searched through all the text
	long mInitFindPos, mLastFindPos;
	bool mRepeatingSearch;
	SearchData mLastSearchData;
	bool mPanePosValid;
	bool mIsMaximized;
	CString mCurrentClipboard;
	bool mCheckExternalModify;

	void DisplayErrorMessage(const CString& msg) {
		MessageBox(msg, ErrorStr, MB_OK|MB_ICONERROR);
	}

	CString GetDefaultExt() const {return gConfigData.mDefExt;}
	void ShowUsage() const;
	void ProcessCommandLine();
	void LoadFile(const CString& fileName, bool newFile);
	bool SaveFileAs(const CString& fileName, FileFormat fileFormat);
	void SetFileInfo(const CString& name, bool exists, FileFormat fileFormat, NewLineType newLineType);
	void RemoveTrailingWhiteSpace();
	void SetLangFromFileExt();
	void NewFile();
	CString GetNewFileName() const;
	bool HasNewFileName() const;
	void OpenFile();
	bool IsFileModified() const;
	bool PromptToSave();
	bool CanSave() const;
	bool Save();
	bool SaveAs();
	void Print();
	const CString& GetFileName() const {return mFileName;}
	void UpdateModified();
	void UpdateTitle();
	void DecorateAllMenus(bool rebuildToolsMenu);
	void ResizeControls();
	void UpdatePosPane();
	void UpdateClipboardPane(bool clipboardChanged);
	void InsertTimeDate();
	enum CaseKind {ckLower, ckUpper, ckTitle, ckToggle};
	void ChangeSelCase(CaseKind kind);
	bool ProcessDynamicCommand(UINT id);
	void ExecuteCommand(const Command& cmd);
	void ShowFindOrReplaceDialog(bool find);
	void GetFindReplaceDlgOutOfTheWay();
	void DoFindOrReplace();
	void DoFindNextOrPrev(bool next);
	void RebuildToolsMenu();
	void DoGoto();
	void DisplayToolsDlg();
	void DisplayFontDlg();
	void DisplayEditorOptionsDlg();
	void DisplaySyntaxColoringDlg();
	void DisplayHotKeysDlg();
	void ToggleWordWrap();

// Implementation
protected:
	HICON mSmallIcon, mLargeIcon;

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CLiteEditDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	afx_msg void OnDrawClipboard();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnExitSizeMove(WPARAM wParam, LPARAM lParam);
	afx_msg void OnClose();
	afx_msg void OnInitMenu(CMenu* menu);
	afx_msg void OnEditChange();
	afx_msg void OnEditSelChange();
	afx_msg LPARAM OnFindReplace(WPARAM wParam, LPARAM lParam);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnHelp();
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg LRESULT OnKickIdle(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LITEEDITDLG_H__B0F985F6_15AC_4766_A519_8F6D842BCCE2__INCLUDED_)
