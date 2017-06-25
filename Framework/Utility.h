#ifndef UtilityH
#define UtilityH

#include <ShlWApi.h>
#include <vector>
#include <algorithm>
using namespace std;

class Persist;
class StringVector;

/*** macros ***/

#ifndef WM_KICKIDLE
	#define WM_KICKIDLE 0x036A
#endif

#ifndef OFN_DONTADDTORECENT
	#define OFN_DONTADDTORECENT 0x02000000
#endif

// for some reason these aren't part of VC6, so I define them myself
#ifdef _UNICODE
	#define _tstoi      _wtoi
	#define _tstol      _wtol
#else
	#define _tstoi      atoi
	#define _tstol      atol
#endif

#if _DEBUG
	#define DEBUG_DISPLAY_LAST_ERROR(success, activity) \
		if (!(success)) {  \
			DWORD err = GetLastError(); \
			CString msg; \
			CString activityStr = activity; \
			msg.Format(TEXT("DEBUG ERROR MESSAGE\n\nThe following error occurred while %s:\n\n%s\nError code: 0x%X"), (LPCTSTR)activityStr, GetErrorMessage(err), err); \
			DisplayError(msg); \
		}

	#define VERIFY_MSG(func, activity) DEBUG_DISPLAY_LAST_ERROR(func, activity)
#else
	#define DEBUG_DISPLAY_LAST_ERROR(success, activity) ((void)0)
	#define VERIFY_MSG(func, activity) func
#endif

#define VERSUCC(x) VERIFY(SUCCEEDED(x))

/*** template functions ***/

template<class T>
T Min(const T& val1, const T& val2) {
	return val1 < val2 ? val1 : val2;
}

template<class T>
T Max(const T& val1, const T& val2) {
	return val1 > val2 ? val1 : val2;
}

//returns true if b1 and b2 are either both true or both false
template<class Bool1, class Bool2>
bool BoolEq(Bool1 b1, Bool2 b2) {
	return (b1 && b2) || (!b1 && !b2);
}

template<class T>
void MemSet(T& data, BYTE value) {
	memset(&data, value, sizeof(data));
}

template<class T>
void MemClear(T& data) {
	MemSet(data, 0x00);
}

template<class T>
void InitSized(T& data) {
	MemClear(data);
	data.cbSize = sizeof(data);
}

template<class T>
void InitLengthed(T& data) {
	MemClear(data);
	data.length = sizeof(data);
}

template<class T>
T Sign(const T& val) {
	if (val < 0)
		return -1;
	else if (val > 0)
		return 1;
	else
		return 0;
}

extern LPCTSTR EmptyStr;
extern LPCTSTR ErrorStr;

/*** key related functions ***/

typedef BYTE VirtualKey;
typedef BYTE KeyModifiers;
const KeyModifiers kmNone = 0x0;
const KeyModifiers kmShift = 0x1;
const KeyModifiers kmCtrl = 0x2;
const KeyModifiers kmAlt = 0x4;

inline bool IsKeyDown(VirtualKey vk) {return (GetKeyState(vk) & 0x8000) != 0;}
bool IsAsyncKeyDown(VirtualKey vk);
KeyModifiers GetKeyModifiers();
CString KeyToString(VirtualKey vk);
CString KeyAndModifiersToString(VirtualKey vk, KeyModifiers mods);

//for conversion between KeyModifiers and the modifiers for a CHotKeyCtrl
WORD ToHotKeyMods(KeyModifiers mods);
KeyModifiers FromHotKeyMods(WORD mods);

/*** string related functions ***/

inline CString CharToStr(TCHAR ch) {return ch == NULL ? EmptyStr : CString(ch);}
inline bool IsVarChar(TCHAR ch) {return _istalnum(ch) || ch == '_';}
inline bool IsWordChar(TCHAR ch) {return _istalpha(ch) || ch == '_';}

inline int StrCmpEx(LPCTSTR s1, LPCTSTR s2, bool matchCase) {
	return matchCase ? StrCmp(s1, s2) : StrCmpI(s1, s2);
}

inline bool ChrCmp(TCHAR ch1, TCHAR ch2, bool ignoreCase) {
	return ignoreCase ? _totlower(ch1) == _totlower(ch2) : ch1 == ch2;
}

