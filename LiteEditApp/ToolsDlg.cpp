// ToolsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "liteedit.h"
#include "ToolsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ToolsDlg dialog


ToolsDlg::ToolsDlg(const CString& fileName, CWnd* pParent /*=NULL*/)
	: CDialog(ToolsDlg::IDD, pParent), mFileName(fileName)
{
	//{{AFX_DATA_INIT(ToolsDlg)
	//}}AFX_DATA_INIT
}

void ToolsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ToolsDlg)
	DDX_Control(pDX, IDC_CHECK_IS_SUBMENU, mCheckIsSubmenu);
	DDX_Control(pDX, IDC_EDIT_VISIBLE_EXTS, mEditVisibleExts);
	DDX_Control(pDX, IDC_BUTTON_IN_SUBMENU, mBtnInSubMenu);
	DDX_Control(pDX, IDC_BUTTON_OUT_SUBMENU, mBtnOutSubMenu);
	DDX_Control(pDX, IDC_CHECK_IS_SEPARATOR, mCheckIsSeparator);
	DDX_Control(pDX, IDC_BUTTON_PARAMETERS_MACRO, mBtnParametersMacro);
	DDX_Control(pDX, IDC_BUTTON_INIT_DIR_MACRO, mBtnInitDirMacro);
	DDX_Control(pDX, IDC_BUTTON_COMMAND_MACRO, mBtnCommandMacro);
	DDX_Control(pDX, IDC_CHECK_HIDE_COMMAND_WINDOW, mCheckHideCommandWindow);
	DDX_Control(pDX, IDC_CHECK_IS_FILTER, mCheckIsFilter);
	DDX_Control(pDX, IDC_HOTKEY, mHotKey);
	DDX_Control(pDX, IDC_EDIT_SHOW_COMMAND, mEditShowCommand);
	DDX_Control(pDX, IDC_EDIT_MENU_ITEM_NAME, mEditMenuItemName);
	DDX_Control(pDX, IDC_BUTTON_INSERT, mBtnInsert);
	DDX_Control(pDX, IDC_LIST_MENU_ITEMS, mListMenuItems);
	DDX_Control(pDX, IDC_EDIT_SHOW_PARAMETERS, mEditShowParameters);
	DDX_Control(pDX, IDC_EDIT_SHOW_INIT_DIR, mEditShowInitDir);
	DDX_Control(pDX, IDC_EDIT_PARAMETERS, mEditParameters);
	DDX_Control(pDX, IDC_EDIT_INIT_DIR, mEditInitDir);
	DDX_Control(pDX, IDC_CHECK_SAVE_FILE, mCheckSaveFile);
	DDX_Control(pDX, IDC_EDIT_COMMAND, mEditCommand);
	DDX_Control(pDX, IDC_EDIT_EXTS, mEditExts);
	DDX_Control(pDX, IDC_BUTTON_MOVE_UP, mBtnMoveUp);
	DDX_Control(pDX, IDC_BUTTON_MOVE_DOWN, mBtnMoveDown);
	DDX_Control(pDX, IDC_BUTTON_DELETE, mBtnDelete);
	DDX_Control(pDX, IDC_BUTTON_BROWSE, mBtnBrowse);
	//}}AFX_DATA_MAP
}

CString ToolsDlg::GetMenuString(long indent, bool isSubmenu, bool isSeparator, const CString& itemName) {
	CString string = DupChar('\t', indent);

	if (isSeparator)
		string += _T("-----------------------------");
	else {
		string += itemName;

		if (isSubmenu)
			string += _T(" >>");

	}

	return string;
}

CString ToolsDlg::GetMenuString(const Command& command) {
	return GetMenuString(command.mIndent, command.mIsSubMenu, command.mIsSeparator, command.mMenuItemName);
}

