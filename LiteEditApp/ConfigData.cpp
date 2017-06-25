#include "StdAfx.h"
#include "resource.h"
#include "ConfigData.h"
#include "Persist.h"
#include "PromptDlg.h"
#include "PathMgr.h"

LanguagePtr LanguageMgr::FindLanguageByExtension(const CString& ext, LanguagePtr skipLang/* = NULL*/) const {
	for (size_t i = 0; i < mLangs.size(); ++i)
		if (mLangs[i] != skipLang && mLangs[i]->HasExtension(ext))
			return mLangs[i];

	return NULL;
}

bool LanguageMgr::LanguageExists(const CString& name) const {
	for (size_t i = 0; i < mLangs.size(); ++i)
		if (mLangs[i]->GetName().CompareNoCase(name) == 0)
			return true;

	return false;
}

void LanguageMgr::Load(bool reload) {
	PathMgr pathMgr;
	mLangs.clear();

	//declare these outside the try/catch so the file name will be available for
	//the error message
	StringVector langFiles;

	//Must search installed dir before RunUninstalled dir so installed files
	//take precedence.
	//Never search installed dir when forcing run uninstalled because the installed files
	//may not even be the same version. If they were newer trying to load them would error and
	//if they were older loading could upgrade them and then the installed Lite Edit could
	//no longer laod them.
	if (!pathMgr.GetForceRunUninstalled())
		FindAllFiles(pathMgr.GetInstalledLanguagePath(TEXT("*")), langFiles, true);

	if (pathMgr.GetForceRunUninstalled() ||
		pathMgr.GetInstalledLanguageDir() != pathMgr.GetRunUninstalledLanguageDir())
		FindAllFiles(pathMgr.GetRunUninstalledLanguagePath(TEXT("*")), langFiles, true, true);

	size_t i = 0;

	try {
		for (i = 0; i < langFiles.size(); ++i) {
			Persist p(langFiles[i], true, reload);
			LanguagePtr lang = new Language();
			lang->DoPersist(p);

			// only let a language show up once if the same language is in
			// both language directories
			if (!LanguageExists(lang->GetName()))
				mLangs.push_back(lang);
		}
	} catch (Exception& e) {
		if (e.mErrorCode == ErrNewerFileVersion) {
			CString msg;
			msg.Format(TEXT("The version of file '%s' is newer than the version of %s. %s could not load one or more language files."), langFiles[i], ProgramName, ProgramName);
			DisplayError(msg);
		} else
			e.DisplayDefaultError();
	}
}

void LanguageMgr::Save() {
	for (size_t i = 0; i < mLangs.size(); ++i) {
		try {
			Persist p(mLangs[i]->GetFilePath(), false, false);
			mLangs[i]->DoPersist(p);
		//It's important that exceptions are cought and displayed for each language
		//file so an error writing one doesn't prevent other from being written.
		//At this point, all the .lang files have been deleted so the remaining ones
		//would otherwise be lost on an exception.
		} catch (CException* e) {
			e->ReportError(MB_OK|MB_ICONERROR);
		}
	}
}

CString LanguageMgr::BuildFilter() const {
	CString filter = AllFilesFilter;
	bool hasTextFiles = false;

	for (size_t i1 = 0; i1 < mLangs.size(); ++i1) {
		const StringVector& exts = mLangs[i1]->m_extensions;
		if (exts.empty())
			continue;

		CString filterExt;
		CString filterDesc = mLangs[i1]->GetName() + TEXT(" Files (");
		for (size_t i2 = 0; i2 < exts.size(); ++i2) {
			if (StrCmpI(exts[i2], TEXT("txt")) == 0)
				hasTextFiles = true;

			filterExt += TEXT("*.") + exts[i2];
			filterDesc += exts[i2];

			if (i2 < exts.size() - 1) {
				filterExt += ';';
				filterDesc += TEXT("; ");
			}
		}

		filterDesc += ')';
		filter += filterDesc + '|' + filterExt + '|';
	}

	//add in text files if there's not already a language defined
	//for it so it's always there
	if (!hasTextFiles)
		filter += TEXT("Text Files (txt)|*.txt|");

	filter += '|';
	return filter;
}

//These are special names for All Files and Text Files which are
//included in the filter list even if there isn't a language for them.
//NOTE: IT WOULD BREAK PERSISTENCE CODE TO CHANGE THE VALUE OF THESE
//STRINGS.
const CString ALL_FILES_FILTER_NAME = "AllFilesFilterName";
const CString TEXT_FILES_FILTER_NAME = "TextFilesFilterName";

//Filters are one based. All files filter is first, then a filter
//for each language, then the text files filter.

CString LanguageMgr::FilterIndexToFilterName(long index) const {
	if (index == 1)
		return ALL_FILES_FILTER_NAME;
	else if (2 <= index && index < (long)mLangs.size() + 2L)
		return mLangs[index-2]->GetName();
	else if (index == (long)mLangs.size() + 2L)
		return TEXT_FILES_FILTER_NAME;
	else
		return EmptyStr;
}

long LanguageMgr::FilterNameToFilterIndex(const CString& name) const {
	for (size_t i = 0; i < mLangs.size(); ++i)
		if (mLangs[i]->GetName() == name)
			return i + 2;

	if (name == TEXT_FILES_FILTER_NAME)
		return mLangs.size() + 2;

	//It's either the all files filter or no match found.
	//Use the first filter either way.
	return 1;
}

CString RecentFileList::EscapeFileName(const CString& fileName) {
	CString result = fileName;
	result.Replace(_T("&"), _T("&&"));
	return result;
}

void RecentFileList::ValidateSize() {
	if ((long)mFiles.size() > mMax) {
		long numToErase = mFiles.size() - mMax;
		mFiles.erase(mFiles.end() - numToErase, mFiles.end());
	}
}

void RecentFileList::PergeNonExistantFiles() {
	size_t i = 0;
	while (i < mFiles.size())
		//This used to call IsLocalFile to
		//not check if the file exists if it is not on this machine since this
		//function gets called on start up and shouldn't be slow if the user's
		//network connection is slow, and to not remove the file if the network
		//has been temporarily disconnected.
		//However, IsLocalFile was only checking for a colon after a drive
		//letter which won't work for a network path mapped to a drive letter.
		//I didn't bother fixing IsLocalFile since I decided against using
		//PergeNonExistantFiles function anyway.
		if (/*IsLocalFile(mFiles[i]) && */!FileExists(mFiles[i]))
			mFiles.erase(mFiles.begin() + i);
		else
			++i;
}

