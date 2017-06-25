#include "StdAfx.h"
#include "Utility.h"
#include "Persist.h"
#include "ShlObj.h"
#include "MultiMon.h"

LPCTSTR EmptyStr = TEXT("");
LPCTSTR ErrorStr = TEXT("Error");

const char UTF16BOMBytes[] = {'\xFF', '\xFE'};
const char UTF8BOMBytes[] = {'\xEF', '\xBB', '\xBF'};

void StringVector::GetLineRange(long line, long& start, long &end) const {
	ASSERT(0 <= line && line < size());
	start = 0;
	for (long i = 0; i <= line; ++i)
		if (i < line)
			start += Item(i).GetLength();
		else
			end = start + Item(i).GetLength();
}

long StringVector::GetTotalLength() const {
	long result = 0;

	for (size_type i = 0; i < size(); ++i)
		result += Item(i).GetLength();

	return result;
}

void StringVector::LineFromChar(long& lineOffset, long& lineNum, long pos) const {
	ASSERT(0 <= pos && pos <= GetTotalLength());
	lineOffset = 0;
	lineNum = -1;

	for (size_type i = 0; i < size(); ++i) {
		long length = Item(i).GetLength();

		if (i < size() - 1) {
			if (pos < length) {
				lineNum = i;
				break;
			}
		} else {
			if (pos <= length) {
				lineNum = i;
				break;
			}
		}

		lineOffset += length;
		pos -= length;
	}
}

long StringVector::FindString(const CString& string, bool matchCase) const {
	for (size_t i = 0; i < size(); ++i)
		if (StrCmpEx(Item(i), string, matchCase) == 0)
			return i;

	return -1;
}

void StringVector::RemoveAllOccurrencesOf(const CString& string, bool matchCase) {
	size_t i = 0;
	while (i < size())
		if (StrCmpEx(Item(i), string, matchCase) == 0)
			erase(begin() + i);
		else
			++i;
}

void StringVector::RemoveDuplicates(bool matchCase) {
	size_t i1 = 0;
	while (i1 < size()) {
		size_t i2 = i1 + 1;
		while (i2 < size())
			if (StrCmpEx(Item(i1), Item(i2), matchCase) == 0)
				erase(begin() + i2);
			else
				++i2;

		++i1;
	}
}

CString StringVector::Concat(LPCTSTR separator, bool includeEmptyItems/* = false*/) const {
	CString result;

	bool addSeparator = false;

	for (size_t i = 0; i < size(); ++i) {
		const CString& item = Item(i);

		if (!item.IsEmpty() || includeEmptyItems) {
			if (addSeparator)
				result += separator;

			result += item;
			addSeparator = true;
		}
	}

	return result;
}

bool IsAsyncKeyDown(VirtualKey vk) {
	if (vk == VK_LBUTTON && GetSystemMetrics(SM_SWAPBUTTON))
		vk = VK_RBUTTON;
	else if (vk == VK_RBUTTON && GetSystemMetrics(SM_SWAPBUTTON))
		vk = VK_LBUTTON;

	return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

KeyModifiers GetKeyModifiers() {
	KeyModifiers val = kmNone;

	if (IsKeyDown(VK_SHIFT))
		val |= kmShift;
	if (IsKeyDown(VK_CONTROL))
		val |= kmCtrl;
	if (IsKeyDown(VK_MENU))
		val |= kmAlt;

	return val;
}

CString KeyToString(VirtualKey vk) {
	if (vk == 0)
		return EmptyStr;

	LPARAM lParam = 0;
	lParam |= MapVirtualKey(vk, 0) << 16;
	lParam |= 1 << 25;//consider left and right of modifier keys the same

	CString result;
	const long SIZE = 20;
	LPTSTR str = result.GetBuffer(SIZE);
	VERIFY(GetKeyNameText(lParam, str, SIZE) != 0);
	result.ReleaseBuffer();
	return result;
}

CString KeyAndModifiersToString(VirtualKey vk, KeyModifiers mods) {
	if (vk == 0)
		return EmptyStr;

	LPCTSTR separator = TEXT("+");
	CString result;

	if ((mods & kmCtrl) != 0)
		AppendString(result, KeyToString(VK_CONTROL), separator);
	if ((mods & kmShift) != 0)
		AppendString(result, KeyToString(VK_SHIFT), separator);
	if ((mods & kmAlt) != 0)
		AppendString(result, KeyToString(VK_MENU), separator);

	AppendString(result, KeyToString(vk), separator);
	return result;
}

WORD ToHotKeyMods(KeyModifiers mods) {
	WORD result = 0;
	if ((mods & kmAlt) != 0)
		result |= HOTKEYF_ALT;
	if ((mods & kmCtrl) != 0)
		result |= HOTKEYF_CONTROL;
	if ((mods & kmShift) != 0)
		result |= HOTKEYF_SHIFT;
	return result;
}

KeyModifiers FromHotKeyMods(WORD mods) {
	KeyModifiers result = kmNone;
	if ((mods & HOTKEYF_ALT) != 0)
		result |= kmAlt;
	if ((mods & HOTKEYF_CONTROL) != 0)
		result |= kmCtrl;
	if ((mods & HOTKEYF_SHIFT) != 0)
		result |= kmShift;
	return result;
}

void AppendString(CString& string, const CString& append, LPCTSTR separator) {
	if (!string.IsEmpty() && !append.IsEmpty())
		string += separator;
	string += append;
}

long GetNumCharsFit(CDC& dc, LPCTSTR string, long length, long maxWidth) {
	static SIZE szDummy;
	maxWidth = Max(0L, maxWidth);
	INT fit = 0;
	VERIFY(GetTextExtentExPoint(dc, string, length, maxWidth, &fit, NULL, &szDummy));

	for (long i = 0; i < fit; ++i)
		if (string[i] == '\n')
			return i + 1;

	return fit;
}

long GetPrevWordPos(LPCTSTR string, long pos, bool eatSpaces) {
	ASSERT(0 <= pos && pos <= lstrlen(string));

	if (eatSpaces) {
		if (pos > 0 && string[pos-1] == '\n')
			--pos;

		while (pos > 0 && IsNbSpace(string[pos-1]))
			--pos;
	}

	if (pos > 0) {
		bool isVarChar = IsVarChar(string[pos-1]);
		while (
			pos > 0 &&
			isVarChar == IsVarChar(string[pos-1]) &&
			!_istspace(string[pos-1]))
			--pos;
	}

	return pos;
}

long GetNextWordPos(const CString& string, long pos, bool eatSpaces) {
	ASSERT(0 <= pos && pos <= string.GetLength());

	if (eatSpaces) {
		if (pos < string.GetLength() && string.GetAt(pos) == '\n')
			++pos;

		// removing the code below because it causes Ctrl+Right to eat
		// spaces before and after a word, which seem unintuitive when
		// there's multiple spaces before a word
#if 1
		while (pos < string.GetLength() && IsNbSpace(string.GetAt(pos)))
			++pos;
#endif
	}

	if (pos < string.GetLength()) {
		bool isVarChar = IsVarChar(string.GetAt(pos));
		while (
			pos < string.GetLength() &&
			isVarChar == IsVarChar(string.GetAt(pos)) &&
			!_istspace(string.GetAt(pos)))
			++pos;
	}

	//trailing spaces are considered part of the word
	if (eatSpaces)
		while (pos < string.GetLength() && IsNbSpace(string.GetAt(pos)))
			++pos;

	return pos;
}

long GetPrevWordBreakPos(LPCTSTR string, long pos) {
	ASSERT(0 <= pos && pos <= lstrlen(string));

	while (pos > 0 && IsNbSpace(string[pos-1]))
		--pos;
	
	while (pos > 0 && !IsNbSpace(string[pos-1]))
		--pos;

	return pos;
}

long GetNextWordBreakPos(LPCTSTR string, long length, long pos) {
	ASSERT(0 <= pos && pos <= length);

	while (pos < length && !IsNbSpace(string[pos]))
		++pos;

	while (pos < length && IsNbSpace(string[pos]))
		++pos;
	
	return pos;
}

bool IsValidIdentifier(const CString& string) {
	ASSERT(!string.IsEmpty());
	if (string[0] != '_' && !_istalpha(string[0]))
		return false;

	for (int i = 1; i < string.GetLength(); ++i)
		if (!IsVarChar(string[i]))
			return false;

	return true;
}

long FindFirstDiff(LPCTSTR s1, LPCTSTR s2) {
	long result = 0;

	while (*s1 != NULL && *s2 != NULL) {
		if (*s1 != *s2)
			return result;

		++result;
		++s1;
		++s2;
	}

	return *s1 == NULL && *s2 == NULL ? -1 : result;
}

void DiffStrings(const CString& s1, const CString& s2, long& diffStart, long& diff1Len, long& diff2Len) {
	diffStart = FindFirstDiff(s1, s2);

	if (diffStart >= 0) {
		//diff1End and diff2End are one past the last difference in s1 and s2.
		long diff1End = s1.GetLength(), diff2End = s2.GetLength();
		while (diff1End > diffStart && diff2End > diffStart && s1[diff1End-1] == s2[diff2End-1]) {
			--diff1End;
			--diff2End;
		}

		diff1Len = diff1End - diffStart;
		diff2Len = diff2End - diffStart;
	} else
		diff1Len = diff2Len = 0;
}

//this wrapper takes care of a const_cast on font and guarantees that lf is initialized
void GetLogFont(const CFont& font, LOGFONT& lf) {
	MemClear(lf);
	VERIFY(const_cast<CFont&>(font).GetLogFont(&lf));
}

HFONT CloneFont(const CFont& font) {
	if ((HFONT)font == NULL)//default font
		return NULL;

	LOGFONT lf;
	GetLogFont(font, lf);
	return CreateFontIndirect(&lf);
}

bool FontsEqual(const CFont& font1, const CFont& font2) {
	if (font1 == font2)
		return true;

	if ((HFONT)font1 == NULL || (HFONT)font2 == NULL)
		return false;

	LOGFONT lf1, lf2;
	GetLogFont(font1, lf1);
	GetLogFont(font2, lf2);

	return memcmp(&lf1, &lf2, sizeof(LOGFONT)) == 0;
}

long PointSizeToLogicalHeight(HDC hdc, long pointSize) {
	CDC dc;
	dc.Attach(hdc);
	int oldMapMode = dc.SetMapMode(MM_TEXT);

	long result = -MulDiv(pointSize, dc.GetDeviceCaps(LOGPIXELSY), 72);

	dc.SetMapMode(oldMapMode);
	dc.Detach();

	return result;
}

void MakeTitleCase(CString& string) {
	bool wordStart = true;
	for (int i = 0; i < string.GetLength(); ++i) {
		TCHAR ch = string.GetAt(i);
		string.SetAt(i, wordStart ? _totupper(ch) : _totlower(ch));

		// consider single quote word start but not apostrophe,
		// the difference is an apostrophe comes immediately after
		// a letter
		if (ch == '\'')
			wordStart = i == 0 || !_istalpha(string.GetAt(i-1));
		else
			wordStart = !_istalpha(ch);
	}
}

void MakeToggleCase(CString& string) {
	for (int i = 0; i < string.GetLength(); ++i) {
		TCHAR ch = string.GetAt(i);
		if (_istalpha(ch)) {
			ch = _istupper(ch) ? _totlower(ch) : _totupper(ch);
			string.SetAt(i, ch);
		}
	}
}

CString DupChar(TCHAR ch, long length) {
	CString result;
	LPTSTR buff = result.GetBuffer(length);
	for (long i = 0; i < length; ++i)
		buff[i] = ch;
	result.ReleaseBuffer(length);
	return result;
}

void StringListToStringVector(const CString& list, LPCTSTR separator, StringVector& vec) {
	int sepLen = _tcsclen(separator);
	ASSERT(sepLen > 0);

	vec.clear();
	int start = 0;
	while (start < list.GetLength()) {
		int end = list.Find(separator, start);
		if (end < 0)
			end = list.GetLength();

		CString item = list.Mid(start, end - start);
		item.TrimLeft(' ');
		item.TrimRight(' ');
		vec.push_back(item);
		start = end + sepLen;
	}
}

//converts \ to \\, tab to \t and new line to \n
CString EscapeChars(const CString& string) {
	CString result;

	for (int i = 0; i < string.GetLength(); ++i) {
		TCHAR ch = string.GetAt(i);
		switch (ch) {
		case '\\' :
			result += "\\\\";
			break;
		case '\n' :
			result += "\\n";
			break;
		case '\t' :
			result += "\\t";
			break;
		default :
			result += ch;
		}
	}

	return result;
}

//converts \\ to \, \t to tab, and \n to new line
CString UnescapeChars(const CString& string) {
	CString result;

	for (int i = 0; i < string.GetLength(); ++i) {
		TCHAR ch = string.GetAt(i);
		TCHAR nextCh = i + 1 < string.GetLength() ? string.GetAt(i+1) : NULL;
		if (ch == '\\' && (nextCh == '\\' || nextCh == 'n' || nextCh == 't')) {
			switch (nextCh) {
			case '\\' :
				result += '\\';
				break;
			case 'n' :
				result += '\n';
				break;
			case 't' :
				result += '\t';
				break;
			default :
				ASSERT(false);
			}
			++i;
		} else
			result += ch;
	}

	return result;
}

CString GetErrorMessage(DWORD errorCode) {
	const DWORD size = 0x400;
	TCHAR buff[size] = TEXT("");
	VERIFY(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, buff, size, NULL) != 0);
	return buff;
}