void ToolsDlg::UpdateCurListItemText()
{
	if (!mProgrammicChange) {
		CString name;
		mEditMenuItemName.GetWindowText(name);

		// mCommands[X].mIndent is always kept up to date with control, but separator,
		// submenu, and menu item name aren't and must come from controls
		bool isSubmenu = mCheckIsSubmenu.GetCheck() == BST_CHECKED;
		bool isSeparator = mCheckIsSeparator.GetCheck() == BST_CHECKED;
		name = GetMenuString(mCommands[mCurItem].mIndent, isSubmenu, isSeparator, name);

		SetListBoxItemText(mListMenuItems, mCurItem, name);
	}
}

void ToolsDlg::RefreshList() {
	int curItem = mCurItem;
	SetCurItem(-1);

	mListMenuItems.ResetContent();
	for (size_t i = 0; i < mCommands.size(); ++i)
		mListMenuItems.AddString(GetMenuString(mCommands[i]));

	curItem = Min(mListMenuItems.GetCount() - 1, curItem);
	SetCurItem(curItem);
}

bool ToolsDlg::SaveCurItem() {
	//save control's data in mCommands
	if (mCurItem >= 0) {
		Command& cmd = mCommands[mCurItem];

		if (!SaveExtList(mEditExts, cmd.mEnabledExtensions))
			return false;

		if (!SaveExtList(mEditVisibleExts, cmd.mVisibleExtensions))
			return false;

		mEditMenuItemName.GetWindowText(cmd.mMenuItemName);
		mEditCommand.GetWindowText(cmd.mCommand);
		mEditParameters.GetWindowText(cmd.mArguments);
		mEditInitDir.GetWindowText(cmd.mInitDir);

		cmd.mIsFilter = mCheckIsFilter.GetCheck() == BST_CHECKED;
		cmd.mSaveFile = mCheckSaveFile.GetCheck() == BST_CHECKED;
		cmd.mHideCommandWindow = mCheckHideCommandWindow.GetCheck() == BST_CHECKED;
		cmd.mIsSubMenu = mCheckIsSubmenu.GetCheck() == BST_CHECKED;
		cmd.mIsSeparator = mCheckIsSeparator.GetCheck() == BST_CHECKED;

		WORD vkey = 0, mods = 0;
		mHotKey.GetHotKey(vkey, mods);
		//a command ID will be created later when the commands are added to a menu
		cmd.mAccel = Accel(vkey, FromHotKeyMods(mods), 0);

		if (cmd.mCommand.IsEmpty() && !cmd.mIsSubMenu && !cmd.mIsSeparator) {
			MessageBox(TEXT("The command cannot be empty."), ErrorStr, MB_OK|MB_ICONERROR);
			mEditCommand.SetFocus();
			return false;
		}
	}

	return true;
}

bool ToolsDlg::SaveExtList(CEdit& editExts, StringVector& extList) {
	CString exts;
	editExts.GetWindowText(exts);
	StringListToStringVector(exts, TEXT(";"), extList);

	for (size_t i = 0; i < extList.size(); ++i) {
		CString& ext = extList[i];
		ext.TrimLeft();
		ext.TrimRight();
		ext.TrimLeft('.');
		if (!IsValidFileExt(ext)) {
			CString message = GetInvalidExtErrorMessage(ext);
			MessageBox(message, ErrorStr, MB_OK|MB_ICONERROR);
			editExts.SetFocus();
			return false;
		}
	}

	return true;
}