void RecentFileList::SetMax(long val) {
	mMax = Max(val, 0L);
	ValidateSize();
}

void RecentFileList::Add(const CString& fileName) {
	ASSERT(AbsolutePathExists(fileName));

	Remove(fileName);
	mFiles.insert(mFiles.begin(), fileName);
	ValidateSize();
}

void RecentFileList::Remove(const CString& fileName) {
	long i = mFiles.FindString(fileName, false);
	if (i >= 0)
		mFiles.erase(mFiles.begin() + i);
}

HMENU RecentFileList::CreateMenu(CommandIdCreator& idCreator) const {
	CMenu menu;
	VERIFY(menu.CreatePopupMenu());

	for (size_t i = 0; i < mFiles.size(); ++i) {
		CString file = EscapeFileName(mFiles[i]);
		CString menuString;
		if (i < 10)
			menuString.Format(TEXT("&%d %s"), i, file);
		else if (i <= 'Z' - 'A' + 10)
			menuString.Format(TEXT("&%c %s"), i - 10 + 'A', file);
		else
			menuString = file;

		VERIFY(menu.AppendMenu(MF_STRING|MF_ENABLED, idCreator.CreateId(), menuString));
	}

	return menu.Detach();
}

void RecentFileList::DoPersist(Persist& p) {
	p.PersistNumber(TEXT("MaxRecentFiles"), mMax);
	p.PersistValVector(TEXT("RecentFiles"), mFiles);

	// I decided against removing non-existent files from recent on start up since
	// it's easy for a network file to not exist due to being temporarily disconnected,
	// and also this could slow down launch time, espeically cold launch due to competing
	// IO, though I haven't benchmarked this.
	//if (p.IsLoading())
	//	PergeNonExistantFiles();
}

//Pass NULL for EditCtrl to calculate a display string instead of the real command string,
//i.e. %Prompt% will be replaced with <Prompt> instead of actually prompting the user for input.
CString GetCommandString(const CString& curFileName, const CString& format, EditCtrl* edit/* = NULL*/) {
	bool displayStringOnly = edit == NULL;

	CString result = ExpandEnvironmentStrings(format);

	CString curDir = GetCurrentDirectory();

	CString filePath = curFileName;
	CString fileDir, fileName, fileExt;
	BreakPath(filePath, fileDir, fileName, fileExt);

	CString fileNameWithExt = fileName;
	AppendString(fileNameWithExt, fileExt, _T("."));

	//do replacment for current directory and path macros
	ReplaceSubstring(result, CurDirMacro, curDir, ffIgnoreCase);
	ReplaceSubstring(result, FilePathMacro, filePath, ffIgnoreCase);
	ReplaceSubstring(result, FileDirMacro, fileDir, ffIgnoreCase);
	ReplaceSubstring(result, FileNameMacro, fileName, ffIgnoreCase);
	ReplaceSubstring(result, FileNameWithExtMacro, fileNameWithExt, ffIgnoreCase);
	ReplaceSubstring(result, FileExtMacro, fileExt, ffIgnoreCase);

	if (edit != NULL) {
		long line, col;
		edit->GetLineAndColNum(line, col);
		CString temp;

		temp.Format(_T("%d"), line);
		ReplaceSubstring(result, LineNumMacro, temp, ffIgnoreCase);

		temp.Format(_T("%d"), col);
		ReplaceSubstring(result, ColNumMacro, temp, ffIgnoreCase);

		ReplaceSubstring(result, SelTextMacro, edit->GetSelText(), ffIgnoreCase);

		temp.Format(_T("%d"), edit->GetSelStart());
		ReplaceSubstring(result, SelStartMacro, temp, ffIgnoreCase);

		temp.Format(_T("%d"), edit->GetSelEnd());
		ReplaceSubstring(result, SelEndMacro, temp, ffIgnoreCase);
	}

	//do replacement for prompt macro
	long pos = 0;
	while (pos < result.GetLength()) {
		pos = FindSubstring(result, PromptMacroStart, ffIgnoreCase, pos);
		if (pos < 0)
			break;
		
		long promptStart = pos;
		long promptStrStart = promptStart + PromptMacroStart.GetLength();
		if (promptStrStart >= result.GetLength())
			break;

		pos = FindSubstring(result, TEXT("%"), ffIgnoreCase, promptStrStart);
		if (pos < 0)
			break;
		
		CString promptStr = result.Mid(promptStrStart, pos - promptStrStart);

		CString replaceStr;
		if (displayStringOnly)
			replaceStr.Format(TEXT("<%s>"), promptStr);
		else {
			promptStr = UnescapeChars(promptStr);
			PromptDlg dlg(promptStr, EmptyStr);
			VERIFY(dlg.DoModal() == IDOK);
			replaceStr = dlg.mText;
		}

		result.Delete(promptStart, pos - promptStart + 1);
		result.Insert(promptStart, replaceStr);
		pos = promptStart + replaceStr.GetLength();
	}

	return result;
}

Command& Command::operator=(const Command& rhs) {
	mMenuItemName = rhs.mMenuItemName;
	mCommand = rhs.mCommand;
	mArguments = rhs.mArguments;
	mInitDir = rhs.mInitDir;
	mEnabledExtensions = rhs.mEnabledExtensions;
	mVisibleExtensions = rhs.mVisibleExtensions;
	mAccel = rhs.mAccel;
	SetFlags(rhs.GetFlags());
	mSubCommands = auto_ptr<Commands>();
	mIndent = rhs.mIndent;

	if (mIsSubMenu)
		*GetSubCommands() = *rhs.GetSubCommands();

	return *this;
}

void Command::DoPersist(Persist& p) {
	p.PersistString(TEXT("MenuItemName"), mMenuItemName);
	p.PersistString(TEXT("Command"), mCommand);
	p.PersistString(TEXT("Arguments"), mArguments);
	p.PersistString(TEXT("InitDir"), mInitDir);

	if (p.IsVersBefore(PersistVers12))
		p.PersistBool(TEXT("SaveFileBeforeExecuting"), mSaveFile);

	p.PersistValVector(TEXT("Extensions"), mEnabledExtensions);

	mAccel.DoPersist(p);
	if (p.IsLoading())
		mAccel.cmd = 0;//the dynamically created item ids from another process aren't meaningful

	if (p.IsVersAtLeast(PersistVers9) && p.IsVersBefore(PersistVers12)) {
		p.PersistBool(TEXT("IsFilter"), mIsFilter);

		// before PersistVers12 there was no mHideCommandWindow option and the window
		// was hidden for filters only
		mHideCommandWindow = mIsFilter;
	}

	if (p.IsVersAtLeast(PersistVers12)) {
		p.PersistValVector(TEXT("VisibleExtensions"), mVisibleExtensions);

		MenuFlags flags = GetFlags();
		p.PersistNumber(_T("MenuFlags"), flags);
		SetFlags(flags);

		if (mIsSubMenu)
			GetSubCommands()->DoPersist(p);
	}
}