TCHAR GetMnemonic(const CString& string) {
	CString noAmpersandString = string;
	//remove all occurrences of && which represent a single & in a menu
	//so I can find real mnemonics
	noAmpersandString.Replace(TEXT("&&"), EmptyStr);

	int pos = noAmpersandString.Find('&');
	if (0 <= pos && pos < noAmpersandString.GetLength() - 1)
		return _totupper(noAmpersandString.GetAt(pos + 1));
	else
		return NULL;
}

// wraps based on maxCharsPerLine, not a display size
void WrapLines(const CString& text, StringVector& wrappedLines, const long maxCharsPerLine) {
	wrappedLines.clear();

	StringVector lines;
	StringListToStringVector(text, _T("\n"), lines);

	for (size_t i = 0; i < lines.size(); ++i) {
		long lineStartPos = 0;
		CString line = lines[i];

		// do/while instead of while so empty lines are included
		do {
			long nextLineStartPos = line.GetLength();
			const long maxLineEndPos = lineStartPos + maxCharsPerLine;

			if (nextLineStartPos > maxLineEndPos) {
				nextLineStartPos = GetPrevWordBreakPos(line, maxLineEndPos);

				// if can't break before maxLineEndPos (i.e. one really long word)
				// allow exceeding it
				if (nextLineStartPos <= lineStartPos)
					nextLineStartPos = GetNextWordBreakPos(line, lineStartPos);
			}

			CString wrappedLine = line.Mid(lineStartPos, nextLineStartPos - lineStartPos);
			wrappedLines.push_back(wrappedLine);
			lineStartPos = nextLineStartPos;
		} while (lineStartPos < line.GetLength());
	}
}

long RegGetDWordValue(HKEY key, LPCTSTR valueName, DWORD& value) {
	DWORD type = REG_NONE;
	DWORD size = sizeof(value);
	long ec = RegQueryValueEx(key, valueName, NULL, &type, (BYTE*)&value, &size);

	if (ec == ERROR_SUCCESS)
		if (type != REG_DWORD)
			ec = E_INVALIDARG;
		else if (size != sizeof(value))
			ec = E_FAIL;

	return ec;
}

long RegGetStringValue(HKEY key, LPCTSTR valueName, CString& value) {
	DWORD type = REG_NONE;
	DWORD size1 = 0;
	long ec = RegQueryValueEx(key, valueName, NULL, &type, NULL, &size1);

	if (ec != ERROR_SUCCESS)
		return ec;
	else if (type != REG_SZ)
		return E_INVALIDARG;

	if (size1 <= sizeof(TCHAR)) { // NULL or less
		value = EmptyStr;
		return ERROR_SUCCESS;
	}

	int len1 = size1 / sizeof(TCHAR) - 1;
	LPTSTR buffer = value.GetBuffer(len1);

	DWORD size2 = size1;
	ec = RegQueryValueEx(key, valueName, NULL, &type, (BYTE*)buffer, &size2);
	int len2 = size2 / sizeof(TCHAR) - 1;
	ASSERT(len2 <= len1);
	value.ReleaseBuffer(len2);

	if (ec == ERROR_SUCCESS && type != REG_SZ)
		return E_INVALIDARG;
	else
		return ec;
}

long Div(long num1, long num2, RoundKinds rk) {
	ASSERT(num2 != 0);

	long sign = Sign(num1) * Sign(num2);
	num1 = abs(num1);
	num2 = abs(num2);
	long result = 0;

	switch (rk) {
	case rkUp :
		result = (num1 + num2 - 1) / num2;
		break;
	case rkOff :
		result = (num1 + num2 / 2) / num2;
		break;
	default :
		ASSERT(rk == rkDown);
		result = num1 / num2;
		break;
	}

	return sign * result;
}

HMENU LoadSubMenu(int menuId, int pos) {
	CMenu menu;
	menu.Attach(LoadMenu(NULL, MAKEINTRESOURCE(menuId)));
	CMenu* subMenu = menu.GetSubMenu(pos);
	menu.RemoveMenu(pos, MF_BYPOSITION);//so subMenu won't be destroyed with menu
	return *subMenu;
}

void SetMenuItemEnabled(HMENU hMenu, UINT itemId, bool enable, UINT byPosOrCommand/* = MF_BYCOMMAND*/) {
	ASSERT(byPosOrCommand == MF_BYPOSITION || byPosOrCommand == MF_BYCOMMAND);
	UINT flags = byPosOrCommand;

	if (enable)
		flags |= MF_ENABLED;
	else
		flags |= MF_DISABLED|MF_GRAYED;

	VERIFY(EnableMenuItem(hMenu, itemId, flags) != -1);
}

void SetSubMenu(HMENU menu, int subPos, HMENU subMenu) {
	CMenu m;
	VERIFY(m.Attach(menu));
	CString name;
	VERIFY(m.GetMenuString(subPos, name, MF_BYPOSITION));
	//ModifyMenu() always changes the menu text which is why I pass in name
	VERIFY(m.ModifyMenu(subPos, MF_BYPOSITION|MF_POPUP, (UINT)subMenu, name));
	m.Detach();
}