bool ToolsDlg::SetCurItem(int item, bool save/* = true*/) {
	ASSERT(-1 <= item && item < (int)mCommands.size());

	if (mCurItem != item) {
		if (save && !SaveCurItem()) {
			//make sure selection doesn't change
			mListMenuItems.SetCurSel(mCurItem);
			return false;
		}

		mCurItem = item;

		//load data from mCommands to the controls
		if (mCurItem >= 0) {
			mProgrammicChange = true;
			const Command& cmd = mCommands[mCurItem];

			CString exts = cmd.mEnabledExtensions.Concat(TEXT("; "));
			mEditExts.SetWindowText(exts);

			exts = cmd.mVisibleExtensions.Concat(TEXT("; "));
			mEditVisibleExts.SetWindowText(exts);

			mEditMenuItemName.SetWindowText(cmd.mMenuItemName);
			mEditCommand.SetWindowText(cmd.mCommand);
			mEditParameters.SetWindowText(cmd.mArguments);
			mEditInitDir.SetWindowText(cmd.mInitDir);

			mCheckIsFilter.SetCheck(cmd.mIsFilter ? BST_CHECKED : BST_UNCHECKED);
			mCheckSaveFile.SetCheck(cmd.mSaveFile ? BST_CHECKED : BST_UNCHECKED);
			mCheckHideCommandWindow.SetCheck(cmd.mHideCommandWindow ? BST_CHECKED : BST_UNCHECKED);
			mCheckIsSubmenu.SetCheck(cmd.mIsSubMenu ? BST_CHECKED : BST_UNCHECKED);
			mCheckIsSeparator.SetCheck(cmd.mIsSeparator ? BST_CHECKED : BST_UNCHECKED);

			WORD mods = ToHotKeyMods(cmd.mAccel.GetModifiers());
			mHotKey.SetHotKey(cmd.mAccel.GetVirtualKey(), mods);

			mProgrammicChange = false;
		}

		mListMenuItems.SetCurSel(mCurItem);
		UpdateEnable();
	}

	return true;
}