inline bool IsNbSpace(TCHAR ch) {return _istspace(ch) && ch != '\n';}

inline bool IsNewLine(TCHAR ch) {return ch == '\n' || ch == '\r';}

void AppendString(CString& string, const CString&, LPCTSTR separator);
long GetNumCharsFit(CDC& dc, LPCTSTR string, long length, long maxWidth);
long GetPrevWordPos(LPCTSTR string, long pos, bool eatSpaces);
long GetNextWordPos(const CString& string, long pos, bool eatSpaces);
long GetPrevWordBreakPos(LPCTSTR string, long pos);
long GetNextWordBreakPos(LPCTSTR string, long length, long pos);
inline long GetNextWordBreakPos(const CString string, long pos) {return GetNextWordBreakPos(string, string.GetLength(), pos);}
bool IsValidIdentifier(const CString& string);
long FindFirstDiff(LPCTSTR s1, LPCTSTR s2);
void DiffStrings(const CString& s1, const CString& s2, long& diffStart, long& diff1Len, long& diff2Len);
void GetLogFont(const CFont& font, LOGFONT& lf);
HFONT CloneFont(const CFont& font);
bool FontsEqual(const CFont& font1, const CFont& font2);
long PointSizeToLogicalHeight(HDC hdc, long pointSize);
void MakeTitleCase(CString& string);
void MakeToggleCase(CString& string);
CString DupChar(TCHAR ch, long length);
void StringListToStringVector(const CString& list, LPCTSTR separator, StringVector& vec);
CString EscapeChars(const CString& string);
CString UnescapeChars(const CString& string);
CString GetErrorMessage(DWORD errorCode);
TCHAR GetMnemonic(const CString& string);
inline CString Quote(const CString& string) {return TEXT("\"") + string + TEXT("\"");}
inline bool StartsWith(const CString& string, const CString& start) {return string.Left(start.GetLength()) == start;}
inline bool EndsWith(const CString& string, const CString& ending) {return string.Right(ending.GetLength()) == ending;}
void WrapLines(const CString& text, StringVector& wrappedLines, const long maxCharsPerLine);

inline long GetStrCharsSize(const CString& string) {
	return sizeof(TCHAR) * string.GetLength();
}

inline long GetStrAllocSize(const CString& string) {
	return sizeof(TCHAR) * (string.GetLength() + 1); // + 1 for NULL terminator
}

/*** registry related functions ***/

const REGSAM KEY_WOW64_32KEY = 0x0200;

inline long RegSetStringValue(HKEY key, LPCTSTR valueName, const CString& value) {
	return RegSetValueEx(key, valueName, 0, REG_SZ, (BYTE*)(LPCTSTR)value, GetStrAllocSize(value));
}

inline long RegSetDWordValue(HKEY key, LPCTSTR valueName, DWORD value) {
	return RegSetValueEx(key, valueName, 0, REG_DWORD, (BYTE*)&value, sizeof(value));
}

long RegGetDWordValue(HKEY key, LPCTSTR valueName, DWORD& value);
long RegGetStringValue(HKEY key, LPCTSTR valueName, CString& value);

/*** math related functions ***/

enum RoundKinds {rkUp, rkDown, rkOff};
long Div(long num1, long num2, RoundKinds rk);

/*** menu related functions ***/

HMENU LoadSubMenu(int menuId, int pos);
void SetMenuItemEnabled(HMENU hMenu, UINT itemId, bool enable, UINT byPosOrCommand = MF_BYCOMMAND);
void SetSubMenu(HMENU menu, int subPos, HMENU subMenu);
int FindMenuItemPosById(HMENU hMenu, UINT id);
void CleanupMenuItemName(CString& name);

/*** clipboard related functions ***/

bool GetClipboardString(HWND hwnd, CString& text, bool convertFromCRLF);
bool SetClipboardString(HWND hwnd, const CString& text, bool convertToCRLF);

bool GetClipboardFiles(HWND hwnd, StringVector& files);
bool GetClipboardFilesString(HWND hwnd, CString& text, bool convertToCRLF);

/*** time related functions ***/

CString GetSystemTimeString(bool longTime);
CString GetSystemDateString(bool longDate);

/*** path related functions ***/