Commands* Command::GetSubCommands() const {
	if (mIsSubMenu) {
		if (mSubCommands.get() == NULL)
			mSubCommands = auto_ptr<Commands>(new Commands());

		return mSubCommands.get();
	} else
		return NULL;
}

void Command::Indent() {
	++mIndent;
}

void Command::Outdent() {
	ATLASSERT(IsIdented());
	--mIndent;
}

bool Command::IsIdented() const {
	return mIndent > 0;
}

bool Command::SameCommand(const Command& rhs) const {
	return
		mCommand == rhs.mCommand &&
		mArguments == rhs.mArguments;
}

const Command::MenuFlags Command::mfSaveFile = 0x1;
const Command::MenuFlags Command::mfIsFilter = 0x2;
const Command::MenuFlags Command::mfHideCommandWindow = 0x4;
const Command::MenuFlags Command::mfIsSeparator = 0x8;
const Command::MenuFlags Command::mfIsSubMenu = 0x10;

Command::MenuFlags Command::GetFlags() const {
	MenuFlags flags = 0;
	if (mSaveFile)
		flags |= mfSaveFile;
	if (mIsFilter)
		flags |= mfIsFilter;
	if (mHideCommandWindow)
		flags |= mfHideCommandWindow;
	if (mIsSeparator)
		flags |= mfIsSeparator;
	if (mIsSubMenu)
		flags |= mfIsSubMenu;
	return flags;
}

void Command::SetFlags(Command::MenuFlags flags) {
	mSaveFile = (flags & mfSaveFile) != 0;
	mIsFilter = (flags & mfIsFilter) != 0;
	mHideCommandWindow = (flags & mfHideCommandWindow) != 0;
	mIsSeparator = (flags & mfIsSeparator) != 0;
	mIsSubMenu = (flags & mfIsSubMenu) != 0;
}

void Commands::AddCommandsToMenu(HMENU hMenu, const CString& curExt, CommandIdCreator& idCreator) {
	vector<UINT> itemsToDisable;
	mAccelTable.Clear();
	AddCommandsToMenuImpl(hMenu, curExt, idCreator, &mAccelTable, itemsToDisable);
	mAccelTable.DecorateMenu(hMenu);

	// this must be done at the end because the enable state of all submenu items
	// is apparently reset to enabled when the submenu is inserted into the parent menu
	for (size_t i = 0; i < itemsToDisable.size(); ++i)
		SetMenuItemEnabled(hMenu, itemsToDisable[i], false);
}

void Commands::AddCommandsToMenuImpl(HMENU hMenu, const CString& curExt, CommandIdCreator& idCreator, AccelTable* accelTable, vector<UINT>& itemsToDisable) {
	CMenu menu;
	VERIFY(menu.Attach(hMenu));

	UINT menuPos = 0;
	for (size_t i = 0; i < size(); ++i) {
		if (Item(i).ShouldShow(curExt)) {
			if (Item(i).mIsSeparator)
				VERIFY(menu.InsertMenu(menuPos, MF_BYPOSITION | MF_SEPARATOR));
			else if (Item(i).mIsSubMenu) {
				CMenu subMenu;
				VERIFY(subMenu.CreatePopupMenu());
				Item(i).GetSubCommands()->AddCommandsToMenuImpl(subMenu, curExt, idCreator, accelTable, itemsToDisable);
				VERIFY(menu.InsertMenu(menuPos, MF_BYPOSITION | MF_POPUP|MF_STRING, (UINT)subMenu.Detach(), Item(i).mMenuItemName));
			} else {
				UINT id = idCreator.CreateId();
				Item(i).mAccel.cmd = id;
				VERIFY(menu.InsertMenu(menuPos, MF_BYPOSITION | MF_STRING, id, Item(i).mMenuItemName));
				accelTable->AddAccel(Item(i).mAccel);

				if (!Item(i).ShouldEnable(curExt))
					itemsToDisable.push_back(id);
			}

			++menuPos;
		}
	}

	menu.Detach();
}

Command* Commands::FindCommand(UINT cmd) {
	for (size_t i = 0; i < size(); ++i)
		if (Item(i).mIsSubMenu) {
			Command* c = Item(i).GetSubCommands()->FindCommand(cmd);
			if (c != NULL)
				return c;
		} else if (!Item(i).mIsSeparator && Item(i).mAccel.cmd == cmd)
			return &Item(i);

	return NULL;
}

void Commands::DoPersist(Persist& p) {
	p.PersistClassVector(TEXT("Commands"), *this);
}


// moves all GetSubCommands into top level and indents the commands to keep track
// of their original hierarchy
void Commands::Flatten() {
	for (long i1 = (long)size() - 1; i1 >= 0; --i1)
		if (Item(i1).mIsSubMenu) {
			Commands* subs = Item(i1).GetSubCommands();
			subs->Flatten();

			for (size_t i2 = 0; i2 < subs->size(); ++i2)
				subs->Item(i2).Indent();

			insert(begin() + i1 + 1, subs->begin(), subs->end());
			//subs now invalid due to insert
			subs = NULL;
			Item(i1).GetSubCommands()->clear();
		}

	ValidateIndent();
}

// reverses Flatten operation
void Commands::Unflatten() {
	ValidateIndent();

	size_t i = 1;
	while (i < size()) {
		if (Item(i).IsIdented() && Item(i - 1).mIsSubMenu) {
			Item(i).Outdent();
			Commands* subs = Item(i - 1).GetSubCommands();
			subs->push_back(Item(i));
			erase(begin() + i);
		} else
			++i;
	}

	for (i = 0; i < size(); ++i) {
		if (Item(i).mIsSubMenu)
			Item(i).GetSubCommands()->Unflatten();

		// all indentions should have been translated to sub commands
		ATLASSERT(!Item(i).IsIdented());
	}
}