void ToolsDlg::UpdateEnable() {
	//badUpdateEnableState happens when a message handler temporarely sets
	//the current item to -1 to keep the current command from being saved
	//while the current command may be in a bad state.
	bool badUpdateEnableState = mCurItem < 0 && !mCommands.empty();

	if (!badUpdateEnableState) {
		CWnd* focus = GetFocus();

		bool hasItem = mCurItem >= 0;

		//change enabled state of all windows except the menu list,
		//ok button, and cancel button

		CWnd* wnd = GetWindow(GW_CHILD);
		while (wnd != NULL) {
			int id = wnd->GetDlgCtrlID();

			if (id != IDC_LIST_MENU_ITEMS &&
				id != IDC_BUTTON_INSERT &&
				id != IDOK &&
				id != IDCANCEL)
				wnd->EnableWindow(hasItem);

			wnd = wnd->GetWindow(GW_HWNDNEXT);
		}

		if (hasItem) {
			mBtnMoveUp.EnableWindow(mCommands.GetPrevMenuAtSameLevel(mCurItem) >= 0);
			mBtnMoveDown.EnableWindow(mCommands.GetNextMenuAtSameLevel(mCurItem) >= 0);

			bool isSubmenuChecked = mCheckIsSubmenu.GetCheck() == BST_CHECKED;
			bool isSeparatorChecked = mCheckIsSeparator.GetCheck() == BST_CHECKED;

			if (isSeparatorChecked)
				mCheckIsSubmenu.EnableWindow(false);
			else if (isSubmenuChecked) // else if so we can't possibly disable both which would leave you unable to reenable either
				mCheckIsSeparator.EnableWindow(false);

			// Can't uncheck Is Submenu when items are inside the submenu.
			// I could leave enabled and display and error when unchecked,
			// but this is easier and probably good enough.
			if (isSubmenuChecked &&
				mCurItem < mCommands.size() - 1 &&
				mCommands[mCurItem].mIndent < mCommands[mCurItem + 1].mIndent)
				mCheckIsSubmenu.EnableWindow(false);

			if (isSeparatorChecked) {
				GetDlgItem(IDC_STATIC_NAME)->EnableWindow(false);
				GetDlgItem(IDC_EDIT_MENU_ITEM_NAME)->EnableWindow(false);
				GetDlgItem(IDC_STATIC_VISIBLE_EXTS)->EnableWindow(false);
				GetDlgItem(IDC_EDIT_VISIBLE_EXTS)->EnableWindow(false);
			}

			if (isSubmenuChecked || isSeparatorChecked) {
				GetDlgItem(IDC_STATIC_ENABLED_EXTS)->EnableWindow(false);
				GetDlgItem(IDC_EDIT_EXTS)->EnableWindow(false);
				GetDlgItem(IDC_STATIC_MACRO_HELP)->EnableWindow(false);
				GetDlgItem(IDC_STATIC_COMMAND)->EnableWindow(false);
				GetDlgItem(IDC_EDIT_COMMAND)->EnableWindow(false);
				GetDlgItem(IDC_BUTTON_COMMAND_MACRO)->EnableWindow(false);
				GetDlgItem(IDC_BUTTON_BROWSE)->EnableWindow(false);
				GetDlgItem(IDC_EDIT_SHOW_COMMAND)->EnableWindow(false);
				GetDlgItem(IDC_STATIC_PARAMETERS)->EnableWindow(false);
				GetDlgItem(IDC_EDIT_PARAMETERS)->EnableWindow(false);
				GetDlgItem(IDC_BUTTON_PARAMETERS_MACRO)->EnableWindow(false);
				GetDlgItem(IDC_EDIT_SHOW_PARAMETERS)->EnableWindow(false);
				GetDlgItem(IDC_STATIC_INIT_DIR)->EnableWindow(false);
				GetDlgItem(IDC_EDIT_INIT_DIR)->EnableWindow(false);
				GetDlgItem(IDC_BUTTON_INIT_DIR_MACRO)->EnableWindow(false);
				GetDlgItem(IDC_EDIT_SHOW_INIT_DIR)->EnableWindow(false);
				GetDlgItem(IDC_CHECK_IS_FILTER)->EnableWindow(false);
				GetDlgItem(IDC_CHECK_SAVE_FILE)->EnableWindow(false);
				GetDlgItem(IDC_CHECK_HIDE_COMMAND_WINDOW)->EnableWindow(false);
				GetDlgItem(IDC_STATIC_HOTKEY)->EnableWindow(false);
				GetDlgItem(IDC_HOTKEY)->EnableWindow(false);
				GetDlgItem(IDC_BUTTON_CHECK_HOTKEY)->EnableWindow(false);
			}

			// can't indent without a submenu to indent into,
			// and can't indent item more than one level inside parent
			bool canIndent = false;
			if (mCurItem > 0)
				if (mCommands[mCurItem-1].mIsSubMenu)
					canIndent = mCommands[mCurItem-1].mIndent >= mCommands[mCurItem].mIndent;
				else
					canIndent = mCommands[mCurItem-1].mIndent > mCommands[mCurItem].mIndent;

			if (!canIndent)
				mBtnInSubMenu.EnableWindow(false);

			// allow outdent if indented and it won't create an invalid indention state
			// such as the next item being indented inside this item without this item
			// being a submenu
			bool canOutdent =
				mCommands[mCurItem].IsIdented() &&
				(isSubmenuChecked || // can always outdent to top level
				 mCurItem == (int)mCommands.size() - 1 || // last item
				 mCommands[mCurItem+1].mIndent < mCommands[mCurItem].mIndent // last item in submenu
				);

			if (!canOutdent)
				mBtnOutSubMenu.EnableWindow(false);

			// disable delete if it's on a non-empty submenu
			if (isSubmenuChecked)
				if (mCurItem < (int)mCommands.size() - 1 && // if is a next item
					mCommands[mCurItem+1].mIndent > mCommands[mCurItem].mIndent) // if submenu is non-empty
					GetDlgItem(IDC_BUTTON_DELETE)->EnableWindow(false);
		}

		GetDlgItem(IDC_BUTTON_CHECK_MNEMONICS)->EnableWindow(mCommands.size() >= 2);

		// If the window with focus is disabled, it's not possible to tab to another
		// control. I explicitly set focus to the list box to work around this.
		// This is likely a bug in MFC.
		if (focus != NULL && !focus->IsWindowEnabled())
			mListMenuItems.SetFocus();
	}
}

