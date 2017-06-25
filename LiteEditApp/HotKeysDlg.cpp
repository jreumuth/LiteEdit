// HotKeysDlg.cpp : implementation file
//

#include "stdafx.h"
#include "liteedit.h"
#include "HotKeysDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// HotKeysDlg dialog


HotKeysDlg::HotKeysDlg(HMENU hMenu, CWnd* pParent /*=NULL*/)
	: CDialog(HotKeysDlg::IDD, pParent)
{
	VERIFY(mMenu.Attach(hMenu));
	//{{AFX_DATA_INIT(HotKeysDlg)
	//}}AFX_DATA_INIT
}

void HotKeysDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(HotKeysDlg)
	DDX_Control(pDX, IDC_LIST_MENU_ITEMS, mListMenuItems);
	DDX_Control(pDX, IDC_HOTKEY, mHotKeyNew);
	//}}AFX_DATA_MAP
}

void HotKeysDlg::InitHotKeyData(const CString& parentName, const CString& indent, CMenu& menu) {
	for (UINT i = 0; i < menu.GetMenuItemCount(); ++i) {
		MENUITEMINFO info;
		InitSized(info);
		info.fMask = MIIM_TYPE|MIIM_ID;
		VERIFY(menu.GetMenuItemInfo(i, &info, TRUE));

		if ((info.fType & MFT_SEPARATOR) != 0)
			continue;

		CMenu* subMenu = menu.GetSubMenu(i);

		HotKeyData data(info.wID);

		if (!gConfigData.mAccelTable.GetHotKey(data.cmd, data.key, data.mods))
			gConfigData.mCommands.mAccelTable.GetHotKey(data.cmd, data.key, data.mods);

		data.isEditable = subMenu == NULL;

		CString name;
		menu.GetMenuString(i, name, MF_BYPOSITION);

		CleanupMenuItemName(name);

		int pos = name.Find('\t');
		CString simpleName = pos >= 0 ? name.Left(pos) : name;
		data.fullName = parentName;
		AppendString(data.fullName, simpleName, TEXT(">>"));

		if (ReplaceSubstring(name, TEXT("\t"), TEXT(" ("), ffIgnoreCase) > 0)
			name += ')';

		mListMenuItems.AddString(indent + name);
		mHotKeyData.push_back(data);

		if (subMenu != NULL)
			InitHotKeyData(data.fullName, indent + TEXT("    "), *subMenu);
	}
}

void HotKeysDlg::UpdateEnable() {
	const HotKeyData& data = GetCurData();
	mHotKeyNew.EnableWindow(data.isEditable);
}

//return false is current item cannot be updated due to an error
bool HotKeysDlg::UpdateCurItem() {
	int newCur = mListMenuItems.GetCurSel();

	if (mCurItem >= 0) {
		HotKeyData& data = mHotKeyData[mCurItem];

		if (data.isEditable) {
			WORD vk, mods;
			mHotKeyNew.GetHotKey(vk, mods);

			data.key = (VirtualKey)vk;
			data.mods = FromHotKeyMods(mods);

			CString text;
			mListMenuItems.GetText(mCurItem, text);
			int pos = text.Find('(');
			if (pos >= 0) {
				--pos;//delete space before ( too
				text.Delete(pos, text.GetLength() - pos);
			}

			if (data.key != 0)
				text.Format(TEXT("%s (%s)"), CString(text), KeyAndModifiersToString(data.key, data.mods));

			SetListBoxItemText(mListMenuItems, mCurItem, text);
		}

		int dup = CheckForDuplicate(mCurItem, newCur);
		if (dup >= 0) {
			mListMenuItems.SetCurSel(mCurItem);
			return false;
		}
	}

	mCurItem = newCur;
	return true;
}

//possibleDup is a duplicate that will be allowed
int HotKeysDlg::CheckForDuplicate(int item, int possibleDup/* = -1*/) {
	int i = FindDuplicate(item);
	if (i == possibleDup)
		i = -1;

	if (i >= 0) {
		CString msg;
		msg.Format(TEXT("This hotkey conflicts with the hotkey for '%s'."), mHotKeyData[i].fullName);
		MessageBox(msg, ErrorStr, MB_OK|MB_ICONERROR);
	}

	return i;
}

HotKeyData& HotKeysDlg::GetCurData() {
	int i = mListMenuItems.GetCurSel();
	ASSERT(0 <= i && i < mHotKeyData.size());
	return mHotKeyData[i];
}

int HotKeysDlg::FindDuplicate(int item) const {
	VirtualKey key = mHotKeyData[item].key;
	if (key == 0)
		return -1;
	KeyModifiers mods = mHotKeyData[item].mods;

	for (size_t i = 0; i < mHotKeyData.size(); ++i)
		if (i != item && mHotKeyData[i].key == key && mHotKeyData[i].mods == mods)
			return i;

	return -1;
}

BEGIN_MESSAGE_MAP(HotKeysDlg, CDialog)
	//{{AFX_MSG_MAP(HotKeysDlg)
	ON_LBN_SELCHANGE(IDC_LIST_MENU_ITEMS, OnSelchangeListMenuItems)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// HotKeysDlg message handlers

BOOL HotKeysDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	mCurItem = -1;
	InitHotKeyData(EmptyStr, EmptyStr, mMenu);
	mListMenuItems.SetCurSel(0);
	OnSelchangeListMenuItems();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void HotKeysDlg::OnSelchangeListMenuItems() 
{
	if (UpdateCurItem()) {
		const HotKeyData& data = GetCurData();
		WORD mods = ToHotKeyMods(data.mods);
		mHotKeyNew.SetHotKey(data.key, mods);

		UpdateEnable();
	}
}

void HotKeysDlg::OnOK() 
{
	if (UpdateCurItem()) {
		AccelTable& mainTable = gConfigData.mAccelTable;
		AccelTable& toolsTable = gConfigData.mCommands.mAccelTable;
		mainTable.Clear();
		toolsTable.Clear();

		for (size_t i = 0; i < mHotKeyData.size(); ++i) {
			Command* command = gConfigData.mCommands.FindCommand(mHotKeyData[i].cmd);
			if (command != NULL) {
				command->mAccel = mHotKeyData[i].AsAccel();
				toolsTable.AddAccel(command->mAccel);
			} else
				mainTable.AddAccel(mHotKeyData[i].AsAccel());
		}

		gConfigData.Save();

		CDialog::OnOK();
	}
}

BOOL HotKeysDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return TRUE;
}
