#ifndef ConfigDataH
#define ConfigDataH

#include "EditCtrl.h"

typedef vector<LanguagePtr> Languages;

class LanguageMgr {
private:
public:
	Languages mLangs;

	LanguagePtr FindLanguageByExtension(const CString& ext, LanguagePtr skipLang = NULL) const;
	bool LanguageExists(const CString& name) const;
	void Load(bool reload);
	void Save();
	CString BuildFilter() const;
	CString FilterIndexToFilterName(long index) const;
	long FilterNameToFilterIndex(const CString& name) const;
};

class RecentFileList {
private:
	static CString EscapeFileName(const CString& fileName);

	long mMax;
	StringVector mFiles;

	void ValidateSize();
	void PergeNonExistantFiles();

public:
	RecentFileList() : mMax(15) {}

	long GetMax() const {return mMax;}
	void SetMax(long val);

	void Add(const CString& fileName);
	void Remove(const CString& fileName);
	const StringVector& GetItems() const {return mFiles;}
	HMENU CreateMenu(CommandIdCreator& idCreator) const;
	void DoPersist(Persist& p);
};

static LPCTSTR FilePathMacro = TEXT("%FilePath%");
static LPCTSTR FileDirMacro = TEXT("%FileDir%");
static LPCTSTR FileNameMacro = TEXT("%FileName%");
static LPCTSTR FileNameWithExtMacro = TEXT("%FileNameWithExt%");
static LPCTSTR FileExtMacro = TEXT("%FileExt%");
static LPCTSTR CurDirMacro = TEXT("%CurDir%");
static LPCTSTR LineNumMacro = TEXT("%LineNum%");
static LPCTSTR ColNumMacro = TEXT("%ColNum%");
static LPCTSTR SelTextMacro = TEXT("%SelText%");
static LPCTSTR SelStartMacro = TEXT("%SelStart%");
static LPCTSTR SelEndMacro = TEXT("%SelEnd%");
static CString PromptMacroStart = TEXT("%Prompt:");

// calculates a display string instead of real command string if you pass NULL for edit
CString GetCommandString(const CString& curFileName, const CString& format, EditCtrl* edit = NULL);

class Commands;

class Command {
public:
	CString mMenuItemName;
	CString mCommand;
	CString mArguments;
	CString mInitDir;
	StringVector mEnabledExtensions;
	StringVector mVisibleExtensions;
	Accel mAccel;
	bool mSaveFile;
	bool mIsFilter;
	bool mHideCommandWindow;
	bool mIsSeparator;
	bool mIsSubMenu;
	long mIndent;
	// UPDATE operator= if you add any new members

	Command(const CString& command = EmptyStr, const CString& menuItemName = TEXT("New &Tool"), const CString& arguments = Quote(FilePathMacro), const CString& initDir = FileDirMacro, bool saveFile = true, VirtualKey key = 0, KeyModifiers mods = kmNone, bool hideCommandWindow = false) :
		mMenuItemName(menuItemName), mCommand(command), mArguments(arguments),
		mInitDir(initDir), mSaveFile(saveFile), mAccel(key, mods, 0), mIsFilter(false),
		mHideCommandWindow(hideCommandWindow), mIsSeparator(false), mIsSubMenu(false),
		mIndent(0) {}

	Command(const Command& rhs) {
		*this = rhs;
	}

	Command& operator=(const Command& rhs);

	bool ShouldEnable(const CString& ext) const {
		return ShouldImpl(mEnabledExtensions, ext);
	}

	bool ShouldShow(const CString& ext) const {
		return ShouldImpl(mVisibleExtensions, ext);
	}

	void DoPersist(Persist& p);
	Commands* GetSubCommands() const;
	void Indent();
	void Outdent();
	bool IsIdented() const;
	bool SameCommand(const Command& rhs) const;

private:
	typedef long MenuFlags;
	static const MenuFlags mfSaveFile;
	static const MenuFlags mfIsFilter;
	static const MenuFlags mfHideCommandWindow;
	static const MenuFlags mfIsSeparator;
	static const MenuFlags mfIsSubMenu;