int FindMenuItemPosById(HMENU hMenu, UINT id) {
	int index = -1;
	CMenu menu;
	VERIFY(menu.Attach(hMenu));

	for (int i = 0; i < menu.GetMenuItemCount(); ++i) {
		ASSERT(menu.GetMenuItemID(i) != (UINT)-1); // need to support submenus?

		if (menu.GetMenuItemID(i) == id) {
			index = i;
			break;
		}
	}

	menu.Detach();
	return index;
}

void CleanupMenuItemName(CString& name) {
	ReplaceSubstring(name, TEXT("&"), EmptyStr, ffIgnoreCase);
	ReplaceSubstring(name, TEXT("..."), EmptyStr, ffIgnoreCase);
}

bool GetClipboardString(HWND hwnd, CString& text, bool convertFromCRLF) {
	bool success = false;

	if (OpenClipboard(hwnd)) {
		UINT clipboardFormat = 0;
		HANDLE handle = NULL;

#ifdef _UNICODE
		clipboardFormat = CF_UNICODETEXT;
		handle = GetClipboardData(clipboardFormat);
		if (handle == NULL)
#endif
		{
			clipboardFormat = CF_TEXT;
			handle = GetClipboardData(clipboardFormat);
		}

		void* data = GlobalLock(handle);

		if (data != NULL) {
			if (clipboardFormat == CF_TEXT)
				text = (LPCSTR)data;
			else
				text = (LPCWSTR)data;

			if (convertFromCRLF)
				ConvertCRLFtoLF(text);

			GlobalUnlock(handle);
			success = true;
		}

		BOOL closeClip = CloseClipboard();
		success = closeClip && success;
	}

	return success;
}

bool SetClipboardString(HWND hwnd, const CString& text, bool convertToCRLF) {
	bool success = false;

	if (OpenClipboard(hwnd)) {
		long extra = 1;//1 for NULL
		if (convertToCRLF)
			extra += GetNumOccurrences(text, '\n');
		DWORD numBytes = sizeof(TCHAR) * (text.GetLength() + extra);
		HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, numBytes);

		LPTSTR buff = (LPTSTR)GlobalLock(hGlobal);
		if (buff != NULL) {
			if (convertToCRLF)
				ConvertLFtoCRLF(buff, text);
			else
				StrCpy(buff, text);

			if (EmptyClipboard()) {
#ifdef _UNICODE
				// Can still be read as CF_TEXT when set as CF_UNICODETEXT. Thanks OS.
				// This behavior is documented in MSDN under
				// Clipboard Formats: Synthesized Clipboard Formats.
				SetClipboardData(CF_UNICODETEXT, hGlobal);
#else
				SetClipboardData(CF_TEXT, hGlobal);
#endif
				success = true;
			}

			GlobalUnlock(hGlobal);
		}

		BOOL closeClip = CloseClipboard();
		success = closeClip && success;
	}

	return success;
}

bool GetClipboardFiles(HWND hwnd, StringVector& files) {
	bool success = false;
	files.clear();

	if (OpenClipboard(hwnd)) {
		HANDLE handle = GetClipboardData(CF_HDROP);
		void* data = GlobalLock(handle);

		if (data != NULL) {
			HDROP hdrop = (HDROP)data;
			UINT count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
			files.reserve(count);

			for (UINT i = 0; i < count; ++i) {
				CString path = DragQueryFile(hdrop, i);

				DWORD attr = GetFileAttributes(path);
				if (attr != -1 && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0)
					files.push_back(path);
			}

			GlobalUnlock(handle);
			success = true;
		}

		BOOL closeClip = CloseClipboard();
		success = closeClip && success;
	}

	return success && files.size() > 0;
}

bool GetClipboardFilesString(HWND hwnd, CString& text, bool convertFromCRLF) {
	StringVector files;
	text = EmptyStr;

	if (GetClipboardFiles(hwnd, files)) {
		for (size_t i = 0; i < files.size(); ++i)
			text += FileToString(files[i], convertFromCRLF);

		return true;
	}

	return false;
}

CString GetSystemTimeString(bool longTime) {
	SYSTEMTIME time;
	GetLocalTime(&time);

	DWORD flags = longTime ? 0 : TIME_NOSECONDS;
	int size = GetTimeFormat(LOCALE_USER_DEFAULT, flags, &time, NULL, NULL, 0);
	ASSERT(size > 0);
	int numChars = size - 1;//-1 to not include NULL
	CString result;
	TCHAR* buff = result.GetBuffer(numChars);
	size = GetTimeFormat(LOCALE_USER_DEFAULT, flags, &time, NULL, buff, size);
	ASSERT(size == numChars + 1);

	result.ReleaseBuffer(numChars);
	return result;
}

CString GetSystemDateString(bool longDate) {
	SYSTEMTIME time;
	GetLocalTime(&time);

	DWORD flags = longDate ? DATE_LONGDATE : DATE_SHORTDATE;
	int size = GetDateFormat(LOCALE_USER_DEFAULT, flags, &time, NULL, NULL, 0);
	ASSERT(size > 0);
	int numChars = size - 1;//-1 to not include NULL
	CString result;
	TCHAR* buff = result.GetBuffer(numChars);
	size = GetDateFormat(LOCALE_USER_DEFAULT, flags, &time, NULL, buff, size);
	ASSERT(size == numChars + 1);

	result.ReleaseBuffer(numChars);
	return result;
}

CString NewLineTypeToString(NewLineType newLineType) {
	switch (newLineType) {
	case nlCR :
		return _T("CR");
	case nlLF :
		return _T("LF");
	case nlCRLF :
		return _T("CR + LF");
	default :
		ASSERT(false);
		return EmptyStr;
	}
}

CString FileFormatToString(FileFormat fileFormat) {
	switch (fileFormat) {
	case ffBinary :
		return _T("unrecognized");
	case ffAscii :
		return _T("ASCII");
	case ffUTF16 :
		return _T("Unicode UTF-16");
	case ffUTF8 :
		return _T("Unicode UTF-8");
	default :
		ASSERT(false);
		return EmptyStr;
	}
}

// Sometimes reading and writing to a file can happen at the same time, resulting
// in an error.
// I was seeing this occasionally with the config file when 2 Lite Edit instances
// where open, you close one, and the other immediately activates and reads the
// config file (due to a change).
// This function works around that problem by retrying to give the other process
// using the file a chance to finish with it.
static void TryOpenFile(CFile& file, const CString& fileName, UINT nOpenFlags) {
	const int TotalWait = 3000;
	const int WaitPerTry = 200;

	int waitSoFar = 0;
	CFileException e;

	while (!file.Open(fileName, nOpenFlags, &e)) {
		if (waitSoFar < TotalWait) {
			Sleep(WaitPerTry);
			waitSoFar += WaitPerTry;
		} else
			AfxThrowFileException(e.m_cause, e.m_lOsError, e.m_strFileName);
	}
}

CString FileToString(const CString& fileName, bool convertNewLinesToLFs/* = true*/, FileFormat* fileFormat/* = NULL*/, NewLineType* newLineType/* = NULL*/, bool* uniformNewLines/* = NULL*/) {
	// shareDenyNone is used so I can open a file being written to. It's useful for
	// files that stay open and kept getting written to.
	CFile file;
	TryOpenFile(file, fileName, CFile::modeRead | CFile::shareDenyNone);
	DWORD len = file.GetLength();

	char* str = new char[len + 2];//+2 for unicode NULL
	AutoArray<char> autoArray(str);
	len = file.Read(str, len);
	str[len] = NULL;//Read() doesn't NULL terminate
	str[len + 1] = NULL;//in case is a unicode file we need to terminate with 2 bytes

	CString result;
	FileFormat ff = ffAscii;
#ifdef _UNICODE
	if (strncmp(str, UTF16BOMBytes, sizeof(UTF16BOMBytes)) == 0)
		ff = ffUTF16;
	else if (strncmp(str, UTF8BOMBytes, sizeof(UTF8BOMBytes)) == 0)
		ff = ffUTF8;

	if (ff == ffUTF16)
		result = (wchar_t*)(str + sizeof(UTF16BOMBytes));
	else if (ff == ffUTF8) {
		char* utf8Str = str + sizeof(UTF8BOMBytes);
		int utf8Size = len + 1 - sizeof(UTF8BOMBytes);
		int utf16Size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8Str, utf8Size, NULL, 0);
		if (utf16Size > 0) {
			LPTSTR buffer = result.GetBuffer(utf16Size - 1);
			VERIFY(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8Str, utf8Size, buffer, utf16Size) == utf16Size);
			result.ReleaseBuffer(utf16Size - 1);
		} else {
			// This should only happen if the file isn't a valid UTF-8 file.
			// This should be a programming error otherwise.
			ASSERT(utf16Size == 0 && GetLastError() == ERROR_NO_UNICODE_TRANSLATION);
			ff = ffAscii;
		}
	}
#endif
	
	if (ff == ffAscii) {
		if (ConvertEmbeddedNulls(str, len)) // to be able to display binary files
			ff = ffBinary;

		result = str;
	}

	if (convertNewLinesToLFs)
		ConvertAnyToLF(result, newLineType, uniformNewLines);

	// Non-uniformity of new lines isn't meaningful for binary files since
	// they're most likely not actually new lines in the binary data.
	// Not doing anything special for newLineType for binary files because
	// there's not really a better value available for newLineType and the
	// value could possibly be meaningful if there's text embedded in a
	// binary file.
	if (uniformNewLines != NULL && ff == ffBinary)
		*uniformNewLines = true;

	if (fileFormat != NULL)
		*fileFormat = ff;

	return result;
}