long ToolsDlg::FindConflictingHotkey() const {
	for (size_t i1 = 0; i1 < mCommands.size(); ++i1) {
		const Accel& a1 = mCommands[i1].mAccel;
		if (a1.GetVirtualKey() != 0) {
			UINT cmdId = gConfigData.mAccelTable.FindHotKey(a1.GetVirtualKey(), a1.GetModifiers());
			if (cmdId != 0)
				return i1;

			for (size_t i2 = i1 + 1; i2 < mCommands.size(); ++i2) {
				const Accel& a2 = mCommands[i2].mAccel;
				if (a1.GetVirtualKey() == a2.GetVirtualKey() &&
					a1.GetModifiers() == a2.GetModifiers())
					return i1;
			}
		}
	}

	return -1;
}

//This is called when you hit OK. Returns true if the hotkeys are valid.
bool ToolsDlg::ValidateHotkeys() {
	long i = FindConflictingHotkey();
	if (i >= 0) {
		CString msg = TEXT("Lite Edit found conflicting hotkeys. For more information click the 'Check Hotkey for Conflicts' button after closing this dialog box.\n\nDo you want to fix the conflict?");
		if (MessageBox(msg, TEXT("Hotkey Conflicts Found"), MB_YESNO|MB_ICONWARNING) == IDYES) {
			SetCurItem(i);
			return false;
		} else
			return true;
	} else
		return true;
}

BEGIN_MESSAGE_MAP(ToolsDlg, CDialog)
	//{{AFX_MSG_MAP(ToolsDlg)
	ON_LBN_SELCHANGE(IDC_LIST_MENU_ITEMS, OnSelchangeListMenuItems)
	ON_EN_CHANGE(IDC_EDIT_MENU_ITEM_NAME, OnChangeEditMenuItemName)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_INSERT, OnButtonInsert)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_DOWN, OnButtonMoveDown)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_UP, OnButtonMoveUp)
	ON_EN_CHANGE(IDC_EDIT_COMMAND, OnChangeEditCommand)
	ON_EN_CHANGE(IDC_EDIT_PARAMETERS, OnChangeEditParameters)
	ON_EN_CHANGE(IDC_EDIT_INIT_DIR, OnChangeEditInitDir)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_CHECK_MNEMONICS, OnButtonCheckMnemonics)
	ON_BN_CLICKED(IDC_BUTTON_CHECK_HOTKEY, OnButtonCheckHotkey)
	ON_BN_CLICKED(IDC_BUTTON_COMMAND_MACRO, OnButtonCommandMacro)
	ON_BN_CLICKED(IDC_BUTTON_INIT_DIR_MACRO, OnButtonInitDirMacro)
	ON_BN_CLICKED(IDC_BUTTON_PARAMETERS_MACRO, OnButtonParametersMacro)
	ON_BN_CLICKED(IDC_CHECK_IS_FILTER, OnCheckIsFilter)
	ON_BN_CLICKED(IDC_BUTTON_IN_SUBMENU, OnButtonInSubmenu)
	ON_BN_CLICKED(IDC_BUTTON_OUT_SUBMENU, OnButtonOutSubmenu)
	ON_BN_CLICKED(IDC_CHECK_IS_SEPARATOR, OnCheckIsSeparator)
	ON_BN_CLICKED(IDC_CHECK_IS_SUBMENU, OnCheckIsSubmenu)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ToolsDlg message handlers

BOOL ToolsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	mListMenuItems.SetTabStops(10);

	mBtnCommandMacro.SetIcon(gConfigData.mMacroIcon);
	mBtnInitDirMacro.SetIcon(gConfigData.mMacroIcon);
	mBtnParametersMacro.SetIcon(gConfigData.mMacroIcon);

	mCurItem = -1;	
	mProgrammicChange = false;
	mCommands = gConfigData.mCommands;
	mCommands.Flatten();
	RefreshList();

	if (!mCommands.empty())
		SetCurItem(0);
	
	UpdateEnable();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void ToolsDlg::OnOK() 
{
	if (SaveCurItem() && ValidateHotkeys()) {
		gConfigData.mCommands = mCommands;
		gConfigData.mCommands.Unflatten();
		gConfigData.Save();
		CDialog::OnOK();
	}
}

void ToolsDlg::OnSelchangeListMenuItems() 
{
	SetCurItem(mListMenuItems.GetCurSel());
}