	MenuFlags GetFlags() const;
	void SetFlags(MenuFlags flags);

	bool ShouldImpl(const StringVector& extList, const CString& ext) const {
		return extList.empty() || extList.HasString(ext, false);
	}

	mutable auto_ptr<Commands> mSubCommands;
	// UPDATE operator= if you add any new members
};

class Commands : public Vector<Command> {
public:
	AccelTable mAccelTable;

	void AddCommandsToMenu(HMENU hMenu, const CString& curExt, CommandIdCreator& idCreator);
	Command* FindCommand(UINT cmd);
	void DoPersist(Persist& p);
	void Flatten();
	void Unflatten();
	int GetNextMenuAtSameLevel(int index) const;
	int GetPrevMenuAtSameLevel(int index) const;
	size_t GetNextMenuIndex(size_t index) const;
	void IndentSubmenu(size_t index);
	void OutdentSubmenu(size_t index);

private:
	void AddCommandsToMenuImpl(HMENU hMenu, const CString& curExt, CommandIdCreator& idCreator, AccelTable* accelTable, vector<UINT>& itemsToDisable);
	void ValidateIndent() const;
};

//Subset of FindReplaceData that's used for searching.
//It's useful to keep "search criteria" data separate from
//FindReplaceData so you can store the search criteria
//and know when it changed.
struct SearchData {
	CString findStr;
	FindFlags flags;
	bool selectionOnly;
	bool escapedChars;//persisted in derived class FindReplaceData
	bool newSearch;

	SearchData() :
		flags(ffIgnoreCRs | ffIgnoreCase), selectionOnly(false), escapedChars(true), newSearch(true) {}

	bool Changed(const SearchData& rhs, bool &changedDirection) {
		if (newSearch) {
			changedDirection = false;
			return true;
		} else {
			changedDirection = (flags & ffReverse) != (rhs.flags & ffReverse);
			bool changedNonDirFlag = (flags & ~ffReverse) != (rhs.flags & ~ffReverse);

			return
				changedNonDirFlag ||
				selectionOnly != rhs.selectionOnly ||
				escapedChars != rhs.escapedChars ||
				StrCmpEx(findStr, rhs.findStr, (flags & ffMatchCase) != 0) != 0;
		}
	}
};

struct FindReplaceData : SearchData {
	//the following are never read by the dialog and so don't need to be initialized
	bool findNext;
	bool replaceCurrent;
	bool replaceAll;
	CString replaceStr;

	StringVector findStrings;
	StringVector replaceStrings;
	bool closeOnFind;//persisted

	FindReplaceData() :
		closeOnFind(true) {}

	void DoPersist(Persist& p);
};

enum SelColorKinds {scInvert, scSystem};

class ColorScheme : public TokenColors {
private:
	CString mName;
	SelColorKinds mSelColor;

public:
	ColorScheme(const CString& name = EmptyStr, SelColorKinds sck = scInvert) : mName(name), mSelColor(sck) {}

	const CString& GetName() const {return mName;}
	void SetName(const CString& name) {mName = name;}
	SelColorKinds GetSelColor() const {return mSelColor;}
	void SetSelColor(SelColorKinds scKind) {mSelColor = scKind;}

	virtual void DoPersist(Persist& p);
};
PTR_DEF(ColorScheme);

class ColorSchemes : public Vector<ColorSchemePtr> {
private:
	long mCurScheme;

public:
	ColorSchemes() : mCurScheme(0) {}
	ColorSchemes(const ColorSchemes& rhs) {*this = rhs;}

	ColorSchemes& operator=(const ColorSchemes& rhs);

	ColorSchemePtr GetCurColorScheme() const {
		return mCurScheme >= 0 && !empty() ? Item(mCurScheme) : NULL;
	}

	long GetCurScheme() const {return mCurScheme;}

	void SetCurScheme(long val) {
		ATLASSERT(-1 <= val && val < (long)size());
		mCurScheme = val;
	}