void StringToFile(const CString& fileName, const CString& data, FileFormat fileFormat/* = ffAscii*/) {
	DWORD attrs = GetFileAttributes(fileName);

	//I'll get an error if I try to access a hidden file, so
	//make it non-hidden so I can write to it
	if (attrs != -1)//if file exists
		VERIFY(SetFileAttributes(fileName, attrs & ~FILE_ATTRIBUTE_HIDDEN));

	UINT flags = CFile::modeWrite | CFile::modeCreate;
	CFile file;
	TryOpenFile(file, fileName, flags);

#ifdef _UNICODE
	if (fileFormat == ffAscii) {
		char* buffer = new char[data.GetLength() + 1];
		AutoArray<char> autoArray(buffer);
		_wcstombsz(buffer, (LPCTSTR)data, data.GetLength());
		file.Write(buffer, data.GetLength());
	} else if (fileFormat == ffUTF16) {
		file.Write(UTF16BOMBytes, sizeof(UTF16BOMBytes));
		file.Write((LPCSTR)(LPCTSTR)data, GetStrCharsSize(data));
	} else if (fileFormat == ffUTF8) {
		int utf8Size = WideCharToMultiByte(CP_UTF8, 0, (LPCTSTR)data, data.GetLength() + 1, NULL, 0, NULL, NULL);
		ASSERT(utf8Size > 0);//data should always be valid

		char* buffer = new char[utf8Size];
		AutoArray<char> autoArray(buffer);
		
		VERIFY(WideCharToMultiByte(CP_UTF8, 0, (LPCTSTR)data, data.GetLength() + 1, buffer, utf8Size, NULL, NULL) == utf8Size);

		file.Write(UTF8BOMBytes, sizeof(UTF8BOMBytes));
		file.Write(buffer, utf8Size - 1);
	} else
		ASSERT(false);
#else
	file.Write((LPCSTR)data, data.GetLength());
#endif

	//I need to restore attributes becaue I cleared the hidden flag
	//and also because I think writing the file with modeCreate
	//clears the attributes
	if (attrs != -1)//if file exists
		VERIFY(SetFileAttributes(fileName, attrs));
}

CString ReadHandle(HANDLE handle, bool convertNewLinesToLFs/* = true*/) {
	CString result;

	while (true) {
		DWORD len = 0;
		VERIFY(PeekNamedPipe(handle, NULL, 0, NULL, &len, NULL));
		if (len == 0)
			break;

		char* buff = new char[len+1];//+1 for NULL
		VERIFY(ReadFile(handle, buff, len, &len, NULL) != 0);
		buff[len] = NULL;

		result += buff;
		delete [] buff;
	}

	if (convertNewLinesToLFs)
		ConvertAnyToLF(result);

	return result;
}

void WriteHandle(HANDLE handle, const CString& data) {
	DWORD len = 0;
	VERIFY(WriteFile(handle, data, data.GetLength(), &len, NULL) != 0);
	ASSERT(data.GetLength() == len);
}

bool FileExists(const CString& name) {
	return GetFileAttributes(name) != -1;
}

bool AbsolutePathExists(const CString& path) {
	CString fullPath = path;
	return SearchForFile(fullPath) && fullPath.CompareNoCase(path) == 0;
}

CString MakePath(const CString& dir, const CString& fileName) {
	CString result = dir;

	if (!result.IsEmpty() && result.GetAt(result.GetLength()-1) != '\\')
		result += '\\';

	result += fileName;

	return result;
}

CString GetDir(const CString& path) {
	CString result = path;

	int pos = result.ReverseFind('\\');
	if (pos > 0) {
		++pos;//don't delete the trailing \ 
		result.Delete(pos, result.GetLength() - pos);
	}

	return result;
}

CString GetFileNameWithExt(const CString& path) {
	CString result = path;

	int pos = result.ReverseFind('\\');
	if (pos > 0)
		result.Delete(0, pos + 1);

	return result;
}

CString GetFileNameNoExt(const CString& path) {
	int findNameStart = Max(path.ReverseFind('\\') + 1, 0);

	if (findNameStart >= path.GetLength())
		return EmptyStr;
	else {
		int pos = FindSubstring(path, TEXT("."), ffIgnoreCase|ffReverse, findNameStart);
		if (pos < 0)
			pos = path.GetLength();

		return path.Mid(findNameStart, pos - findNameStart);
	}
}

//returns true iff path has an extension
bool HasExt(const CString& path) {
	return !GetFileExt(path).IsEmpty();
}

//returns the extension of the file or path name, NOT including the dot.
//returns an empty string if the file name does not have an extension.
CString GetFileExt(const CString& path) {
	int findNameStart = Max(path.ReverseFind('\\'), 0);
	int pos = FindSubstring(path, TEXT("."), ffIgnoreCase|ffReverse, findNameStart);
	if (pos >= 0)
		return path.Right(path.GetLength() - pos - 1);
	else
		return EmptyStr;
}

bool IsValidFileName(const CString& fileName) {
	return fileName.FindOneOf(InvalidFileNameChars) < 0;
}

bool IsValidFileExt(const CString& ext) {
	return ext.FindOneOf(InvalidFileExtChars) < 0;
}

CString GetInvalidExtErrorMessage(const CString& ext) {
	ASSERT(!IsValidFileExt(ext));
	CString message;
	message.Format(TEXT("Extension '%s' is not valid. Extensions cannot contain any of the following characters:\n\t%s"), ext, InvalidFileExtChars);
	return message;
}

FILETIME GetFileModifyTime(const CString& fileName) {
	WIN32_FILE_ATTRIBUTE_DATA data;
	MemClear(data);
	VERIFY(GetFileAttributesEx(fileName, GetFileExInfoStandard, &data));
	return data.ftLastWriteTime;
}

bool SearchForFile(CString& fileName) {
	DWORD len = SearchPath(NULL, fileName, NULL, 0, NULL, NULL);
	if (len == 0)//cannot find file
		return false;

	CString path;
	LPTSTR buff = path.GetBuffer(len);
	len = SearchPath(NULL, fileName, NULL, len, buff, NULL);
	ASSERT(len > 0);
	path.ReleaseBuffer(len);
	fileName = path;
	return true;
}

void BreakPath(const CString& path, CString& dir, CString& name, CString& ext) {
	int slash = path.ReverseFind('\\');
	int dot = path.ReverseFind('.');

	dir = slash >= 0 ? path.Left(slash + 1) : EmptyStr;
	ext = dot >= 0 ? path.Right((path.GetLength() - 1) - (dot + 1) + 1) : EmptyStr;

	int nameStart = slash >= 0 ? slash + 1 : 0;
	int nameEnd = dot >= 0 ? dot - 1 : path.GetLength() - 1;
	name = path.Mid(nameStart, nameEnd - nameStart + 1);
}

CString ExpandEnvironmentStrings(const CString& string) {
	DWORD size = ExpandEnvironmentStrings(string, NULL, 0);

	CString result;
	LPTSTR buff = result.GetBuffer(size);
	VERIFY(ExpandEnvironmentStrings(string, buff, size) != 0);
	result.ReleaseBuffer();
	return result;
}

bool BrowseForFolder(HWND hwnd, CString& path, UINT flags/* = 0*/) {
	LPTSTR buff = path.GetBuffer(MAX_PATH);

	BROWSEINFO info;
	MemClear(info);
	info.hwndOwner = hwnd;
	info.pszDisplayName = buff;
	info.lpszTitle = TEXT("Choose installation directory:");
	info.ulFlags = flags;
	LPITEMIDLIST item = SHBrowseForFolder(&info);

	bool result = item != NULL;
	if (result) {
		result = SHGetPathFromIDList(item, buff) != FALSE;

		LPMALLOC malloc = NULL;
		HRESULT hr = SHGetMalloc(&malloc);

		if (SUCCEEDED(hr)) {
			malloc->Free(item);
			malloc->Release();
		} else
			ASSERT(false);
	}

	path.ReleaseBuffer();
	return result;
}

/******* The CreateLink code was taken from some place in MSDN *******/

// CreateLink - uses the shell's IShellLink and IPersistFile interfaces
//   to create and store a shortcut to the specified object.
// Returns the result of calling the member functions of the interfaces.
// lpszPathObj - address of a buffer containing the path of the object.
// lpszPathLink - address of a buffer containing the path where the
//   shell link is to be stored.
// lpszDesc - address of a buffer containing the description of the
//   shell link.

HRESULT CreateLink(LPCTSTR lpszPathObj, LPCTSTR lpszPathLink, LPCTSTR lpszDesc)
{
    HRESULT hres;
    IShellLink* psl;

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) &psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the
        // description.
        VERSUCC(psl->SetPath(lpszPathObj));
        VERSUCC(psl->SetDescription(lpszDesc));
		VERSUCC(psl->SetWorkingDirectory(GetDir(lpszPathObj)));

       // Query IShellLink for the IPersistFile interface for saving the
       // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);

        if (SUCCEEDED(hres)) {
#ifdef UNICODE
            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(lpszPathLink, TRUE);
#else
			int buffSize = lstrlen(lpszPathLink) + 1;
            WCHAR* wsz = new WCHAR[buffSize];
			AutoArray<WCHAR> deleter(wsz);

            // Ensure that the string is Unicode.
            int charsCopied =
				MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, buffSize);

			ASSERT(charsCopied == buffSize);

            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(wsz, TRUE);