void ToolsDlg::OnChangeEditMenuItemName() 
{
	UpdateCurListItemText();
}

void ToolsDlg::OnButtonBrowse() 
{
	CString filter = TEXT("Executable Files (exe; com; bat; cmd; lnk; pif)|*.exe; *.com; *.bat; *.cmd; *.lnk; *.pif|") + AllFilesFilter + '|';
	CFileDialog dlg(TRUE, TEXT("exe"), NULL, OFN_HIDEREADONLY|OFN_DONTADDTORECENT, filter, this);
	if (dlg.DoModal() == IDOK)
		mEditCommand.SetWindowText(dlg.GetPathName());
}

void ToolsDlg::OnButtonDelete() 
{
	ASSERT(0 <= mCurItem && mCurItem < (int)mCommands.size());
	int i = mCurItem;
	if (SetCurItem(-1, false)) {
		mCommands.erase(mCommands.begin() + i);
		RefreshList();
		i = Min((int)mCommands.size() - 1, i);
		SetCurItem(i);

		//SetCurItem() didn't call UpdateEnable() if i is -1 because mCurItem didn't
		//change
		if (mCurItem == -1)
			UpdateEnable();
	}
}

void ToolsDlg::OnButtonInsert() 
{
	if (SetCurItem(-1)) {
		mCommands.push_back(Command());	
		RefreshList();
		SetCurItem((int)mCommands.size() - 1);
	}
}

void ToolsDlg::OnButtonMoveDown() 
{
	ASSERT(0 <= mCurItem && mCurItem < (int)mCommands.size() - 1);
	int curItem = mCurItem;
	if (SetCurItem(-1)) {
		size_t newStart = mCommands.GetNextMenuIndex(mCommands.GetNextMenuAtSameLevel(curItem));
		size_t end = mCommands.GetNextMenuIndex(curItem);
		size_t numItems = end - curItem;
		ASSERT(end > curItem);
		ASSERT(newStart >= end);
		newStart -= numItems;

		vector<Command> toMove;
		toMove.insert(toMove.begin(), mCommands.begin() + curItem, mCommands.begin() + end);
		mCommands.erase(mCommands.begin() + curItem, mCommands.begin() + end);
		mCommands.insert(mCommands.begin() + newStart, toMove.begin(), toMove.end());

		RefreshList();
		SetCurItem(newStart);
	}
}

void ToolsDlg::OnButtonMoveUp() 
{
	ASSERT(1 <= mCurItem && mCurItem < (int)mCommands.size());
	int curItem = mCurItem;
	if (SetCurItem(-1)) {
		size_t newStart = mCommands.GetPrevMenuAtSameLevel(curItem);
		size_t end = mCommands.GetNextMenuIndex(curItem);
		ASSERT(newStart < curItem);
		ASSERT(curItem < end);

		vector<Command> toMove;
		toMove.insert(toMove.begin(), mCommands.begin() + curItem, mCommands.begin() + end);
		mCommands.erase(mCommands.begin() + curItem, mCommands.begin() + end);
		mCommands.insert(mCommands.begin() + newStart, toMove.begin(), toMove.end());

		RefreshList();
		SetCurItem(newStart);
	}
}

void ToolsDlg::OnChangeEditCommand() 
{
	CString string;
	mEditCommand.GetWindowText(string);
	string = GetCommandString(mFileName, string);
	SearchForFile(string);
	mEditShowCommand.SetWindowText(string);
}

void ToolsDlg::OnChangeEditParameters() 
{
	CString string;
	mEditParameters.GetWindowText(string);
	string = GetCommandString(mFileName, string);
	mEditShowParameters.SetWindowText(string);
}

void ToolsDlg::OnChangeEditInitDir() 
{
	CString string;
	mEditInitDir.GetWindowText(string);
	string = GetCommandString(mFileName, string);
	mEditShowInitDir.SetWindowText(string);
}

BOOL ToolsDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	OnButtonHelp();
	return TRUE;
}

