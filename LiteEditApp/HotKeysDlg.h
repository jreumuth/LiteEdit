#if !defined(AFX_HOTKEYSDLG_H__5D40F42F_EA63_4707_8ED3_CF427B7A047D__INCLUDED_)
#define AFX_HOTKEYSDLG_H__5D40F42F_EA63_4707_8ED3_CF427B7A047D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HotKeysDlg.h : header file
//

#include "ConfigData.h"

struct HotKeyData {
	UINT cmd;
	VirtualKey key;
	KeyModifiers mods;
	bool isEditable;
	CString fullName;

	HotKeyData(UINT _cmd) : cmd(_cmd), key(0), mods(kmNone), isEditable(false) {}
	Accel AsAccel() const {return Accel(key, mods, cmd);}
};

WORD ToHotKeyMods(KeyModifiers mods);
KeyModifiers FromHotKeyMods(WORD mods);

/////////////////////////////////////////////////////////////////////////////
// HotKeysDlg dialog

class HotKeysDlg : public CDialog
{
// Construction
public:
	HotKeysDlg(HMENU hMenu, CWnd* pParent = NULL);   // standard constructor
	~HotKeysDlg() {mMenu.Detach();}

// Dialog Data
	//{{AFX_DATA(HotKeysDlg)
	enum { IDD = IDD_HOTKEYS_DLG };
	CListBox	mListMenuItems;
	CHotKeyCtrl	mHotKeyNew;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(HotKeysDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	CMenu mMenu;
	vector<HotKeyData> mHotKeyData;
	int mCurItem;

	void InitHotKeyData(const CString& parentName, const CString& indent, CMenu& menu);
	void UpdateEnable();
	bool UpdateCurItem();
	HotKeyData& GetCurData();
	int FindDuplicate(int item) const;
	int CheckForDuplicate(int item, int possibleDup = -1);

protected:

	// Generated message map functions
	//{{AFX_MSG(HotKeysDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListMenuItems();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HOTKEYSDLG_H__5D40F42F_EA63_4707_8ED3_CF427B7A047D__INCLUDED_)