	void DoPersist(Persist& p);	
};

static LPCTSTR DateMacro = TEXT("%Date%");
static LPCTSTR LongDateMacro = TEXT("%LongDate%");
static LPCTSTR TimeMacro = TEXT("%Time%");
static LPCTSTR LongTimeMacro = TEXT("%LongTime%");

CString GetTimeDateString(const CString& format);

static const int MAX_CUSTOM_COLORS = 16;

static const CString AllFilesFilter = TEXT("All Files|*.*|");

struct ConfigData {
	CString mDefExt;
	CString mTimeDateFormat;
	bool mRemoveTrailingWhitespaceOnSave;
	NewLineType mDefaultNewLineType;//can never be nlDefault
	FileFormat mDefaultFileFormat;
	bool mAutoIndent;
	long mTabWidth;
	RecentFileList mRecentFileList;
	Commands mCommands;
	FindReplaceData mFindReplaceData;
	CRect mWinPos;
	bool mMaximized;
	bool mShowLineNumbers;
	bool mTabInsertsSpaces;
	bool mShowWhitespace;
	bool mWordWrap;
	bool mAddOpenedFilesToWindowsRecentDocuments;
	long mUndoLimit;

	//I persist a name for the open file filter instead of its
	//index because the index would be wrong if languages were
	//added, remove, or reordered
	CString mOpenFilterName;

	CFont mFont;
	ColorSchemes mColorSchemes;
	long mCurrentTokenForConfiguringColor; // not persisted across shutdown as this probably is necessary for the user
	LanguageMgr mLangMgr;
	COLORREF mCustomColors[MAX_CUSTOM_COLORS];
	AccelTable mAccelTable;

	//these aren't persisted, but need to be shared
	CString mTempClip;
	CString mLastClipboard;
	HICON mMacroIcon;

	~ConfigData() {
		SafeDestroyIcon(mMacroIcon);
	}

	void InitDefaultValues();
	bool DoPersist(bool load, bool reload);
	void Load() {DoPersist(true, false);}
	void Save() {DoPersist(false, false);}
	bool ReloadIfModified(EditCtrl& edit);
	void SetEdit(EditCtrl& edit);
	bool CanSwitchClipboard() const;
	void SwitchClipboard(HWND hwnd);
	bool CanLastClipboard() const;
	void LastClipboard(HWND hwnd);

	bool Execute(const Command& cmd, const CString& fileName, HWND hwnd, EditCtrl& edit) const;
	void ExecuteHelp(HelpFile helpFile, HWND hwnd) const;
	void LaunchURL(const CString& url, HWND hwnd) const;

	long GetOpenFilterIndex() const {return mLangMgr.FilterNameToFilterIndex(mOpenFilterName);}
	void SetOpenFilterIndex(long index) {mOpenFilterName = mLangMgr.FilterIndexToFilterName(index);}

	// returns the new line type to save the file with given the option
	// specified by newLineType
	NewLineType CalcNewLineType(NewLineType newLineType) const {
		if (newLineType == nlDefault)
			return mDefaultNewLineType;
		else
			return newLineType;
	}

private:
	void AddConfigDataForOldVersions(long version, bool convertingOldVersion);
	CString mCommandFileName;

	Command GetOldWindowsExplorerCommand() const {
		return Command(TEXT("explorer.exe"), TEXT("&Windows Explorer"), TEXT("/e, ") + Quote(FileDirMacro), EmptyStr, false, 'E', kmCtrl);
	}

	Command GetWindowsExplorerCommand() const {
		CString winExplCmd = _T("/c if exist \"%FilePath%\" (explorer /e,/select,\"%FilePath%\") else (explorer /e,\"%FileDir%\")");
		return Command(mCommandFileName, TEXT("&Windows Explorer"), winExplCmd, EmptyStr, false, 'E', kmCtrl, true);
	}

	bool GetConfigFileModified();

	FILETIME mConfigFileModifyTime;
};

extern ConfigData gConfigData;

#endif