void ToolsDlg::OnButtonHelp() 
{
	gConfigData.ExecuteHelp(hfTools, m_hWnd);
}

bool ToolsDlg::HasDuplicateImpl(int index, CString& msg, int count) {
	for (int i1 = index; i1 < count; ++i1) {
		const TCHAR EDIT_TOOLS_MNEMONIC = 'E';
		CString s1;
		mListMenuItems.GetText(i1, s1);
		TCHAR m1 = GetMnemonic(s1);
		long indent1 = mCommands[i1].mIndent;

		if (m1 != NULL) {
			//special check for conflict with Edit Tools
			if (indent1 == 0 && m1 == EDIT_TOOLS_MNEMONIC) {
				CString s2 = TEXT("&Edit Tools");
				msg.Format(TEXT("Duplicate mnemonic '%c' found in the following menu items:\n%s\n%s\n\nNote that %s adds the %s menu item to the Tools menu automatically, and you cannot modify the mnemonic for the %s menu item."), m1, s1, s2, ProgramName, s2, s2);
				return true;
			}

			//check if m1 conflicts with the mnemonic for any menu item after it
			//that's in the same menu
			int i2 = i1;
			while (true) {
				i2 = mCommands.GetNextMenuAtSameLevel(i2);
				if (i2 < 0 || i2 == count) // if reached end of submenu
					break;

				CString s2;
				mListMenuItems.GetText(i2, s2);

				TCHAR m2 = GetMnemonic(s2);

				if (m1 == m2) {
					msg.Format(TEXT("Duplicate mnemonic '%c' found in the following menu items:\n%s\n%s"), m1, s1, s2);
					return true;
				}
			}
		}
	}

	return false;
}

void ToolsDlg::OnButtonCheckMnemonics() 
{
	LPCTSTR title = TEXT("Check Mnemonics");
	int count = mListMenuItems.GetCount();
	CString msg;
	bool hasDuplicate = HasDuplicateImpl(0, msg, count);

	if (hasDuplicate)
		MessageBox(msg, title, MB_OK|MB_ICONWARNING);
	else
		MessageBox(TEXT("There are no duplicate mnemonics."), title, MB_OK|MB_ICONINFORMATION);
}

void ToolsDlg::OnButtonCheckHotkey() 
{
	LPCTSTR title = TEXT("Check Hotkey for Conflicts");

	WORD vk = 0, m = 0;
	mHotKey.GetHotKey(vk, m);
	VirtualKey key = vk;
	KeyModifiers mods = FromHotKeyMods(m);

	if (key != 0) {
		size_t i;
		for (i = 0; i < mCommands.size(); ++i)
			if (i != mCurItem)
				if (mCommands[i].mAccel.GetVirtualKey() == key && mCommands[i].mAccel.GetModifiers() == mods) {
					CString name = mCommands[i].mMenuItemName;
					CleanupMenuItemName(name);
					CString msg;
					msg.Format(TEXT("This hotkey conflicts with the hotkey for %s."), name);
					MessageBox(msg, title, MB_OK|MB_ICONWARNING);
					return;
				}

		UINT cmd = gConfigData.mAccelTable.FindHotKey(key, mods);
		if (cmd != 0) {
			LPCTSTR msg = TEXT("This hotkey conflicts with the hotkey for a menu item that is not in the Tools menu. You may set this hotkey and resolve the conflict in the Options>>Hotkeys dialog box or you may choose another hotkey.");
			MessageBox(msg, title, MB_OK|MB_ICONWARNING);
			return;
		}
	}

	MessageBox(TEXT("This hotkey has no conflicts."), title, MB_OK|MB_ICONINFORMATION);
}

enum MacroId {
	midNone = 0, midFilePath, midFileDir, midFileName, midFileNameWithExt, midFileExt,
	midCurDir, midLineNum, midColNum, midSelText, midSelStart, midSelEnd, midPrompt};