#endif
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

// Only use for csidl paths that must exist on all OSes.
// Use the overload that returns a bool if the csidl may not exist.
void GetSpecialFolderPath(int csidl, TCHAR path[]) {
	VERIFY(SHGetSpecialFolderPath(NULL, path, csidl, TRUE));
}

bool GetSpecialFolderPath(int csidl, CString& path) {
	LPTSTR pathBufer = path.GetBuffer(MAX_PATH);
	BOOL succ = SHGetSpecialFolderPath(NULL, pathBufer, csidl, TRUE);
	path.ReleaseBuffer();
	return Bool(succ);
}

//creates every directory in dir that does not exist.
//returns true if successful.
bool CreateDirPath(const CString& dir) {
	int pos = 0;
	CString parent;

	while (0 <= pos && pos < dir.GetLength()) {
		pos = dir.Find('\\', pos);
		if (pos < 0)
			parent = dir;
		else {
			parent = dir.Left(pos);
			++pos;
		}

		if (!FileExists(parent) && !CreateDirectory(parent, NULL))
			return false;
	}

	return true;
}

//This will only delete a directory if it is empty.
//It is NOT an error to try to delete a file that does not exist.
bool UninstallFile(const CString& path) {
	DWORD attrs = GetFileAttributes(path);
	if (attrs == -1)//if file does not exist
		return true;

	//remove read only so it can be deleted
	VERIFY(SetFileAttributes(path, attrs & ~FILE_ATTRIBUTE_READONLY));

	bool isDir = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
	BOOL res = isDir ? RemoveDirectory(path) : DeleteFile(path);

	DEBUG_DISPLAY_LAST_ERROR(res, "deleting file '" + path + "'");

	return res != FALSE;
}

bool GetFileReadOnly(const CString& path) {
	DWORD attrs = GetFileAttributes(path);
	return attrs != -1 && (attrs & FILE_ATTRIBUTE_READONLY) != 0;
}

void SetFileReadOnly(const CString& path, bool readOnly) {
	DWORD attrs = GetFileAttributes(path);

	if (attrs != -1 && (attrs & FILE_ATTRIBUTE_READONLY) != 0)
		VERIFY(SetFileAttributes(path, attrs & ~FILE_ATTRIBUTE_READONLY));
}

long GetNumDigits(long num) {
	ASSERT(num > 0);

	long digits = 0;
	while (num > 0) {
		++digits;
		num /= 10;
	}

	return digits;
}

void FindAllFiles(const CString& fileName, StringVector& vec, bool includeFullPath, bool keepExistingItems/* = false*/) {
	if (!keepExistingItems)
		vec.clear();

	CFileFind find;

	BOOL found = find.FindFile(fileName);
	while (found) {
		found = find.FindNextFile();

		if (includeFullPath)
			vec.push_back(find.GetFilePath());
		else
			vec.push_back(find.GetFileName());
	}
}

CString GetUserName() {
	DWORD size = 0;
	VERIFY(!::GetUserName(NULL, &size));
	ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
	ASSERT(size > 0);

	DWORD numChars = size - 1; // to not include NULL terminator

	CString result;
	LPTSTR buff = result.GetBuffer(numChars);

	VERIFY(::GetUserName(buff, &size));

	result.ReleaseBuffer(numChars);
	return result;

}

#if 0
// AllowAllAccess isn't quite working. For some reason one of the the lists under security
// is left empty. For now I've decided to use PublicDocuments instead of CommonAppData so
// this method isn't needed.

typedef BOOL WINAPI CreateWellKnownSid(int WellKnownSidType, PSID DomainSid, PSID pSid, DWORD *cbSid);

// Useful for allowing write access to stuff in C:\ProgramData for Vista/7.
void AllowAllAccess(const CString& file) {
	const int WinAuthenticatedUserSid = 17;
	const int SECURITY_MAX_SID_SIZE = 68;

	ModuleLoader module("Advapi32.dll");
	CreateWellKnownSid* createWellKnownSidPtr = (CreateWellKnownSid*)module.GetProcAddress("CreateWellKnownSid");

	if (createWellKnownSidPtr != NULL) {
		DWORD sidSize = SECURITY_MAX_SID_SIZE;
		PSID sid = LocalAlloc(LMEM_FIXED, sidSize);

		BOOL succ = createWellKnownSidPtr(WinAuthenticatedUserSid, NULL, sid, &sidSize);
		DEBUG_DISPLAY_LAST_ERROR(succ, "call CreateWellKnownSid");

		DWORD aclLength = 1000 + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + sidSize;
		ACL acl;
		VERIFY(InitializeAcl(&acl, aclLength, ACL_REVISION));

		DWORD accessMask = MAXIMUM_ALLOWED | GENERIC_ALL | GENERIC_EXECUTE | GENERIC_WRITE | GENERIC_READ | STANDARD_RIGHTS_READ | STANDARD_RIGHTS_WRITE | STANDARD_RIGHTS_EXECUTE;
		AddAccessAllowedAce(&acl, ACL_REVISION, accessMask, sid);

		SECURITY_DESCRIPTOR secDesc;
		VERIFY(InitializeSecurityDescriptor(&secDesc, SECURITY_DESCRIPTOR_REVISION));
		VERIFY(SetSecurityDescriptorDacl(&secDesc, TRUE, &acl, TRUE));
		VERIFY(SetFileSecurity(file, DACL_SECURITY_INFORMATION, &secDesc));

		LocalFree(sid);
	}
}

#endif

CString GetCurrentDirectory() {
	DWORD buffChars = GetCurrentDirectory(0, NULL);
	DWORD numChars = buffChars - 1;
	CString curDir;
	LPTSTR buffer = curDir.GetBuffer(numChars);
	VERIFY(GetCurrentDirectory(buffChars, buffer) == numChars);
	curDir.ReleaseBuffer(numChars);
	return curDir;
}

CString DragQueryFile(HDROP hDrop, UINT i) {
	ASSERT(i >= 0);

	UINT chars = DragQueryFile(hDrop, i, NULL, 0);
	CString path;
	LPTSTR buffer = path.GetBuffer(chars);
	VERIFY(DragQueryFile(hDrop, i, buffer, chars + 1) == chars);
	path.ReleaseBuffer(chars);
	return path;
}

CString GetFullPathName(const CString& fileName) {
	DWORD size1 = GetFullPathName(fileName, 0, NULL, NULL);
	CString path;
	LPTSTR buff = path.GetBuffer(size1);
	DWORD len = GetFullPathName(fileName, size1, buff, NULL);
	ASSERT(len + 1 == size1);

	DWORD size2 = GetLongPathName(buff, NULL, 0);
	if (size2 > size1) {
		path.ReleaseBuffer(len);
		buff = path.GetBuffer(size2);
		len = GetLongPathName(buff, buff, size2);
		ASSERT(len + 1 == size2);
	}

	path.ReleaseBuffer(len);
	return path;
}

void MoveWindowBy(CWnd& wnd, int dx, int dy) {
	CRect rc;
	wnd.GetWindowRect(&rc);
	wnd.GetParent()->ScreenToClient(&rc);
	rc.OffsetRect(dx, dy);
	wnd.MoveWindow(&rc);
}

bool FindItemByData(const CComboBox& comboBox, DWORD itemData, int& index) {
	for (index = 0; index < comboBox.GetCount(); ++index)
		if (comboBox.GetItemData(index) == itemData)
			return true;

	return false;
}

void SetListBoxItemText(CListBox& listBox, int index, const CString& text) {
	int sel = listBox.GetCurSel();
	VERIFY(listBox.DeleteString(index) != LB_ERR);
	VERIFY(listBox.InsertString(index, text) >= 0);
	listBox.SetCurSel(sel);
}

void StringVectorToComboBox(CComboBox& comboBox, const StringVector& vec) {
	comboBox.ResetContent();
	for (size_t i = 0; i < vec.size(); ++i)
		comboBox.AddString(vec[i]);
}

CRect GetMonitorRect(HWND hwnd) {
	HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	InitSized(info);
	VERIFY(GetMonitorInfo(monitor, &info));
	return info.rcWork;
}

void LinkifyText(CRichEditCtrl& richEdit, const CString& text) {
// note that since I'm using rich edit 1.0, it always the ASCII version

#ifdef _UNICODE
	char* strText = new char[text.GetLength() + 1];
	AutoArray<char> autoArray(strText);
	_wcstombsz(strText, (LPCTSTR)text, text.GetLength());
	strText[text.GetLength()] = NULL;
#else
	char* strText = const_cast<LPTSTR>((LPCTSTR)text);
#endif

	FINDTEXTEX ft;
	MemClear(ft);
	ft.chrg.cpMin = 0;
	ft.chrg.cpMax = richEdit.GetTextLength();
	ft.lpstrText = strText;

	long pos = richEdit.FindText(0, &ft);

	if (pos >= 0) {
		CHARRANGE cr;
		richEdit.GetSel(cr);

		CHARFORMAT2A cf;
		InitSized(cf);
		cf.dwMask = CFM_LINK;
		cf.dwEffects = CFE_LINK;
		richEdit.SetSel(ft.chrgText);
		VERIFY(richEdit.SetSelectionCharFormat(cf));

		richEdit.SetSel(cr);
	}
}