void Commands::ValidateIndent() const {
#ifdef _DEBUG
	if (!empty()) {
		// 1st item can't be indented
		ASSERT(Item(0).mIndent == 0);

		// an item cannot be indented two levels inside the parent
		for (size_t i = 1; i < size(); ++i)
			ASSERT(Item(i).mIndent <= Item(i - 1).mIndent + 1);
	}
#endif
}

int Commands::GetNextMenuAtSameLevel(int index) const {
	long level = Item(index).mIndent;
	++index;

	while (index < size()) {
		long ithLevel = Item(index).mIndent;

		if (ithLevel == level)
			return index;
		else if (ithLevel < level)
			break;
		++index;
	}

	return -1;
}

int Commands::GetPrevMenuAtSameLevel(int index) const {
	long level = Item(index).mIndent;
	--index;

	while (index >= 0) {
		long ithLevel = Item(index).mIndent;

		if (ithLevel == level)
			return index;
		else if (ithLevel < level)
			break;
		--index;
	}

	return -1;
}

// return index of next command that is not in a submenu of the command specified
// by index. returns size of command list if all remaining items are submenu of
// specified item.
size_t Commands::GetNextMenuIndex(size_t index) const {
	long level = Item(index).mIndent;

	do {
		++index;
	} while (index < size() && Item(index).mIndent > level);

	return index;
}

// indents index item and any sub commands indented within it
void Commands::IndentSubmenu(size_t index) {
	for (size_t end = GetNextMenuIndex(index); index < end; ++index)
		Item(index).Indent();
}

// outdents index item and any sub commands indented within it
void Commands::OutdentSubmenu(size_t index) {
	for (size_t end = GetNextMenuIndex(index); index < end; ++index)
		Item(index).Outdent();
}

void FindReplaceData::DoPersist(Persist& p) {
	p.PersistBool(TEXT("AllowEscapedChars"), escapedChars);
	p.PersistBool(TEXT("CloseDialogOnFind"), closeOnFind);
}

void ColorScheme::DoPersist(Persist& p) {
	p.PersistString(TEXT("Name"), mName);
	p.PersistEnum(TEXT("SelColor"), mSelColor);
	TokenColors::DoPersist(p);
}

ColorSchemes& ColorSchemes::operator=(const ColorSchemes& rhs) {
	clear();
	for (size_t i = 0; i < rhs.size(); ++i)
		push_back(new ColorScheme(*rhs[i]));

	mCurScheme = rhs.mCurScheme;
	return *this;
}

void ColorSchemes::DoPersist(Persist& p) {
	p.PersistNumber(TEXT("CurrentColorScheme"), mCurScheme);
	p.PersistPtrVector(TEXT("ColorScheme"), *this);
}

CString GetTimeDateString(const CString& format) {
	CString string = format;

	ReplaceSubstring(string, DateMacro, GetSystemDateString(false), ffIgnoreCase);
	ReplaceSubstring(string, LongDateMacro, GetSystemDateString(true), ffIgnoreCase);
	ReplaceSubstring(string, TimeMacro, GetSystemTimeString(false), ffIgnoreCase);
	ReplaceSubstring(string, LongTimeMacro, GetSystemTimeString(true), ffIgnoreCase);

	return string;
}

//Initialize default values for all config data.
//Add initialization to AddConfigDataForOldVersions instead if an initial value
//won't suffice when reading in an old config file, such as a new accelerator
//that would be overwritten when reading in an old accelerator table.
void ConfigData::InitDefaultValues() {
	mMacroIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MACRO), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);

	MemClear(mConfigFileModifyTime);

	// Older windows OSes used command.com instead of cmd.exe. Use cmd.exe if it exists
	// and otherwise use command.com.
	mCommandFileName = TEXT("cmd.exe");
	// copy parameter to not modify original; we don't need the full path and it looks
	// more simple without it
	if (!SearchForFile(CString(mCommandFileName))) {
		mCommandFileName = TEXT("Command.com");
		ASSERT(SearchForFile(CString(mCommandFileName)));
	}

	mDefExt = TEXT("txt");
	mTimeDateFormat = CString(DateMacro) + TEXT(" ") + TimeMacro;
	mRemoveTrailingWhitespaceOnSave = true;
	mDefaultNewLineType = nlCRLF;
	mDefaultFileFormat = ffAscii;
	mAutoIndent = true;
	mTabWidth = 4;
	mMaximized = false;
	mShowLineNumbers = false;
	mTabInsertsSpaces = false;
	mShowWhitespace = false;
	mWordWrap = false;
	mAddOpenedFilesToWindowsRecentDocuments = true;
	mUndoLimit = 5000;

	LOGFONT lf;
	MemClear(lf);
	StrCpy(lf.lfFaceName, TEXT("Courier New"));
	HDC screenDc = GetDC(NULL);
	lf.lfHeight = PointSizeToLogicalHeight(screenDc, 10);
	ReleaseDC(NULL, screenDc);
	lf.lfWeight = FW_NORMAL;
	VERIFY(mFont.CreateFontIndirect(&lf));
	//mFont.Attach((HFONT)GetStockObject(ANSI_FIXED_FONT));
	//mFont.GetLogFont(&lf);

	mCurrentTokenForConfiguringColor = 0;

	for (int i = 0; i < MAX_CUSTOM_COLORS; ++i)
		mCustomColors[i] = RGB(0xFF, 0xFF, 0xFF);

	// IMPORTANT: add new accelerators to AddConfigDataForOldVersions

	mAccelTable.AddAccel(Accel('N', kmCtrl, ID_FILE_NEW));
	mAccelTable.AddAccel(Accel('O', kmCtrl, ID_FILE_OPEN));
	mAccelTable.AddAccel(Accel('S', kmCtrl, ID_FILE_SAVE));
	mAccelTable.AddAccel(Accel('P', kmCtrl, ID_FILE_PRINT));

	mAccelTable.AddAccel(Accel('Z', kmCtrl, ID_EDIT_UNDO));
	mAccelTable.AddAccel(Accel('Y', kmCtrl, ID_EDIT_REDO));
	mAccelTable.AddAccel(Accel('X', kmCtrl, ID_EDIT_CUT));
	mAccelTable.AddAccel(Accel('C', kmCtrl, ID_EDIT_COPY));
	mAccelTable.AddAccel(Accel('V', kmCtrl, ID_EDIT_PASTE));
	mAccelTable.AddAccel(Accel('A', kmCtrl, ID_EDIT_SELECTALL));
	mAccelTable.AddAccel(Accel('W', kmCtrl, ID_EDIT_ADVANCED_SWITCHCLIPBOARD));
	mAccelTable.AddAccel(Accel('D', kmCtrl|kmShift, ID_EDIT_ADVANCED_INSERTDATETIME));
	mAccelTable.AddAccel(Accel('L', kmCtrl|kmShift, ID_EDIT_ADVANCED_LOWERCASE));
	mAccelTable.AddAccel(Accel('U', kmCtrl|kmShift, ID_EDIT_ADVANCED_UPPERCASE));
	mAccelTable.AddAccel(Accel('T', kmCtrl|kmShift, ID_EDIT_ADVANCED_TITLECASE));

	mAccelTable.AddAccel(Accel('F', kmCtrl, ID_SEARCH_FIND));
	mAccelTable.AddAccel(Accel(VK_F3, kmNone, ID_SEARCH_FINDNEXT));
	mAccelTable.AddAccel(Accel(VK_F3, kmShift, ID_SEARCH_FINDPREVIOUS));
	mAccelTable.AddAccel(Accel('H', kmCtrl, ID_SEARCH_REPLACE));
	mAccelTable.AddAccel(Accel('G', kmCtrl, ID_SEARCH_GOTO));

	mAccelTable.AddAccel(Accel(VK_F2, kmCtrl, ID_BOOKMARKS_TOGGLE));
	mAccelTable.AddAccel(Accel(VK_F2, kmNone, ID_BOOKMARKS_NEXT));
	mAccelTable.AddAccel(Accel(VK_F2, kmShift, ID_BOOKMARKS_PREVIOUS));

	mAccelTable.AddAccel(Accel('W', kmCtrl|kmShift, ID_OPTIONS_WORDWRAP));

	mAccelTable.AddAccel(Accel(VK_F1, kmNone, ID_HELP_CONTENTS));

	// IMPORTANT: add new accelerators to AddConfigDataForOldVersions

	PathMgr pathMgr;

	mCommands.push_back(Command(mCommandFileName, TEXT("&Console Window"), EmptyStr, FileDirMacro, false, 'C', kmShift|kmCtrl));

	mCommands.push_back(GetWindowsExplorerCommand());
	
	mCommands.push_back(Command(pathMgr.GetInternetExplorerPath(), TEXT("&Internet Explorer"), Quote(FilePathMacro), FileDirMacro, true, 'I', kmCtrl));
	
	//Lite Edit command was added to make up for lack of multi-window. Is it more clutter than useful?
	//mCommands.push_back(Command(GetFileNameWithExt(pathMgr.GetAppPath()), TEXT("&Lite Edit"), EmptyStr, EmptyStr, false, 'E', kmShift|kmCtrl));

	Command exeCurFile(TEXT("%FileDir%%FileName%.exe"), TEXT("E&xecute Current File"), EmptyStr, FileDirMacro, true, VK_F5, kmNone);
	exeCurFile.mEnabledExtensions.push_back(TEXT("c"));
	exeCurFile.mEnabledExtensions.push_back(TEXT("cpp"));
	exeCurFile.mEnabledExtensions.push_back(TEXT("pas"));
	mCommands.push_back(exeCurFile);

	// IMPORTANT: add new commands to AddConfigDataForOldVersions
}