// There are 3 ways Windows API path functions deal with buffers:
// 1. Most functions provide a way to determine the buffer size so you can
// allocate a buffer of the correct size. I do NOT just use a buffer of MAX_PATH
// for these functions because it should be possible for them to supports paths
// longer than MAX_PATH (at least eventually even if they don't currently).
// 2. Some function assume the buffer is at least MAX_PATH. SHGetPathFromIDList
// and SHGetSpecialFolderPath do this. Such functions could never work with
// paths longer than MAX_PATH without breaking existing code, so I just use MAX_PATH
// for these functions.
// 3. Some functions input a buffer and size (and therefore do not assume the
// buffer is MAX_PATH), but don't provide a way to return the size needed.
// GetModuleFileName does this. I created BIG_MAX_PATH for these functions
// because it's possible for them to supports paths longer than MAX_PATH,
// though I have to pick some maximum size.
const BIG_MAX_PATH = Max(MAX_PATH, 1024);

enum NewLineType {nlDefault = -1, nlCRLF = 0, nlCR, nlLF};//CR is '\r' and LF is '\n'
enum FileFormat {ffBinary, ffAscii, ffUTF16, ffUTF8};

CString NewLineTypeToString(NewLineType newLineType);
CString FileFormatToString(FileFormat fileFormat);
CString FileToString(const CString& fileName, bool convertNewLinesToLFs = true, FileFormat* fileFormat = NULL, NewLineType* newLineType = NULL, bool* uniformNewLines = NULL);
void StringToFile(const CString& fileName, const CString& data, FileFormat fileFormat = ffAscii);
CString ReadHandle(HANDLE handle, bool convertNewLinesToLFs = true);
void WriteHandle(HANDLE handle, const CString& data);
bool FileExists(const CString& name);
bool AbsolutePathExists(const CString& path);
CString MakePath(const CString& dir, const CString& fileName);
CString GetDir(const CString& path);
CString GetFileNameWithExt(const CString& path);
CString GetFileNameNoExt(const CString& path);
bool HasExt(const CString& path);
CString GetFileExt(const CString& path);
static CString InvalidFileNameChars = TEXT("\\/*:?\"<>|");
bool IsValidFileName(const CString& fileName);
static CString InvalidFileExtChars = InvalidFileNameChars + '.';
bool IsValidFileExt(const CString& ext);
CString GetInvalidExtErrorMessage(const CString& ext);
FILETIME GetFileModifyTime(const CString& fileName);
bool SearchForFile(CString& fileName);
void BreakPath(const CString& path, CString& dir, CString& name, CString& ext);
CString ExpandEnvironmentStrings(const CString& string);
bool BrowseForFolder(HWND hwnd, CString& path, UINT flags = 0);
HRESULT CreateLink(LPCTSTR lpszPathObj, LPCTSTR lpszPathLink, LPCTSTR lpszDesc);
void GetSpecialFolderPath(int csidl, TCHAR path[]);
bool GetSpecialFolderPath(int csidl, CString& path);
bool CreateDirPath(const CString& dir);
bool UninstallFile(const CString& path);
bool GetFileReadOnly(const CString& path);
void SetFileReadOnly(const CString& path, bool readOnly);
long GetNumDigits(long num);
void FindAllFiles(const CString& fileName, StringVector& vec, bool includeFullPath, bool keepExistingItems = false);
CString GetUserName();
void AllowAllAccess(const CString& file);
CString GetCurrentDirectory();
CString DragQueryFile(HDROP hDrop, UINT i);
CString GetFullPathName(const CString& fileName);

/*** windows related functions ***/

void MoveWindowBy(CWnd& wnd, int dx, int dy);
bool FindItemByData(const CComboBox& comboBox, DWORD itemData, int& index);
void SetListBoxItemText(CListBox& listBox, int index, const CString& text);
void StringVectorToComboBox(CComboBox& comboBox, const StringVector& vec);
CRect GetMonitorRect(HWND hwnd);
void LinkifyText(CRichEditCtrl& richEdit, const CString& text);

inline void SafeDestroyIcon(HICON& icon) {
	if (icon != NULL) {
		VERIFY(DestroyIcon(icon));
		icon = NULL;
	}

}

// sets error1 to error2 if that will not overwrite any error already in error1
inline void SetError(long& error1, long error2) {
	if (error1 == ERROR_SUCCESS)
		error1 = error2;
}