//Searches for an occurence of find inside search. end is actually one past the last
//index that you want to search. Pass -1 to end to specify the end of the string.
//findEnd is one after the index in search where the find string ends. It's the same
//as the return value + find.GetLength() except it's adjusted for CRs ignored when
//using ffIgnoreCRs.
long FindSubstring(const CString& search, const CString& find, FindFlags flags, long start/* = 0*/, long end/* = -1*/, long* findEnd/* = NULL*/) {
	if (end == -1)
		end = search.GetLength();

	ASSERT(0 <= start && (start < search.GetLength() || search.IsEmpty()));
	ASSERT(0 <= end && end <= search.GetLength());
	ASSERT(start <= end);
	ASSERT(!find.IsEmpty());

	bool matchCase = (flags & ffMatchCase) != 0;
	bool ignoreCase = (flags & ffIgnoreCase) != 0;
	bool wholeWord = (flags & ffWholeWord) != 0;
	bool reverse = (flags & ffReverse) != 0;
	bool ignoreCRs = (flags & ffIgnoreCRs) != 0;

	//must pass in ffMatchCase or ffIgnoreCase, but not both
	ASSERT(matchCase != ignoreCase);

	for (long i1 = reverse ? end-1 : start; start <= i1 && i1 < end; reverse ? --i1 : ++i1) {
		//if wholeWord, then i1 must be the first char in the string or either
		//the i1th char or the (i1 - 1)th must be not be alphabetic
		if (wholeWord && i1 > 0) {
			bool isNotWholeWord =
				IsWordChar(search.GetAt(i1 - 1)) &&
				IsWordChar(search.GetAt(i1));

			if (isNotWholeWord)
				continue;
		}

		// found will be true iff the string starting at i1 matches the find string
		bool found = true;
		long pos = i1;
		long i2 = 0;
		while (i2 < find.GetLength()) {
			if (pos >= search.GetLength())
				found = false;
			else {
				TCHAR ch = search.GetAt(pos);

				if (ignoreCRs && ch == '\r') {
					//if the i1th char is a CR skip to next iteration in outer loop so the
					//returned pos won't be the skipped CR
					if (pos == i1)
						found = false;
					else
						++pos;
				} else {
					found = ChrCmp(ch, find.GetAt(i2), ignoreCase);
					++pos;
					++i2;
				}
			}

			if (!found)
				break;
		}

		//if wholeWord, then i1 must be the last char in the string or either the
		//i1th char or the (i1 + 1)th must not be alphabetic
		if (found && wholeWord) {
			long lastCharIndex = i1 + find.GetLength() - 1;
			found =
				lastCharIndex >= search.GetLength() - 1 ||
				!IsWordChar(search.GetAt(lastCharIndex)) ||
				!IsWordChar(search.GetAt(lastCharIndex+1));
		}

		if (found) {
			if (findEnd != NULL)
				*findEnd = pos;

			return i1;
		}
	}

	return -1;
}

//Searches for an occurence of find inside search. The search starts at start and ends
//at end. If end is less than or equal to start, the search will go from start to
//the end of the string, and then start again from the beginning of the string and go
//to end; hence, the search is cyclic. end is actually one past the last index that you
//want to search. You can NOT pass -1 for end to specify the length of search.
long CyclicFindSubstring(const CString& search, const CString& find, FindFlags flags, long start, long end, long* findEnd/* = NULL*/) {
	ASSERT(end >= 0);//only FindSubstring() accepts -1 for the end of the string.

	if (start == search.GetLength())
		start = 0;

	if (end > start)
		return FindSubstring(search, find, flags, start, end, findEnd);
	else {
		if ((flags & ffReverse) == 0) {//if forwards
			long pos = FindSubstring(search, find, flags, start, -1, findEnd);
			if (pos < 0)
				pos = FindSubstring(search, find, flags, 0, end, findEnd);
			return pos;
		} else {//if reverse
			long pos = FindSubstring(search, find, flags, 0, end, findEnd);
			if (pos < 0)
				pos = FindSubstring(search, find, flags, start, -1, findEnd);
			return pos;
		}
	}
}

//posToUpdate is an array of indices in string that will be updated to compensate for the text replacement
long ReplaceSubstring(CString& string, const CString& find, const CString& replace, FindFlags flags, long start/* = 0*/, long maxReplacments/* = -1*/, long posToUpdate[]/* = NULL*/, int numPosToUpdate/* = 0*/) {
	ASSERT(0 <= start && (start < string.GetLength() || string.IsEmpty()));
	ASSERT(BoolEq(posToUpdate == NULL, numPosToUpdate == 0));

	long pos = start;
	long numReplacments = 0;
	int i;

	while (true) {
		if (pos >= string.GetLength())
			break;
		if (numReplacments >= maxReplacments && maxReplacments >= 0)
			break;

		long posEnd;
		pos = FindSubstring(string, find, flags, pos, -1, &posEnd);
		if (pos < 0)
			break;

		long findLen = posEnd - pos;

		//update posToUpdate indices
		for (i = 0; i < numPosToUpdate; ++i)
			if (posToUpdate[i] >= posEnd)
				posToUpdate[i] += replace.GetLength() - findLen;

		string.Delete(pos, findLen);
		string.Insert(pos, replace);
		pos += replace.GetLength();
		++numReplacments;
	}

	return numReplacments;
}

long FindOneOf(const CString& string, const CString& charsToFind, long start) {
	ASSERT(0 <= start && start <= string.GetLength());
	ASSERT(charsToFind.GetLength() > 0);

	for (int i1 = start; i1 < string.GetLength(); ++i1)
		for (int i2 = 0; i2 < charsToFind.GetLength(); ++i2)
			if (string[i1] == charsToFind[i2])
				return i1;

	return -1;
}

long ReverseFindOneOf(const CString& string, const CString& charsToFind, long start) {
	ASSERT(0 <= start && start <= string.GetLength());
	ASSERT(charsToFind.GetLength() > 0);

	if (!string.IsEmpty()) {
		if (start == string.GetLength())
			--start;

		for (int i1 = start; i1 >= 0; --i1)
			for (int i2 = 0; i2 < charsToFind.GetLength(); ++i2)
				if (string[i1] == charsToFind[i2])
					return i1;
	}

	return -1;
}

//this function is significantly faster than the GetNumOccurrences()
//that takes a string
void GetNumNewLines(const CString& string, long& numCRs, long& numLFs, long start/* = 0*/, long end/* = -1*/) {
	ASSERT(0 <= start && start <= string.GetLength());
	ASSERT(-1 <= end && end <= string.GetLength());

	if (end < 0)
		end = string.GetLength();

	numCRs = numLFs = 0;

	LPCTSTR p = (LPCTSTR)string + start;
	LPCTSTR e = (LPCTSTR)string + end;

	while (p < e) {
		if (*p == '\r')
			++numCRs;
		if (*p++ == '\n')
			++numLFs;
	}
}

long GetNumNewLines(const CString& string, long start/* = 0*/, long end/* = -1*/) {
	long numCRs, numLFs;
	GetNumNewLines(string, numCRs, numLFs, start, end);
	return numCRs + numLFs;
}

//this function is significantly faster than the GetNumOccurrences()
//that takes a string
long GetNumOccurrences(const CString& string, TCHAR ch, long start/* = 0*/, long end/* = -1*/) {
	ASSERT(0 <= start && start <= string.GetLength());
	ASSERT(-1 <= end && end <= string.GetLength());

	if (end < 0)
		end = string.GetLength();

	long occurrences = 0;

	LPCTSTR p = (LPCTSTR)string + start;
	LPCTSTR e = (LPCTSTR)string + end;

	while (p < e)
		if (*p++ == ch)
			++occurrences;

	return occurrences;
}

long GetNumOccurrences(const CString& string, const CString& chars, long start/* = 0*/, long end/* = -1*/) {
	ASSERT(0 <= start && start <= string.GetLength());
	ASSERT(-1 <= end && end <= string.GetLength());
	ASSERT(chars.GetLength() > 0);

	if (end < 0)
		end = string.GetLength();

	long occurrences = 0;

	for (int i1 = start; i1 < end; ++i1)
		for (int i2 = 0; i2 < chars.GetLength(); ++i2)
			if (string[i1] == chars[i2]) {
				++occurrences;
				break;//string[i1] can't be equal to another char in chars
			}

	return occurrences;
}

// struct meant to be put into a vector and sorted.
// arrayIndex keeps track of position before sorting.
struct ConvIndex {
	long index;
	long arrayIndex;
	long CRs;

	ConvIndex(long _index, long _arrayIndex) :
		index(_index), arrayIndex(_arrayIndex), CRs(0) {}

	bool operator<(const ConvIndex& rhs) const {
		return index < rhs.index;
	}
};