//When a new options is persisted, you must add a new persistence
//version in Persist.h and update CurPersistVers. If you don't, you
//will get errors when you use an old config file.
//load is true when loading and false when saving.
//reload is false for the load during inital start up and true for
//subsequen loads. I check this for things I don't want to change on activate.
//Returns true if no errors occurred.
bool ConfigData::DoPersist(bool load, bool reload) {
	// if reload is true, then load must be true also
	ASSERT(!reload || load);

	bool success = false;
	bool tempBool;
	CRect tempRect;

	CString configFilePath = PathMgr().GetActualConfigFilePath();

	bool useSystemSelectionColor = false;
	long filterIndex = 0;

	try {
		//save or load data, except if I'm loading and the
		//file doesn't exist
		if (!load || FileExists(configFilePath)) {
			Persist p(configFilePath, load, reload);

			LPCTSTR advancedSettingsComment =
			_T("The settings after this comment are advanced settings that you may configure ")
			_T("by directly editing this file. Modify the values to the right of the equals ")
			_T("sign to configure these settings.")
			_T("\n\nConsider making a backup copy of this file before editing it. If you put the ")
			_T("config file in a bad state and cannot fix it, you can always delete the config ")
			_T("file which causes Lite Edit to recreate the config file with the default settings.")
			_T("\n\nNOTE: You cannot use Lite Edit to edit this file because Lite Edit overwrites ")
			_T("this file on shutdown.");
			p.AddComment(advancedSettingsComment, false, true);

			if (p.IsVersAtLeast(PersistVers13)) {
				p.AddComment(_T("The maximum number of undo items. -1 means no limit."), false, false);
				p.PersistNumber(_T("UndoLimit"), mUndoLimit);

				p.PersistNewLine();
				p.PersistBool(_T("AddOpenedFilesToWindowsRecentDocuments"), mAddOpenedFilesToWindowsRecentDocuments);
			}

			p.AddComment(_T("End of advanced settings section."), true, true);

			p.PersistString(TEXT("DefaultExtention"), mDefExt);
			p.PersistString(TEXT("TimeDateFormat"), mTimeDateFormat);
			p.PersistBool(TEXT("RemoveTrailingWhitespaceOnSave"), mRemoveTrailingWhitespaceOnSave);
			p.PersistEnum(TEXT("NewLineType"), mDefaultNewLineType);
			p.PersistBool(TEXT("AutoIndent"), mAutoIndent);
			p.PersistNumber(TEXT("TabWidth"), mTabWidth);

			if (reload)
				p.PersistBool(TEXT("Maximized"), tempBool);
			else
				p.PersistBool(TEXT("Maximized"), mMaximized);

			if (p.IsVersAtLeast(PersistVers7))
				p.PersistString(TEXT("OpenFilterName"), mOpenFilterName);
			else
				p.PersistNumber(TEXT("OpenFilterIndex"), filterIndex);

			if (p.IsVersBefore(PersistVers3))
				p.PersistBool(TEXT("UseSystemSelectionColor"), useSystemSelectionColor);

			p.PersistNewLine();

			if (reload)
				p.PersistVal(TEXT("WinPos"), tempRect);
			else
				p.PersistVal(TEXT("WinPos"), mWinPos);

			p.PersistNewLine();

			mRecentFileList.DoPersist(p);

			p.PersistNewLine();

			ASSERT(sizeof(HFONT) == sizeof(mFont.m_hObject));//make sure reference cast is safe
			p.PersistVal(TEXT("Font"), (HFONT&)mFont.m_hObject);

			p.PersistNewLine();

			if (p.IsVersBefore(PersistVers3)) {
				//just a TokenColors used to be persisted
				ColorSchemePtr customColorScheme = new ColorScheme(TEXT("Custom Color Scheme"), useSystemSelectionColor ? scSystem : scInvert);
				customColorScheme->TokenColors::DoPersist(p);
				mColorSchemes.push_back(customColorScheme);
				mColorSchemes.SetCurScheme(mColorSchemes.size() - 1);
			} else
				mColorSchemes.DoPersist(p);

			p.PersistNewLine();

			p.PersistStaticNumberArray(TEXT("CustomColors"), mCustomColors, MAX_CUSTOM_COLORS);

			p.PersistNewLine();

			mAccelTable.DoPersist(p);

			p.PersistNewLine();

			mCommands.DoPersist(p);

			p.PersistNewLine();

			mFindReplaceData.DoPersist(p);

			if (p.IsVersAtLeast(PersistVers2)) {
				if (p.IsVersAtLeast(PersistVers4))
					p.PersistNewLine();

				p.PersistBool("ShowLineNumbers", mShowLineNumbers);
			}

			if (p.IsVersAtLeast(PersistVers5)) {
				p.PersistBool("TabInsertsSpaces", mTabInsertsSpaces);
				p.PersistBool("ShowWhitespace", mShowWhitespace);
			}

			if (p.IsVersAtLeast(PersistVers6))
				if (reload) {
					// Don't realod wrap wrap setting because:
					// 1. It's reasonable to want this to be a different value for
					//    each instance since the preferred settings depends on what
					//    file you're viewing and the window width.
					// 2. Changing the word wrap setting can be very slow in some
					//    cases. I wouldn't want to make the user wait on activate.
					p.PersistBool("WordWrap", tempBool);
				} else
					p.PersistBool("WordWrap", mWordWrap);

			if (p.IsVersAtLeast(PersistVers11))
				p.PersistEnum("FileFormat", mDefaultFileFormat);

			//***************************************************
			// ADD CODE TO PERSIST NEW OPTIONS ABOVE THIS COMMENT
			//***************************************************

			if (load) {
				if (p.IsVersBefore(CurPersistVer))
					AddConfigDataForOldVersions(p.GetVersion(), true);
			}
		} else if (load) {//loading but no ini file exists
			ASSERT(!FileExists(configFilePath));

			//pass first persist version so that all new config data will be added
			AddConfigDataForOldVersions(PersistVers1, false);
		}

		success = true;
	} catch (Exception& e) {
		if (e.mErrorCode == ErrNewerFileVersion) {
			CString msg;
			msg.Format(TEXT("The version of file '%s' is newer than the version of %s. %s will be overwritten. You should back up %s now if you don't want to lose your configuration data."), configFilePath, ProgramName, ConfigFileName, ConfigFileName);
			DisplayError(msg);
		} else
			e.DisplayDefaultError();
	} catch (CException* e) {
		e->ReportError(MB_OK|MB_ICONERROR);
	}

	GetConfigFileModified();

	//mLangMgr gets saved by the LanguagesPage
	if (load) {
		mLangMgr.Load(reload);

		//Convert filter index to filter name if loading an
		//old config file which persisted the index.
		if (filterIndex > 0)
			mOpenFilterName = mLangMgr.FilterIndexToFilterName(filterIndex);
	}
	
	return success;
}