/*** search related functions ***/

//I define both ffMatchCase and ffIgnoreCase as non-zero so you have to be explicit and
//pass one (i.e. no default on case sensitivity).
typedef BYTE FindFlags;
const FindFlags ffMatchCase = 0x1;
const FindFlags ffIgnoreCase = 0x2;
const FindFlags ffWholeWord = 0x4;
const FindFlags ffReverse = 0x8;
const FindFlags ffIgnoreCRs = 0x10;//ignore CRs in search string; CRs in find string are NOT ignored

long FindSubstring(const CString& search, const CString& find, FindFlags flags, long start = 0, long end = -1, long* findEnd = NULL);
long CyclicFindSubstring(const CString& search, const CString& find, FindFlags flags, long start, long end, long* findEnd = NULL);
long ReplaceSubstring(CString& string, const CString& find, const CString& replace, FindFlags flags, long start = 0, long maxReplacments = -1, long posToUpdate[] = NULL, int numPosToUpdate = 0);

long FindOneOf(const CString& string, const CString& charsToFind, long start);
long ReverseFindOneOf(const CString& string, const CString& charsToFind, long start);

/*** new line related functions ***/

void GetNumNewLines(const CString& string, long& numCRs, long& numLFs, long start = 0, long end = -1);
long GetNumNewLines(const CString& string, long start = 0, long end = -1);
long GetNumOccurrences(const CString& string, TCHAR ch, long start = 0, long end = -1);
long GetNumOccurrences(const CString& string, const CString& chars, long start = 0, long end = -1);
void ConvertCRLFIndicesToLFIndices(const CString& string, long* indices, long numIndices, long conversionStartPos = 0, long conversionEndPos = -1);
void ConvertLFIndicesToCRLFIndices(const CString& string, long* indices, long numIndices, long convertedIndexBase = 0);
long ConvertLFPosToCRLFPos(const CString& text, long pos, long convertedPosBase = 0, bool allowPosPastEndOfText = false);

void AdjustIndices(long pos, long* indicesToAdjust, const long numIndicesToAdjust, long adjustAmount);

void ConvertCRLFtoLF(CString& string);
void ConvertCRLFtoLF(LPTSTR dest, LPCTSTR src);

CString ConvertLFtoCRLF(const CString& string, long lineCount = -1);
void ConvertLFtoCRLF(LPTSTR dest, LPCTSTR src);

void ConvertLFtoCR(CString& string);
void ConvertLFtoCR(LPTSTR dest, LPCTSTR src);

long ConvertAnyToLF(CString& string, NewLineType* newLineType = NULL, bool* uniformNewLines = NULL);
long ConvertAnyToLF(LPTSTR dest, LPCTSTR src, NewLineType* newLineType = NULL, bool* uniformNewLines = NULL);

long RemoveCR(CString& string, long* indicesToAdjust = NULL, long numIndicesToAdjust = 0);
long RemoveCR(LPTSTR dest, LPCTSTR src, long* indicesToAdjust = NULL, long numIndicesToAdjust = 0);

bool ConvertEmbeddedNulls(char* ptr, int len);

/*** utility classes ***/

class Bool {
private:
	bool m_value;

public:
	Bool(bool value) : m_value(value) {}
	Bool(BOOL value) : m_value(value != FALSE) {}
	operator bool() {return m_value;}
	operator BOOL() {return m_value ? TRUE : FALSE;}
};

template<class T>
class Vector : public vector<T> {
protected:
	const T& Item(size_t i) const {return (*this)[i];}
	T& Item(size_t i) {return (*this)[i];}

public:
	iterator find(const T& item) {return std::find(begin(), end(), item);}
	const_iterator find(const T& item) const {return std::find(begin(), end(), item);}
};

class StringVector : public Vector<CString> {
public:
	void GetLineRange(long line, long& start, long &end) const;
	long GetTotalLength() const;
	void LineFromChar(long& lineOffset, long& lineNum, long pos) const;
	long FindString(const CString& string, bool matchCase) const;
	bool HasString(const CString& string, bool matchCase) const {return FindString(string, matchCase) >= 0;}
	void RemoveAllOccurrencesOf(const CString& string, bool matchCase);
	void RemoveDuplicates(bool matchCase);
	CString Concat(LPCTSTR separator, bool includeEmptyItems = false) const;
};