// conversionStartPos is the position to start the conversion from.
// Indices after conversionEndPos are not converted, except conversionEndPos
// isn't used if -1. (this algorithm already doesn't convert beyond the indices
// passed in, so you don't need to pass this in for performance)
// CRLF indices are only converted to LF indices starting from conversionStartPos.
void ConvertCRLFIndicesToLFIndices(const CString& string, long* indices, long numIndices, long conversionStartPos/* = 0*/, long conversionEndPos/* = -1*/) {
	//if indices is NULL, then numIndices must be 0
	ASSERT(indices != NULL || numIndices == 0);

	if (conversionEndPos < 0)
		conversionEndPos = string.GetLength();

	// I use convIndices so this algorithm can work with sorted indices
	// without requiring indices to be sorted

	vector<ConvIndex> convIndices;
	for (size_t i1 = 0; i1 < (size_t)numIndices; ++i1)
		convIndices.push_back(ConvIndex(indices[i1], i1));

	sort(convIndices.begin(), convIndices.end());

	//calculate the CRs between each index in convIndices
	for (i1 = 0; i1 < convIndices.size(); ++i1) {
		long start = i1 == 0 ? 0 : convIndices[i1 - 1].index;
		start = Max(start, conversionStartPos);

		if (start < convIndices[i1].index && convIndices[i1].index <= conversionEndPos)
			convIndices[i1].CRs = GetNumOccurrences(string, '\r', start, convIndices[i1].index);
	}

	//adjust the indices in convIndices for the CRs
	for (i1 = 0; i1 < convIndices.size(); ++i1)
		for (size_t i2 = i1; i2 < convIndices.size(); ++i2)
			convIndices[i2].index -= convIndices[i1].CRs;

	// copy the converted indices back to the original array
	for (i1 = 0; i1 < convIndices.size(); ++i1)
		indices[convIndices[i1].arrayIndex] = convIndices[i1].index;
}

// indices are offsets from convertedIndexBase, which is a position in CRLF indices.
void ConvertLFIndicesToCRLFIndices(const CString& string, long* indices, long numIndices, long convertedIndexBase/* = 0*/) {
	//if indices is NULL, then numIndices must be 0
	ASSERT(indices != NULL || numIndices == 0);

	// I use convIndices so this algorithm can work with sorted indices
	// without requiring indices to be sorted

	vector<ConvIndex> convIndices;
	for (size_t i1 = 0; i1 < (size_t)numIndices; ++i1)
		convIndices.push_back(ConvIndex(indices[i1], i1));

	sort(convIndices.begin(), convIndices.end());

	//adjust the indices in convIndices for the CRs
	long lastConvertedPos = 0;
	for (i1 = 0; i1 < convIndices.size(); ++i1)
		if (convIndices[i1].index >= convertedIndexBase) {
			long pos = ConvertLFPosToCRLFPos(string, convIndices[i1].index, Max(lastConvertedPos, convertedIndexBase));
			long amountAdded = pos - convIndices[i1].index;

			if (amountAdded > 0)
				for (size_t i2 = i1; i2 < convIndices.size(); ++i2)
					convIndices[i2].index += amountAdded;

			lastConvertedPos = pos;
		}

	// copy the converted indices back to the original array
	for (i1 = 0; i1 < convIndices.size(); ++i1)
		indices[convIndices[i1].arrayIndex] = convIndices[i1].index;
}

// pos is an offset from convertedPosBase, which is a position in CRLF indices.
// You could get a convertedPosBase by adding to the result of a previous call
// to ConvertLFPosToCRLFPos. 
long ConvertLFPosToCRLFPos(const CString& text, long pos, long convertedPosBase/* = 0*/, bool allowPosPastEndOfText/* = false*/) {
	ASSERT(convertedPosBase <= pos);

	long start = convertedPosBase;
	int length = text.GetLength();

	ASSERT(allowPosPastEndOfText || pos <= length);

	while (start < pos) {//false when no CRs b/t start and pos
		//pass pos + 1 because GetNumOccurrences() excludes the end char from the count
		long end = Min(pos + 1, length);
		ASSERT(allowPosPastEndOfText || start < end);//pos is not a valid CRLF position. It goes past the end of the string.
		long numCRs = GetNumOccurrences(text, '\r', start, end);
		start = end;//next time, start looking where we stopped this time
		pos += numCRs;

		if (allowPosPastEndOfText && end == length)
			break;
	}

	return pos;
}

void AdjustIndices(long pos, long* indicesToAdjust, const long numIndicesToAdjust, long adjustAmount) {
	//if indicesToAdjust is NULL, then numIndicesToAdjust must be 0
	ASSERT(indicesToAdjust != NULL || numIndicesToAdjust == 0);

	for (long i = 0; i < numIndicesToAdjust; ++i)
		if (indicesToAdjust[i] >= pos)
			indicesToAdjust[i] = Max(indicesToAdjust[i] + adjustAmount, pos);
}

void ConvertCRLFtoLF(CString& string) {
	LPTSTR buff = string.GetBuffer(string.GetLength());
	ConvertCRLFtoLF(buff, buff);
	string.ReleaseBuffer();
}

//src uses \r\n for new lines. dset will use \n.
//soft line breaks (i.e. \r\r\n) are sripped out.
void ConvertCRLFtoLF(LPTSTR dest, LPCTSTR src) {
	while (true) {
		//need loop in case there's a line break after a soft line break
		while (*src == '\r') {
			++src;//if we have \r\n, just copy the \n

			//skip wordwrap line break (i.e. \r\r\n) entirely
			if (*src == '\r' && src[1] == '\n')
				src += 2;
		}

		*dest = *src;

		if (*dest == NULL)
			break;

		++dest;
		++src;
	}
}

CString ConvertLFtoCRLF(const CString& string, long lineCount/* = -1*/) {
	long lenNeeded = string.GetLength();
	lenNeeded += lineCount < 0 ? GetNumOccurrences(string, '\n') : lineCount - 1;
	CString result;
	LPTSTR buff = result.GetBuffer(lenNeeded);
	ConvertLFtoCRLF(buff, string);
	result.ReleaseBuffer(lenNeeded);
	return result;
}

//src uses \n for new lines. dest will use \r\n.
void ConvertLFtoCRLF(LPTSTR dest, LPCTSTR src) {
	//not possible to convert if dest == src, unless src is empty
	ASSERT(dest != src || *src == NULL);

	while (true) {
		ASSERT(*src != '\r');//is src already CRLF?

		if (*src == '\n') {
			*dest = '\r';
			++dest;
		}

		*dest = *src;

		if (*dest == NULL)
			break;

		++dest;
		++src;
	}
}

void ConvertLFtoCR(CString& string) {
	long length = string.GetLength();
	LPTSTR buff = string.GetBuffer(length);
	ConvertLFtoCR(buff, buff);
	string.ReleaseBuffer(length);
}

void ConvertLFtoCR(LPTSTR dest, LPCTSTR src) {
	while (true) {
		*dest = *src == '\n' ? '\r' : *src;

		if (*dest == NULL)
			break;

		++dest;
		++src;
	}
}

long ConvertAnyToLF(CString& string, NewLineType* newLineType/* = NULL*/, bool* uniformNewLines/* = NULL*/) {
	LPTSTR buff = string.GetBuffer(string.GetLength());
	long numCharsRemoved = ConvertAnyToLF(buff, buff, newLineType, uniformNewLines);
	string.ReleaseBuffer();
	return numCharsRemoved;
}

inline void FoundNewLine(NewLineType& newLineType, bool& uniformNewLines, const NewLineType& foundNewLineType) {
	if (newLineType == nlDefault)
		newLineType = foundNewLineType;
	else if (newLineType != foundNewLineType)
		uniformNewLines = false;
}

//converts a LF, CR, or CRLF string to a LF string
//stores the new line type in newLineType
//stores whether the new line types are consistent in the file in uniformNewLines
long ConvertAnyToLF(LPTSTR dest, LPCTSTR src, NewLineType* newLineType/* = NULL*/, bool* uniformNewLines/* = NULL*/) {
	long numCharsRemoved = 0;
	NewLineType _newLineType = nlDefault;
	bool _uniformNewLines = true;

	while (true) {
		if (*src == '\r' && src[1] != '\n') {//found just CR
			*dest = '\n';
			FoundNewLine(_newLineType, _uniformNewLines, nlCR);
		} else {
			if (*src == '\r') {//found CRLF, so skip the CR
				++src;
				++numCharsRemoved;
				FoundNewLine(_newLineType, _uniformNewLines, nlCRLF);
			} else if (*src == '\n')//found just LF
				FoundNewLine(_newLineType, _uniformNewLines, nlLF);

			ASSERT(*src != '\r');//should never happen

			*dest = *src;

			if (*dest == NULL)
				break;
		}

		++dest;
		++src;
	}

	if (newLineType != NULL)
		*newLineType = _newLineType;
	if (uniformNewLines != NULL)
		*uniformNewLines = _uniformNewLines;

	return numCharsRemoved;
}

long RemoveCR(CString& string, long* indicesToAdjust/* = NULL*/, long numIndicesToAdjust/* = 0*/) {
	LPTSTR buff = string.GetBuffer(string.GetLength());
	long numCRsRemoved = RemoveCR(buff, buff, indicesToAdjust, numIndicesToAdjust);
	string.ReleaseBuffer();
	return numCRsRemoved;
}

long RemoveCR(LPTSTR dest, LPCTSTR src, long* indicesToAdjust/* = NULL*/, long numIndicesToAdjust/* = 0*/) {
	long iSrc = 0, iDest = 0;
	long numCRsRemoved = 0;

	while (true) {
		if (src[iSrc] != '\r') {
			dest[iDest] = src[iSrc];

			if (dest[iDest] == NULL)
				break;
			
			++iDest;
		} else {
			++numCRsRemoved;
			AdjustIndices(iDest, indicesToAdjust, numIndicesToAdjust, -1);
		}

		++iSrc;
	}

	return numCRsRemoved;
}