bool ConfigData::ReloadIfModified(EditCtrl& edit) {
	if (GetConfigFileModified() && DoPersist(true, true)) {
		SetEdit(edit);
		return true;
	} else
		return false;
}

void ConfigData::SetEdit(EditCtrl& edit) {
	edit.SetFont(mFont);
	edit.SetAutoIndent(mAutoIndent);
	edit.SetTabWidth(mTabWidth);
	edit.SetShowLineNumbers(mShowLineNumbers);
	edit.SetTabInsertsSpaces(mTabInsertsSpaces);
	edit.SetShowWhitespace(mShowWhitespace);
	edit.SetWordWrap(mWordWrap);
	edit.SetUndoLimit(mUndoLimit);

	ColorSchemePtr curCS = mColorSchemes.GetCurColorScheme();
	edit.SetUseSystemSelectionColors(curCS == NULL || mColorSchemes.GetCurColorScheme()->GetSelColor() == scSystem);
	edit.SetTokenColors(curCS.GetPtr());
}

bool ConfigData::CanSwitchClipboard() const {
	return
		IsClipboardFormatAvailable(CF_TEXT) ||
#ifdef _UNICODE
		IsClipboardFormatAvailable(CF_UNICODETEXT) ||
#endif
		!mTempClip.IsEmpty();
}

//Edit>>Advanced>>Switch Clipboard command.
void ConfigData::SwitchClipboard(HWND hwnd) {
	CString clipboard;
	bool succ = GetClipboardString(hwnd, clipboard, false);

	//Allow user to get mTempClip even if I can't store what's currently on the clipboard.
	//But prompt the user if they would loose the data currently on the clipboard.
	if (!succ && !mTempClip.IsEmpty()) {
		bool clipboardHasData = CountClipboardFormats() > 0;

		if (!clipboardHasData)
			succ = true;
		else if (MessageBox(hwnd, _T("Since the clipboard does not contain text, Lite Edit cannot store the data on the clipboard for you to switch back to later. Do you want to Switch Clipboards and loose the data currently on the clipboard?"), _T("Warning"), MB_YESNO|MB_ICONWARNING) == IDYES)
			succ = true;
	}

	if (succ) {
		succ = SetClipboardString(hwnd, mTempClip, false);
		ATLASSERT(succ);

		if (succ)
			mTempClip = clipboard;
	}
}

bool ConfigData::CanLastClipboard() const {
	return mLastClipboard.GetLength() > 0;
}

//Edit>>Advanced>>Last Clipboard command.
void ConfigData::LastClipboard(HWND hwnd) {
	if (mLastClipboard.GetLength() > 0)
		SetClipboardString(hwnd, mLastClipboard, true);
}