class StringIterator {
public:
	virtual long GetNumStrings() const = 0;
	virtual const CString& GetNthString(long i) const = 0;
};

long FindMatch(const StringIterator& vec, const CString& string, bool matchCase);

enum StringToken {stNone, stIdentifier, stWhiteSpace, stSymbol};
long GetNumTokenCount(const CString& string);
StringToken GetNextToken(const CString& string, long start, CString& token);

class WaitCursor {
private:
	HCURSOR mOldCursor;
	bool mShowWaitCursor;

public:
	WaitCursor(bool showWaitCursor = true) : mShowWaitCursor(showWaitCursor) {
		if (mShowWaitCursor)
			mOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	}

	~WaitCursor() {
		if (mShowWaitCursor)
			SetCursor(mOldCursor);
	}
};

struct Accel : ACCEL {
	Accel();
	Accel(VirtualKey vkey, KeyModifiers mods, WORD command);

	VirtualKey GetVirtualKey() const {return (VirtualKey)key;}
	KeyModifiers GetModifiers() const;
	void DoPersist(Persist& p);
};

class AccelTable {
private:
	HACCEL m_handle;
	vector<Accel> m_accels;

	void CreateIfNecessary() {
		if (m_handle == NULL)
			m_handle = CreateAcceleratorTable(m_accels.begin(), m_accels.size());
	}

	void Destroy() {
		if (m_handle != NULL) {
			VERIFY(DestroyAcceleratorTable(m_handle));
			m_handle = NULL;
		}
	}

	vector<Accel>::const_iterator FindAccelByCmd(UINT cmd) const;

public:
	AccelTable() : m_handle(NULL) {}
	AccelTable(const AccelTable& rhs) {*this = rhs;}
	~AccelTable() {Destroy();}

	AccelTable& operator=(const AccelTable& rhs) {
		m_accels = rhs.m_accels;
		Destroy();
		return *this;
	}

	operator HACCEL() {
		CreateIfNecessary();
		return m_handle;
	}

	void AddAccel(const Accel& accel) {
		Destroy();
		m_accels.push_back(accel);
	}

	void Clear() {
		Destroy();
		m_accels.clear();
	}

	bool GetHotKey(UINT cmd, VirtualKey& key, KeyModifiers& mods) const;
	UINT FindHotKey(VirtualKey key, KeyModifiers mods) const;
	void DecorateMenu(HMENU hMenu);
	void DoPersist(Persist& p);
};

//there should only be one of these for every menu
class CommandIdCreator {
private:
	const UINT mBase;
	UINT mCurNewId;
	vector<UINT> mOldIds;

	//Don't copy this thing. Pass it around by reference.
	CommandIdCreator(const CommandIdCreator&);
	CommandIdCreator& operator=(const CommandIdCreator&);

public:
	//initialize mBase to a high enough base value to not duplicate
	//anything the resource editor will generate
	CommandIdCreator() : mBase(40000), mCurNewId(mBase) {}

	UINT CreateId();
	void ReleaseId(UINT id);
	void ReleaseSubMenuItemsIds(HMENU hMenu);
};

class AutoBool {
private:
	bool mOldValue;
	bool& mBool;
public:

	AutoBool(bool& _bool, bool _value) : mBool(_bool) {
		mOldValue = mBool;
		mBool = _value;
	}

	~AutoBool() {mBool = mOldValue;}
};

template<class T>
class AutoIncDec {
private:
	T& mValue;

public:
	AutoIncDec(T& value) : mValue(value) {
		++mValue;
	}

	~AutoIncDec() {
		--mValue;
	}
};

class ModuleLoader {
private:
	HMODULE mModule;

public:
	ModuleLoader(LPCTSTR libFileName) {
		mModule = LoadLibrary(libFileName);
		ASSERT(mModule != NULL);
	}

	~ModuleLoader() {
		VERIFY(FreeLibrary(mModule));
	}

	operator HMODULE() const {return mModule;}

	FARPROC GetProcAddress(LPCSTR procName) const {
		return
			mModule == NULL ?
			NULL :
			::GetProcAddress(mModule, procName);
	}
};