void ToolsDlg::OnMacro(CButton& button, CEdit& edit) {
	CMenu menu;
	VERIFY(menu.CreatePopupMenu());
	VERIFY(menu.AppendMenu(MF_STRING, midFilePath, _T("File &Path")));
	VERIFY(menu.AppendMenu(MF_STRING, midFileDir, _T("File &Directory")));
	VERIFY(menu.AppendMenu(MF_STRING, midFileName, _T("File &Name")));
	VERIFY(menu.AppendMenu(MF_STRING, midFileNameWithExt, _T("File Name with Extensi&on")));
	VERIFY(menu.AppendMenu(MF_STRING, midFileExt, _T("File E&xtension")));
	VERIFY(menu.AppendMenu(MF_STRING, midCurDir, _T("C&urrent Directory")));
	VERIFY(menu.AppendMenu(MF_SEPARATOR));
	VERIFY(menu.AppendMenu(MF_STRING, midLineNum, _T("&Line Number")));
	VERIFY(menu.AppendMenu(MF_STRING, midColNum, _T("&Column Number")));
	VERIFY(menu.AppendMenu(MF_STRING, midSelText, _T("Selected &Text")));
	VERIFY(menu.AppendMenu(MF_STRING, midSelStart, _T("Selection &Start")));
	VERIFY(menu.AppendMenu(MF_STRING, midSelEnd, _T("Selection &End")));
	VERIFY(menu.AppendMenu(MF_SEPARATOR));
	VERIFY(menu.AppendMenu(MF_STRING, midPrompt, _T("P&rompt")));

	CRect rc;
	button.GetWindowRect(&rc);
	CPoint pt = rc.CenterPoint();

	UINT flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD;
	MacroId id = (MacroId)menu.TrackPopupMenu(flags, pt.x, pt.y, this);

	CString macro;
	if (id == midFilePath)
		macro = FilePathMacro;
	else if (id == midFileDir)
		macro = FileDirMacro;
	else if (id == midFileName)
		macro = FileNameMacro;
	else if (id == midFileNameWithExt)
		macro = FileNameWithExtMacro;
	else if (id == midFileExt)
		macro = FileExtMacro;
	else if (id == midCurDir)
		macro = CurDirMacro;
	else if (id == midLineNum)
		macro = LineNumMacro;
	else if (id == midColNum)
		macro = ColNumMacro;
	else if (id == midSelText)
		macro = SelTextMacro;
	else if (id == midSelStart)
		macro = SelStartMacro;
	else if (id == midSelEnd)
		macro = SelEndMacro;
	else if (id == midPrompt)
		macro = PromptMacroStart + _T("%");

	if (id != midNone)
		edit.ReplaceSel(macro, TRUE);
}

void ToolsDlg::OnButtonCommandMacro() 
{
	OnMacro(mBtnCommandMacro, mEditCommand);
}

void ToolsDlg::OnButtonParametersMacro() 
{
	OnMacro(mBtnParametersMacro, mEditParameters);
}

void ToolsDlg::OnButtonInitDirMacro() 
{
	OnMacro(mBtnInitDirMacro, mEditInitDir);
}

void ToolsDlg::OnCheckIsFilter() 
{
	// set most likely defaults if command is a filter
	if (mCheckIsFilter.GetCheck() == BST_CHECKED) {
		mCheckSaveFile.SetCheck(BST_UNCHECKED);
		mCheckHideCommandWindow.SetCheck(BST_CHECKED);
	}
}

void ToolsDlg::OnButtonInSubmenu() 
{
	if (mCurItem >= 0) {
		mCommands.IndentSubmenu(mCurItem);
		RefreshList();
	}
}

void ToolsDlg::OnButtonOutSubmenu() 
{
	if (mCurItem >= 0) {
		mCommands.OutdentSubmenu(mCurItem);
		RefreshList();
	}
}

void ToolsDlg::OnCheckIsSeparator() 
{
	if (mCurItem >= 0) {
		UpdateCurListItemText();
		UpdateEnable();
	}
}

void ToolsDlg::OnCheckIsSubmenu() 
{
	if (mCurItem >= 0) {
		UpdateCurListItemText();
		UpdateEnable();
	}
}