bool ConfigData::Execute(const Command& cmd, const CString& fileName, HWND hwnd, EditCtrl& edit) const {
	CString rawProgName = GetCommandString(fileName, cmd.mCommand, &edit);
	CString progName = rawProgName;
	SearchForFile(progName);
	progName = Quote(progName);
	CString commandLine = GetCommandString(fileName, cmd.mArguments, &edit);
	commandLine = progName + _T(" ") + commandLine;
	LPTSTR commandLineBuff = commandLine.GetBuffer(commandLine.GetLength());
	CString initDirString = GetCommandString(fileName, cmd.mInitDir, &edit);
	LPCTSTR initDir = initDirString.IsEmpty() ? NULL : (LPCTSTR)initDirString;
	DWORD flags = 0;
	BOOL inheritHandle = (Bool)cmd.mIsFilter;
	PROCESS_INFORMATION procInfo;
	MemClear(procInfo);
	STARTUPINFO startUpInfo;
	MemClear(startUpInfo);
	startUpInfo.cb = sizeof(startUpInfo);
	SECURITY_ATTRIBUTES secAttrs;
	MemClear(secAttrs);
	secAttrs.nLength = sizeof(secAttrs);
	secAttrs.bInheritHandle = inheritHandle;
	CString errorMsg;
	CString filterText;
	bool filterTextEndsInNewLine = false;

	PathMgr pathMgr;
	CString inPath = pathMgr.GetStdInTempFilePath();
	CString outPath = pathMgr.GetStdOutTempFilePath();
	CString errPath = pathMgr.GetStdErrTempFilePath();

	AutoHandle stdIn;
	AutoHandle stdOut;
	AutoHandle stdErr;

	//show a wait cursor for filters
	WaitCursor wc(cmd.mIsFilter);

	if (cmd.mIsFilter) {
		filterText = edit.GetSelText();
		if (edit.GetWordWrap())
			RemoveCR(filterText);

		filterTextEndsInNewLine = EndsWith(filterText, _T("\n"));

		StringToFile(inPath, ConvertLFtoCRLF(filterText));

		//make hidden
		VERIFY(SetFileAttributes(inPath, GetFileAttributes(inPath)|FILE_ATTRIBUTE_HIDDEN));

		// Ideally, I would like the filter feature to not have to create temporary files,
		// but unfortunately I ran into problems trying to do this with CreatePipe.
		// When you call a filter passing in a handle created by CreatePipe,
		// you can only write data until the buffer that the filter reads from is full.
		// Then the filter is supposed to read from the buffer, and then let the calling
		// program write more data. But to do this, the calling program and filter would
		// have to somehow let each other know when they should write to or read from the
		// buffer. Furthermore, if the filter doesn't get the entire string from the calling
		// program, the filter never gets to the "end of the file" and hangs. I changed
		// Lite Edit to use temporary files because of this problem.

		stdIn = CreateFile(inPath, GENERIC_READ|GENERIC_WRITE, NULL, &secAttrs, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL);
		stdOut = CreateFile(outPath, GENERIC_READ|GENERIC_WRITE, NULL, &secAttrs, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
		stdErr = CreateFile(errPath, GENERIC_READ|GENERIC_WRITE, NULL, &secAttrs, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);

		startUpInfo.dwFlags |= STARTF_USESTDHANDLES;
		startUpInfo.hStdInput = stdIn;
		startUpInfo.hStdOutput = stdOut;
		startUpInfo.hStdError = stdErr;
		//startUpInfo.lpTitle = const_cast<LPTSTR>((LPCTSTR)cmd.mMenuItemName);
	}

	if (cmd.mHideCommandWindow) {
		startUpInfo.dwFlags |= STARTF_USESHOWWINDOW;
		startUpInfo.wShowWindow = SW_HIDE;
	}

	BOOL succ = CreateProcess(NULL, commandLineBuff, &secAttrs, &secAttrs, inheritHandle, flags, NULL, initDir, &startUpInfo, &procInfo);

	//IMPORTANT: call GetLastError() right after CreateProcess() so nothing can override
	//the error code

	if (!succ) {
		DWORD err = GetLastError();
		errorMsg = GetErrorMessage(err);
	}

	commandLine.ReleaseBuffer();

	if (succ && cmd.mIsFilter) {
		//standard output and error may not have all the text until the command
		//finishes, so wait for the command to finish
		VERIFY(WaitForSingleObject(procInfo.hProcess, INFINITE) != WAIT_FAILED);

		//I must close these handles before I can read from these files.
		stdOut.Close();
		stdErr.Close();

		errorMsg = FileToString(errPath, true);

		if (errorMsg.IsEmpty()) {
			filterText = FileToString(outPath, true);

			//Filters seem to ignore the last new line passed in, if there is one,
			//and also to return an extra new line (maybe this works out well on
			//the console). These behaviors cancel each other out as long as the
			//filter input ends in a new line. But if the input doesn't in a new
			//line, the behaviors don't cancel each other out and I have to remove
			//the extra trailing new line. I could always add an extra new line to
			//the filter's input and always remove a trailing new line from the filter's
			//output to cancel out the filter ignoring and adding a trailng new line.
			//That would cause a trailing new line, if present, to be sent to the filter
			//and I used to do that. However, I discovered the user would probably not
			//expect a trailing new line to be passed to the filter. A trailing new line
			//is selected when the selection end is at the beginning of a line, and the
			//user probably wouldn't expect an extra blank line to be sent to the filter
			//in that case.
			if (!filterTextEndsInNewLine && EndsWith(filterText, _T("\n")))
				filterText.Delete(filterText.GetLength() - 1);

			edit.ReplaceSel(filterText, true);
		} else
			succ = FALSE;
	}

	// delete the filter files so I don't leave junk around
	if (cmd.mIsFilter) {
		// I can't delete the files until the handles are closed.
		// It's ok to call Close more than once.
		stdIn.Close();
		stdOut.Close();
		stdErr.Close();

		VERIFY(DeleteFile(inPath));
		VERIFY(DeleteFile(outPath));
		VERIFY(DeleteFile(errPath));
	}

	if (!succ) {
		CString msg;
		msg.Format(_T("Error executing command '%s':\n\n%s"), rawProgName, errorMsg);
		MessageBox(hwnd, msg, ErrorStr, MB_OK|MB_ICONERROR);
	}

	return (Bool)succ;
}

void ConfigData::ExecuteHelp(HelpFile helpFile, HWND hwnd) const {
	CString url = PathMgr().GetHelpFilePath(helpFile);
	LaunchURL(url, hwnd);
}

void ConfigData::LaunchURL(const CString& url, HWND hwnd) const {
#if 0
	SHELLEXECUTEINFO info;
	InitSized(info);
	info.hwnd = hwnd;
	info.lpVerb = TEXT("open");
	info.lpFile = _T("foo");//url;
	info.nShow = SW_SHOWDEFAULT;
	info.fMask = SEE_MASK_WAITFORINPUTIDLE;
	VERIFY(ShellExecuteEx(&info));
	int err = (int)info.hInstApp;
#endif

	// uses system web browser, but does not work with named anchors
	// (the part after a # in the URL) for local URLs.
	// I wrote Lite Edit to not use URLs with named anchors.
	int err = (int)ShellExecute(hwnd, TEXT("open"), url, NULL, NULL, SW_SHOWDEFAULT);

	if (err <= 32)
		DisplayError(GetErrorMessage(err));

#if 0
	// Can display URLs containing a named anchor, but may only work with win >= 2000.
	// Also often doesn't return a failed error code on failure and may show a dialog.
	CString rumDllCmd;
	rumDllCmd.Format(_T("/c rundll32 URL.dll, FileProtocolHandler \"file:///%s\""), url);
	int err = (int)ShellExecute(hwnd, _T("open"), _T("cmd.exe"), rumDllCmd, NULL, SW_HIDE);
#endif

#if 0
	// launches in IE. works with named anchors.
	Command ieCommand(PathMgr().GetInternetExplorerPath());
	ieCommand.mArguments = Quote(url);
	Execute(ieCommand, EmptyStr, hwnd);
#endif
}

//Usually, when reading an old version of a config file, using the
//default value (from InitDefaultValues) of options newer than the
//config file is good enough. If not, add code to this method to
//configure the new option.
//One case where InitDefaultValues isn't good enough is when reading
//an old config file would overwrite the initialization in InitDefaultValues,
//such as reading in an old accelerator table.
//This method is also called when no config file yet exists.
void ConfigData::AddConfigDataForOldVersions(long version, bool convertingOldVersion) {
	if (version < PersistVers2) {
		mAccelTable.AddAccel(Accel('G', kmCtrl|kmShift, ID_EDIT_ADVANCED_TOGGLECASE));

		CString charMapFileName = TEXT("%SystemRoot%\\System32\\charmap.exe");
		if (FileExists(ExpandEnvironmentStrings(charMapFileName)))
			mCommands.push_back(Command(charMapFileName, TEXT("Character &Map"), EmptyStr, EmptyStr, false, 'M', kmCtrl));
	}

	if (version < PersistVers3) {
		ColorSchemePtr cs;

		//sky, windows looking default colors, formerly T-Shirt
		cs = new ColorScheme(TEXT("Sky"), scSystem);
		cs->Item(tkPlainText) = RGB(0x0, 0x0, 0x0);
		cs->Item(tkComment) = RGB(0x0, 0x80, 0x0);
		cs->Item(tkKeyword) = RGB(0x0, 0x0, 0xFF);
		cs->Item(tkSymbol) = RGB(0xC0, 0x00, 0xC0);
		cs->Item(tkString) = RGB(0x60, 0x60, 0xC0);
		cs->Item(tkBackground) = RGB(0xFF, 0xFF, 0xFF);
		mColorSchemes.push_back(cs);

		//ocean, formerly pac man colors
		cs = new ColorScheme(TEXT("Ocean"), scInvert);
		cs->Item(tkPlainText) = RGB(255, 255, 0);
		cs->Item(tkComment) = RGB(120, 255, 255);
		cs->Item(tkKeyword) = RGB(255, 255, 255);
		cs->Item(tkSymbol) = RGB(255, 255, 255);
		cs->Item(tkString) = RGB(64, 255, 64);
		cs->Item(tkBackground) = RGB(0, 0, 172);
		mColorSchemes.push_back(cs);

		//forest
		cs = new ColorScheme(TEXT("Forest"), scInvert);
		cs->Item(tkPlainText) = RGB(0, 32, 0);
		cs->Item(tkComment) = RGB(255, 0, 0);
		cs->Item(tkKeyword) = RGB(0, 0, 255);
		cs->Item(tkSymbol) = RGB(255, 0, 255);
		cs->Item(tkString) = RGB(64, 32, 0);
		cs->Item(tkBackground) = RGB(212, 255, 212);
		mColorSchemes.push_back(cs);

		//space
		cs = new ColorScheme(TEXT("Space"), scInvert);
		cs->Item(tkPlainText) = RGB(255, 255, 255);
		cs->Item(tkComment) = RGB(176, 176, 255);
		cs->Item(tkKeyword) = RGB(128, 255, 128);
		cs->Item(tkSymbol) = RGB(255, 128, 255);
		cs->Item(tkString) = RGB(255, 255, 0);
		cs->Item(tkBackground) = RGB(0, 0, 0);
		mColorSchemes.push_back(cs);
	}

	if (version < PersistVers6) {
		mAccelTable.AddAccel(Accel('P', kmCtrl, ID_FILE_PRINT));
		mAccelTable.AddAccel(Accel('W', kmCtrl|kmShift, ID_OPTIONS_WORDWRAP));
	}

	if (version < PersistVers10)
		mAccelTable.AddAccel(Accel('L', kmCtrl, ID_EDIT_ADVANCED_LASTCLIPBOARD));

	if (convertingOldVersion) {
		if (version < PersistVers12) {
			Command oldWE = GetOldWindowsExplorerCommand();
			Command newWE = GetWindowsExplorerCommand();

			for (size_t i = 0; i < mCommands.size(); ++i)
				if (mCommands[i].SameCommand(oldWE)) {
					mCommands[i].mCommand = newWE.mCommand;
					mCommands[i].mArguments = newWE.mArguments;
					mCommands[i].mHideCommandWindow = newWE.mHideCommandWindow;
					break;
				}
		}
	}
}

// Returns true if the config file needs to be reloaded and also
// updates the stored config file modify time.
bool ConfigData::GetConfigFileModified() {
	CString configFilePath = PathMgr().GetActualConfigFilePath();

	bool exists = FileExists(configFilePath);
	FILETIME modifyTime;
	if (exists)
		modifyTime = GetFileModifyTime(configFilePath);
	else
		GetSystemTimeAsFileTime(&modifyTime); // assume anything saved after now will be newer

	bool reloadNeeded =
		exists &&
		CompareFileTime(&modifyTime, &mConfigFileModifyTime) > 0;

	mConfigFileModifyTime = modifyTime;
	return reloadNeeded;
}

ConfigData gConfigData;