inline void DisplayError(const CString& msg, HWND hwnd = NULL) {
	MessageBox(hwnd, msg, ErrorStr, MB_OK|MB_ICONERROR);
}

typedef long ErrorCodes;
const ErrorCodes ErrNewerFileVersion = 1;
const ErrorCodes ErrPersistedDataNotFound = 2;
const ErrorCodes ErrApplicationBase = 10000;

//This is an exception class for exceptions thrown by the application framework.
//You can use this class for applicatation specific errors by inheriting from it,
//defining error codes starting at ErrApplicationBase, and overwritting the method
//Exception::GetErrorMessage(). The advantage of inheriting from this class for
//application specific errors is that the same catch can be used for application and
//framework errors.
class Exception {
public:
	Exception(ErrorCodes errorCode, const CString& message = EmptyStr) :
	  mErrorCode(errorCode), mMessage(message) {}

	virtual ~Exception() {}

	const ErrorCodes mErrorCode;

	virtual CString GetErrorMessage() const;

	//this method should not be called if the error message needs arguments,
	//e.g. the message contains the format specifier %s.
	void DisplayDefaultError(HWND hwnd = NULL) const {
		DisplayError(GetErrorMessage(), hwnd);
	}

private:
	CString mMessage;
};

//This class handles lite edit specific command line arguments.
//To add a new command line argument:
//  1. Modify ParseParam to detect the argument
//  2. Modify CLiteEditDlg::ProcessCommandLine() to handle the argument
//  3. Modify CLiteEditDlg::OnInitDialog() where mCmdLineInfo.mShowUsage
//     is handled the document the argument
class LiteEditCommandLineInfo : public CCommandLineInfo {
private:
	typedef CCommandLineInfo Base;
	enum ParseStates {psInit, psGoto};
	ParseStates mParseState;

public:
	bool mShowUsage;
	long mLineNumber, mColumnNumber;

	LiteEditCommandLineInfo() :
		mShowUsage(false), mParseState(psInit),
		mLineNumber(0), mColumnNumber(0) {}

	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);
};

class AutoGlobal {
private:
	HGLOBAL mGlobal;

	AutoGlobal(const AutoGlobal& rhs);
	AutoGlobal& operator=(const AutoGlobal& rhs);

public:
	AutoGlobal(HGLOBAL global) : mGlobal(global) {}
	~AutoGlobal() {VERIFY(GlobalFree(mGlobal) == NULL);}
};

class AutoHandle {
private:
	HANDLE mHandle;

	AutoHandle(const AutoHandle& rhs);
	AutoHandle& operator=(const AutoHandle& rhs);

public:
	AutoHandle(HANDLE handle = NULL) : mHandle(handle) {}

	~AutoHandle() {
		Close();
	}

	void Close() {
		if (mHandle != NULL) {
			VERIFY(CloseHandle(mHandle));
			mHandle = NULL;
		}
	}

	AutoHandle& operator=(const HANDLE& rhs) {
		Close();
		mHandle = rhs;
		return *this;
	}

	operator HANDLE() {return mHandle;}
};

template<class T>
class AutoArray {
private:
	T* mArray;

	AutoArray(const AutoArray<T>& rhs);
	AutoArray& operator=(const AutoArray<T>& rhs);

public:
	AutoArray(T* array) : mArray(array) {}

	~AutoArray() {
		delete [] mArray;
	}
};

class Version {
private:
	ULONG mMajor, mMinor, mRevision;
	mutable CString mString;

public:
	Version(ULONG major = 0, ULONG minor = 0, ULONG revision = 0) :
	  mMajor(major), mMinor(minor), mRevision(revision) {}

	ULONG GetMajor() const {return mMajor;}
	ULONG GetMinor() const {return mMinor;}
	ULONG GetRevision() const {return mRevision;}

	CString AsString() const;

	int Compare(const Version& rhs) const;

	bool operator==(const Version& rhs) const {
		return Compare(rhs) == 0;
	}

	bool operator<(const Version& rhs) const {
		return Compare(rhs) < 0;
	}

	bool operator>(const Version& rhs) const {
		return Compare(rhs) > 0;
	}

	bool operator<=(const Version& rhs) const {
		return Compare(rhs) <= 0;
	}

	bool operator>=(const Version& rhs) const {
		return Compare(rhs) >= 0;
	}
};

#endif