#if 0
void RemoveCR(LPTSTR dest, LPCTSTR src) {
	while (true) {
		if (*src != '\r') {
			*dest = *src;
			
			++dest;

			if (*dest == NULL)
				break;
		}

		++src;
	}
}
#endif

bool ConvertEmbeddedNulls(char* ptr, int len) {
	bool containsNull = false;

	for (char* end = ptr + len; ptr < end; ++ptr)
		if (*ptr == NULL) {
			*ptr = ' ';
			containsNull = true;
		}

	return containsNull;
}

long FindMatchImpl(const StringIterator& vec, const CString& string, bool matchCase, long first, long last) {
	if (first > last)
		return -1;

	long mid = (first + last) / 2;
	int comp = StrCmpEx(vec.GetNthString(mid), string, matchCase);

	if (comp == 0)
		return mid;
	else if (comp < 0)
		first = mid + 1;
	else
		last = mid - 1;

	return FindMatchImpl(vec, string, matchCase, first, last);
}

//Returns index of string into vec. Uses a binary search. vec must be sorted
//case sensitively if matchCase is true. vec must be sorted case insensitively
//if matchCase is false.
long FindMatch(const StringIterator& vec, const CString& string, bool matchCase) {
	return FindMatchImpl(vec, string, matchCase, 0, vec.GetNumStrings() - 1);
}

long GetNumTokenCount(const CString& string) {
	const long length = string.GetLength();
	long count = 0;
	long pos = 0;

	StringToken curState = stNone;

	while (pos < length) {
		TCHAR ch = string.GetAt(pos);

		switch (curState) {
		case stNone:
			if (_istspace(ch))
				curState = stWhiteSpace;
			else if (_istalpha(ch) || ch == '_')
				curState = stIdentifier;
			else
				curState = stSymbol;

			++count;
			++pos;
			break;
		case stWhiteSpace:
			if (_istspace(ch))
				++pos;
			else
				curState = stNone;
			break;
		case stIdentifier:
			if (IsVarChar(ch))
				++pos;
			else
				curState = stNone;
			break;
		case stSymbol:
			if (_istspace(ch) || ch == '_' || _istalpha(ch))
				curState = stNone;
			else
				++pos;
			break;
		}
	}

	return count;
}

// never returns stNone
StringToken GetNextToken(const CString& string, long start, CString& token) {
	const long length = string.GetLength();
	ASSERT(0 <= start && start < length);

	TCHAR ch = string.GetAt(start);
	long pos = start + 1;
	StringToken result;

	if (_istspace(ch)) {
		while (pos < length && _istspace(string.GetAt(pos)))
			++pos;

		result = stWhiteSpace;
	} else if (_istalpha(ch) || ch == '_') {
		while (pos < length && IsVarChar(string.GetAt(pos)))
			++pos;

		result = stIdentifier;
	} else {
		while (pos < length) {
			ch = string.GetAt(pos);
			if (_istspace(ch) || _istalpha(ch) || ch == '_')
				break;

			++pos;
		}

		result = stSymbol;
	}

	token = string.Mid(start, pos - start);
	return result;
}

Accel::Accel() {
	key = 0;
	fVirt = 0;
	cmd = 0;
}

Accel::Accel(VirtualKey vkey, KeyModifiers mods, WORD command) {
	fVirt = FVIRTKEY;
	if ((mods & kmShift) != 0)
		fVirt |= FSHIFT;
	if ((mods & kmCtrl) != 0)
		fVirt |= FCONTROL;
	if ((mods & kmAlt) != 0)
		fVirt |= FALT;

	key = vkey;
	cmd = command;
}

KeyModifiers Accel::GetModifiers() const {
	KeyModifiers mods = 0;
	if ((fVirt & FSHIFT) != 0)
		mods |= kmShift;
	if ((fVirt & FCONTROL) != 0)
		mods |= kmCtrl;
	if ((fVirt & FALT) != 0)
		mods |= kmAlt;
	return mods;
}

void Accel::DoPersist(Persist& p) {
	p.PersistNumber(TEXT("Command"), cmd);
	p.PersistNumber(TEXT("Key"), key);
	p.PersistNumber(TEXT("Flags"), fVirt);
}

vector<Accel>::const_iterator AccelTable::FindAccelByCmd(UINT cmd) const {
	for (vector<Accel>::const_iterator it = m_accels.begin(); it != m_accels.end(); ++it)
		if (it->cmd == cmd)
			return it;

	return m_accels.end();
}

bool AccelTable::GetHotKey(UINT cmd, VirtualKey& key, KeyModifiers& mods) const {
	vector<Accel>::const_iterator it = FindAccelByCmd(cmd);
	if (it != m_accels.end()) {
		key = it->GetVirtualKey();
		mods = it->GetModifiers();
		return true;
	} else
		return false;
}

UINT AccelTable::FindHotKey(VirtualKey key, KeyModifiers mods) const {
	if (key != 0)
		for (size_t i = 0; i < m_accels.size(); ++i)
			if (m_accels[i].GetVirtualKey() == key && m_accels[i].GetModifiers() == mods)
				return m_accels[i].cmd;

	return 0;
}

void AccelTable::DecorateMenu(HMENU hMenu) {
	CMenu menu;
	menu.Attach(hMenu);

	for (size_t i = 0; i < m_accels.size(); ++i) {
		CString string;
		menu.GetMenuString(m_accels[i].cmd, string, MF_BYCOMMAND);

		//remove old shortcut
		long pos = string.Find('\t');
		if (pos != -1)
			string.Delete(pos, string.GetLength() - pos);

		CString hotkey = KeyAndModifiersToString(m_accels[i].GetVirtualKey(), m_accels[i].GetModifiers());
		AppendString(string, hotkey, TEXT("\t"));
		//ModifyMenu may fail if the command isn't in the menu which is OK.
		menu.ModifyMenu(m_accels[i].cmd, MF_BYCOMMAND|MF_STRING, m_accels[i].cmd, string);
	}

	menu.Detach();
}

void AccelTable::DoPersist(Persist& p) {
	p.PersistClassVector(TEXT("AcceleratorTable"), m_accels);

	if (p.IsLoading())
		Destroy();
}

UINT CommandIdCreator::CreateId() {
	if (mOldIds.empty())
		return mCurNewId++;
	else {
		UINT newId = mOldIds.back();
		mOldIds.erase(&mOldIds.back());
		return newId;
	}
}

void CommandIdCreator::ReleaseId(UINT id) {
	if (id >= mBase && id != (UINT)-1)//only release id if it was created by CreatorId
		mOldIds.push_back(id);
}

//safe to call if some sub menu items were not creator by CommandIdCreator
void CommandIdCreator::ReleaseSubMenuItemsIds(HMENU hMenu) {
	CMenu menu;
	VERIFY(menu.Attach(hMenu));

	for (int i = 0; i < menu.GetMenuItemCount(); ++i) {
		UINT id = menu.GetMenuItemID(i);
		if (id == (UINT)-1)// -1 is submenu
			ReleaseSubMenuItemsIds(menu.GetSubMenu(i)->GetSafeHmenu());
		else if (id != 0) // 0 is separator
			ReleaseId(id);
	}

	menu.Detach();
}

CString Exception::GetErrorMessage() const {
	if (!mMessage.IsEmpty())
		return mMessage;

	CString msg;

	switch (mErrorCode) {
	case ErrNewerFileVersion :
		msg = TEXT("The file version is newer than the application version.");
		break;
	default : {
		ASSERT(false);
		msg.Format(TEXT("Undefined error %d"), mErrorCode);
	}
	}

	return msg;
}

void LiteEditCommandLineInfo::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) {
	if (bFlag && mParseState == psInit && (StrCmpI(pszParam, TEXT("?")) == 0 || StrCmpI(pszParam, TEXT("HELP")) == 0))
		mShowUsage = true;
	else if (bFlag && mParseState == psInit && (StrCmpI(pszParam, TEXT("goto")) == 0 || StrCmpI(pszParam, TEXT("GL")) == 0))
		mParseState = psGoto;
	else if (!bFlag && mParseState == psGoto) {
		CString param = pszParam;
		long colonPos = param.Find(_T(":"));

		if (colonPos >= 0) {// if optional column number included
			CString lineNum = param.Left(colonPos);
			CString colNum = param.Right(param.GetLength() - colonPos - 1);
			mLineNumber = _tstoi(lineNum);
			mColumnNumber = _tstoi(colNum);
		} else {
			mLineNumber = _tstoi(pszParam);
			mColumnNumber = 0;// in case goto is specific multiple times
		}

		mParseState = psInit;
	} else
		Base::ParseParam(pszParam, bFlag, bLast);
}

CString Version::AsString() const {
	if (mString.IsEmpty())
		if (mRevision == 0)
			mString.Format(_T("%d.%d"), mMajor, mMinor);
		else
			mString.Format(_T("%d.%d.%d"), mMajor, mMinor, mRevision);

	return mString;
}

int Version::Compare(const Version& rhs) const {
	if (mMajor != rhs.mMajor)
		return mMajor - rhs.mMajor;

	if (mMinor != rhs.mMinor)
		return mMinor - rhs.mMinor;

	return mRevision - rhs.mRevision;
}
