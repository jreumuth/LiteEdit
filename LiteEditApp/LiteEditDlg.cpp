// LiteEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LiteEdit.h"
#include "LiteEditDlg.h"
#include "GotoDlg.h"
#include "Persist.h"
#include "ToolsDlg.h"
#include "SyntaxColoringDlg.h"
#include "EditorOptionsDlg.h"
#include "HotkeysDlg.h"
#include "AboutDlg.h"
#include "TimeDateFormatDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const int SBI_POS = 0;
static const int SBI_CLIPBOARD = 1;

static const UINT TID_EXTERNAL_MODIFY = 1;

//unfortunately, I have to hard code menu positions in
//since sub menus don't have an id
static const UINT MP_FILE = 0;
static const UINT MP_RECENT = 8;
static const UINT MP_TOOLS = 4;

/////////////////////////////////////////////////////////////////////////////
// CLiteEditDlg dialog

CLiteEditDlg::CLiteEditDlg(const LiteEditCommandLineInfo& cmdLineInfo, CWnd* pParent /*=NULL*/)
	: CDialog(CLiteEditDlg::IDD, pParent), mCmdLineInfo(cmdLineInfo)
{
	//{{AFX_DATA_INIT(CLiteEditDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	mSmallIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	mLargeIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
}

void CLiteEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLiteEditDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLiteEditDlg, CDialog)
	//{{AFX_MSG_MAP(CLiteEditDlg)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_CHANGECBCHAIN()
	ON_WM_DRAWCLIPBOARD()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	ON_WM_CLOSE()
	ON_WM_INITMENU()
	ON_CONTROL(ECN_CHANGE, ID_EDIT_CTRL, OnEditChange)
	ON_CONTROL(ECN_SELCHANGE, ID_EDIT_CTRL, OnEditSelChange)
	ON_REGISTERED_MESSAGE(WM_FINDREPLACE, OnFindReplace)
	ON_WM_ACTIVATE()
	ON_WM_CANCELMODE()
	ON_WM_HELPINFO()
	ON_WM_QUERYENDSESSION()
	ON_WM_DROPFILES()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CLiteEditDlg::PreTranslateMessage(MSG* pMsg) {
	return
		TranslateAccelerator(m_hWnd, gConfigData.mAccelTable, pMsg) ||
		TranslateAccelerator(m_hWnd, gConfigData.mCommands.mAccelTable, pMsg);
}

void CLiteEditDlg::ShowUsage() const {
	CString message = _T("Lite Edit command line usage:\n");
	message += _T("    LiteEdit [<file name>] [-goto <line number>[:<column number>]]\n\n");
	message += _T("Explanation of flags:\n");
	message += _T("    -goto\t Go to the specified line and column numbers\n\n");
	message += _T("Examples:\n");
	message += _T("    LiteEdit MyApplication.cpp\n");
	message += _T("    LiteEdit C:\\Dev\\Source\\MyApplication.cpp\n");
	message += _T("    LiteEdit MyApplication.cpp -goto 27\n");
	message += _T("    LiteEdit MyApplication.cpp -goto 27:4");

	//Show the message box on the desktop (i.e. pass NULL as first parameter)
	//so there will be a task bar item for it. At this point, there's not a
	//task bar item for Lite Edit yet.
	::MessageBox(NULL, message, _T("Lite Edit Command Line Help"), MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND);

	exit(0);
}

//If user passed a file name, open it if it exists, otherwise, create a new file with
//that name. If no file name was passed, then create a new file with a default name.
void CLiteEditDlg::ProcessCommandLine() {
	CString path;

	if (FileExists(mCmdLineInfo.m_strFileName))
		path = GetFullPathName(mCmdLineInfo.m_strFileName);
	else
		path = mCmdLineInfo.m_strFileName;

	bool newFile = !FileExists(path);
	if (path.IsEmpty())
		path = GetNewFileName();

	try {
		LoadFile(path, newFile);
	} catch (CException* e) {
		// error should ony be possible if the file exists and we tried to load it
		ASSERT(FileExists(path));

		// if error occurs loading the file, create a new file and display the error
		LoadFile(GetNewFileName(), true);
		e->ReportError(MB_ICONERROR);
	}

	if (mCmdLineInfo.mLineNumber > 0)
		mEdit.GotoLineAndColNum(mCmdLineInfo.mLineNumber, mCmdLineInfo.mColumnNumber);
}

//Loads fileName into mEdit. If newFile is true, then we create a newFile.
//If newFile is false, then we open an existing file. This parameter needs
//to be passed in because you could create a new file with the same same
//as an existing file, e.g. Untitiled.txt.
void CLiteEditDlg::LoadFile(const CString& fileName, bool newFile) {
	WaitCursor wc;

	bool exists = !newFile && FileExists(fileName);

	//display error message and don't do anything
	if (!newFile && !exists) {
		CString msg;
		msg.Format(TEXT("File '%s' does not exist."), fileName);
		DisplayErrorMessage(msg);
		gConfigData.mRecentFileList.Remove(fileName);
		gConfigData.Save(); // save updated file MRU list
		return;
	}

	// update to make things look nicer if the file takes a long time to load
	UpdateWindow();

	//Load the file text now before the text is modified or the file name is changed
	//because an exception could be thrown while trying to open the file. Lite Edit
	//shouldn't be an intermetiate state of loading the file if an exception is
	//thrown.
	CString fileText;
	FileFormat fileFormat = gConfigData.mDefaultFileFormat;
	NewLineType newLineType = nlDefault;
	bool uniformNewLines = true;
	if (!newFile)
		fileText = FileToString(fileName, true, &fileFormat, &newLineType, &uniformNewLines);

	newLineType = gConfigData.CalcNewLineType(newLineType);

	//Setting the file name will set the language. Setting the text to empty
	//prevents reparsing existing text as a different language.
	//Also, setting the language before loading the real text
	//prevents the real text from being parsed by the old language.
	//This also sets the correct file name and modified mark while the file is loading.
	mEdit.SetText(EmptyStr, false);
	SetFileInfo(fileName, exists, fileFormat, newLineType);

	// Disable word wrap if loading a binary file.
	// Loading a file with wordwrap can be very slow, especially for some binary files.
	// Word wrap can cause many binary files to take minutes instead of seconds to load.
	// Additionally, word wrap is not likely as useful for binary files.
	if (fileFormat == ffBinary) {
		gConfigData.mWordWrap = false;
		mEdit.SetWordWrap(gConfigData.mWordWrap);
	}

	if (newFile)
		mEdit.SetReadOnly(false);
	else {
		ATLASSERT(exists);
		mEdit.SetText(fileText, false);
		bool readOnly = GetFileReadOnly(fileName);
		mEdit.SetReadOnly(readOnly);

		if (gConfigData.mAddOpenedFilesToWindowsRecentDocuments)
			SHAddToRecentDocs(SHARD_PATH, (LPCTSTR)fileName);
	}

	UpdateTitle();//to update readonly marker
	mEdit.EmptyUndoBuffer();

	if (!uniformNewLines) {
		CString msg;
		msg.Format(_T("The file contains more than one type of new line. All the new lines have been converted to the first type found, %s."), NewLineTypeToString(newLineType));
		MessageBox(msg, _T("Inconsistent New Lines"), MB_OK|MB_ICONINFORMATION);
	}
}

//Saves text in mEdit to disk as fileName. Retruns false is save is canceled.
bool CLiteEditDlg::SaveFileAs(const CString& fileName, FileFormat fileFormat) {
	if (GetFileReadOnly(fileName)) {
		CString msg;
		msg.Format(TEXT("You cannot save over read only file:\n%s\n\nYou must save this file with a different name or make the file on disk writeable."), fileName);
		DisplayErrorMessage(msg);
		return false;
	} else {
		WaitCursor wc;

		if (gConfigData.mRemoveTrailingWhitespaceOnSave)
			RemoveTrailingWhiteSpace();

		NewLineType newLineType = gConfigData.CalcNewLineType(mFileInfo.newLineType);

		StringToFile(fileName, mEdit.GetTextEx(newLineType), fileFormat);
		mEdit.SetModified(false);
		SetFileInfo(fileName, true, fileFormat, newLineType);
		return true;
	}
}

//Store last modified time of this file in mFileInfo. see OnActivate().
void CLiteEditDlg::SetFileInfo(const CString& name, bool exists, FileFormat fileFormat, NewLineType newLineType) {
	CString oldExt = GetFileExt(mFileName);
	CString newExt = GetFileExt(name);

	mFileName = name;

	if (AbsolutePathExists(mFileName)) {
		gConfigData.mRecentFileList.Add(mFileName);
		gConfigData.Save(); // save updated file MRU list
	}

	mFileInfo.exists = exists;
	mFileInfo.fileFormatOnDisk = fileFormat;
	mFileInfo.fileFormat = fileFormat;
	mFileInfo.newLineTypeOnDisk = newLineType;
	mFileInfo.newLineType = newLineType;

	if (exists)
		mFileInfo.modifyTime = GetFileModifyTime(GetFileName());

	// if extension changes then what items are visible and enabled may also
	if (oldExt.CompareNoCase(newExt) != 0)
		RebuildToolsMenu(); 

	SetLangFromFileExt(); // must set fileFormat before calling this

	UpdateTitle(); // since name and possibly modified marker changed
}

//removes the trailing white space from each line in mEdit and
//adjusts the selection to account for the removed whitespace.
void CLiteEditDlg::RemoveTrailingWhiteSpace() {
	long fvl = mEdit.GetFirstVisibleLine();
	long fvp = mEdit.GetFirstVisiblePixel();
	long pos = 0;
	vector<long> bookmarks;
	const long count = mEdit.GetLineCount();
	long sel[2];
	mEdit.GetSel(sel[0], sel[1]);

	CString convertedText;
	LPTSTR convertedTextBuff = convertedText.GetBuffer(mEdit.GetTextLength());
	LPTSTR convertedTextBuffPtr = convertedTextBuff;

	//this loop will:
	//1. convert convertedTextBuff to the text in mEdit without ths trailing whitespace
	//   or wrapped new lines
	//2. adjust sel to account for the removed whitespace
	//3. convert sel from wrapped indices to non-wrapped indices
	for (long i = 0; i < count; ++i) {
		if (mEdit.HasBookmark(i))
			bookmarks.push_back(i);

		CString line = mEdit.GetLine(i);
		bool doesLineWrap = mEdit.DoesLineWrap(i);

		//don't remove trailing whitespace from a wrapped line
		if (doesLineWrap) {
			pos += line.GetLength();
			AdjustIndices(pos, sel, 2, -1);//convert sel to non-wrapped indices
		} else {
			long len = line.GetLength();

			//this is basically an efficient way of
			//doing line.TrimRight(" \t")
			LPTSTR buff = line.GetBuffer(len);
			LPTSTR p = buff + len - 1;
			while (*p == ' ' || *p == '\t')
				--p;
			line.ReleaseBuffer(p - buff + 1);

			long numSpacesRemoved = len - line.GetLength();
			pos += line.GetLength();

			//adjust the selection for the removed spaces
			if (numSpacesRemoved != 0)
				AdjustIndices(pos, sel, 2, -numSpacesRemoved);
		}

		StrCpy(convertedTextBuffPtr, line);
		convertedTextBuffPtr += line.GetLength();

		//append a new line if it's not the last line and it's not a
		//wrapped line
		if (i < count - 1 && !doesLineWrap) {
			*convertedTextBuffPtr++ = '\n';
			++pos;
		}
	}

	int newLen = convertedTextBuffPtr - convertedTextBuff;
	convertedText.ReleaseBuffer(newLen);

	if (convertedText.GetLength() != mEdit.GetTextLength()) {//if whitespace was removed
		mEdit.ModifyText(0, mEdit.GetTextLength(), convertedText, sel[0], sel[1], skNone);
		mEdit.SetFirstVisibleLine(fvl);
		mEdit.SetFirstVisiblePixel(fvp);
		mEdit.AddBookmarks(bookmarks);
	}
}

void CLiteEditDlg::SetLangFromFileExt() {
	LanguagePtr lang;

	// binary data cannot meaningfully be parsed, so leaving language as NULL for
	// binary files
	if (mFileInfo.fileFormat != ffBinary)
		lang = gConfigData.mLangMgr.FindLanguageByExtension(GetFileExt(GetFileName()));

	mEdit.SetLanguage(lang);
}

void CLiteEditDlg::NewFile() {
	if (PromptToSave())
		LoadFile(GetNewFileName(), true);
}

CString CLiteEditDlg::GetNewFileName() const {
	CString title = TEXT("Untitled");
	AppendString(title, GetDefaultExt(), TEXT("."));
	return title;
}

//Returns true if the file name is the one given from File>>New.
//It works correctly if the default extension has changed since
//calling File>>New.
bool CLiteEditDlg::HasNewFileName() const {
	return mFileName.Find(TEXT("Untitled.")) == 0 || mFileName == TEXT("Untitled");
}

//File>>Open command
void CLiteEditDlg::OpenFile() {
	if (PromptToSave()) {
		CString filter = gConfigData.mLangMgr.BuildFilter();
		// using OFN_DONTADDTORECENT because LoadFile will add to recent based on
		// the config setting
		CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_DONTADDTORECENT, filter, this);
		dlg.m_ofn.nFilterIndex = gConfigData.GetOpenFilterIndex();

		if (dlg.DoModal() == IDOK) {
			//Unforunately, I can't rely on CFileDialog() to add the default extension
			//because on Win 98 (at least) it will still add the default extension
			//if an extension is specified.
			CString path = dlg.GetPathName();
			if (!HasExt(path))
				AppendString(path, GetDefaultExt(), TEXT("."));

			LoadFile(path, false);
		}

		//store this for the next time we open a file
		gConfigData.SetOpenFilterIndex(dlg.m_ofn.nFilterIndex);
	}
}

bool CLiteEditDlg::IsFileModified() const {
	return 
		mEdit.GetModified() ||
		mFileInfo.fileFormatOnDisk != mFileInfo.fileFormat ||
		mFileInfo.newLineTypeOnDisk != mFileInfo.newLineType;
}

//Prompts user to save file if necessary and saves if told to. Returns false if the save
//is canceled.
bool CLiteEditDlg::PromptToSave() {
	if (IsFileModified()) {
		int res = MessageBox(TEXT("Save modified file?"), TEXT("Save File"), MB_YESNOCANCEL|MB_ICONWARNING);
		if (res == IDYES)
			return Save();
		else
			return res != IDCANCEL;
	} else
		return true;
}

bool CLiteEditDlg::CanSave() const {
	if (IsFileModified())
		return true;

	if (mFileInfo.exists) {
		FILETIME ft = GetFileModifyTime(GetFileName());
		// if file overwritten on disk
		if (CompareFileTime(&ft, &mFileInfo.modifyTime) != 0)
			return true;
	} else {
		// if absolute path that doesn't exist, e.g. new file or file deleted on disk
		if (!HasNewFileName())
			return true;
	}

	return false;
}

//File>>Save command. Returns false if save is canceled.
bool CLiteEditDlg::Save() {
	if (CanSave()) {
		if (FileExists(GetFileName())) {
			if (mFileInfo.fileFormat == ffBinary) {
				// binary files have NULLs replaced with spaces so saving would corrupt the file
				DisplayError(_T("Lite Edit cannot save the file because it is not an ASCII or unicode (UTF-16 or UTF-8 little endian) text file."), m_hWnd);
				return false;
			} else {
				if (mFileInfo.fileFormatOnDisk == ffBinary) {
					CString msg;
					msg.Format(_T("The file on disk has an unrecognized file format. Saving over the file will convert the file to an %s text file. Do you wish to continue and save over the file?"), FileFormatToString(mFileInfo.fileFormat));

					if (MessageBox(msg, _T("Convert File Format"), MB_YESNO | MB_ICONWARNING) == IDNO)
						return false;
				}

				return SaveFileAs(GetFileName(), mFileInfo.fileFormat);
			}
		} else
			return SaveAs();
	}

	return true;
}

//File>>Save As command. Retruns false is save is canceled.
bool CLiteEditDlg::SaveAs() {
	CString filter = TEXT("ASCII Text|*.*|");
#ifdef _UNICODE
	filter += _T("Unicode UTF-16 Text|*.*|Unicode UTF-8 Text|*.*|");
#endif
	filter += '|';

	DWORD flags = OFN_OVERWRITEPROMPT;
	// despite name, this option applies to saved files too
	if (!gConfigData.mAddOpenedFilesToWindowsRecentDocuments)
		flags |= OFN_DONTADDTORECENT;

	CFileDialog dlg(FALSE, GetDefaultExt(), GetFileName(), flags, filter, this);

	FileFormat initialFileFormat = mFileInfo.fileFormat == ffBinary ?
		gConfigData.mDefaultFileFormat : mFileInfo.fileFormat;

	if (initialFileFormat == ffAscii)
		dlg.m_ofn.nFilterIndex = 1;
	else if (initialFileFormat == ffUTF16)
		dlg.m_ofn.nFilterIndex = 2;
	else if (initialFileFormat == ffUTF8)
		dlg.m_ofn.nFilterIndex = 3;
	else
		ASSERT(false);


	if (dlg.DoModal() == IDOK) {
		FileFormat fileFormat = mFileInfo.fileFormat;
		if (dlg.m_ofn.nFilterIndex == 1)
			fileFormat = ffAscii;
		else if (dlg.m_ofn.nFilterIndex == 2)
			fileFormat = ffUTF16;
		else if (dlg.m_ofn.nFilterIndex == 3)
			fileFormat = ffUTF8;
		else
			ASSERT(false);
		return SaveFileAs(dlg.GetPathName(), fileFormat);
	} else
		return false;
}

#include "WinSpool.h"

void CLiteEditDlg::Print() {
	CWaitCursor wc;

	DWORD printFlags = PD_ALLPAGES|PD_HIDEPRINTTOFILE|PD_NOPAGENUMS|PD_COLLATE;
	CPrintDialog dlg(FALSE, printFlags);

	if (dlg.DoModal() == IDOK) {
		wc.Restore();

		//these must be freed, or else the user may have to restart
		//their computer to print again
		AutoGlobal devNamesFreer(dlg.m_pd.hDevNames);
		AutoGlobal devModeFreer(dlg.m_pd.hDevMode);

		CString text = dlg.PrintSelection() ? mEdit.GetSelText() : mEdit.GetText();

		//DrawTextEx() will do its own wrapping based on the page width,
		//so remove the control's wrapping
		if (mEdit.GetWordWrap())
			RemoveCR(text);

		CDC printerDc;
		printerDc.Attach(dlg.GetPrinterDC());

		if ((HDC)printerDc == NULL) {
			DisplayErrorMessage(_T("Printer not available."));
			return;
		}

		printerDc.SetMapMode(MM_TEXT);

		CString fileNameWithExt = GetFileNameWithExt(GetFileName());

		//Don't print the file name on the page if the name is the
		//one from File>>New. In that case, the user is probably
		//just using Lite Edit to print pasted text and probably
		//doesn't want to see Untitle.cpp at the top of every page.
		CString title;
		if (!HasNewFileName())
			title = fileNameWithExt;

		CString docName = ProgramName + " - " + fileNameWithExt;

		DOCINFO info;
		InitSized(info);
		info.lpszDocName = docName;

		if (printerDc.StartDoc(&info) <= 0) {
			DisplayErrorMessage(_T("The printer did not initalize."));
			return;
		}

		UINT flags = DT_EXPANDTABS|DT_TABSTOP|DT_LEFT|DT_TOP|DT_WORDBREAK|DT_EDITCONTROL|DT_NOPREFIX;
		DRAWTEXTPARAMS params;
		InitSized(params);
		params.iTabLength = mEdit.GetTabWidth();

		LOGFONT lf;
		GetLogFont(mEdit.GetFontObject(), lf);

		// Scale the font from mEdit to the printer.
		// This makes an inch on the screen equal an inch on the printer.
		// It's also equivalent to converting the lf.lfHeight to an mEdit point size
		// and then using that point size for the printer.
		{
			EditCtrl::CurDC editCtrlDc(mEdit);
			lf.lfHeight = MulDiv(lf.lfHeight,
				printerDc.GetDeviceCaps(LOGPIXELSY),
				editCtrlDc.GetDeviceCaps(LOGPIXELSY));
		}

		CFont font;
		VERIFY(font.CreateFontIndirect(&lf));

		CGdiObject* oldFont = printerDc.SelectObject(&font);

		TEXTMETRIC tm;
		MemClear(tm);
		VERIFY(printerDc.GetTextMetrics(&tm));
		const long lineHeight = tm.tmHeight + tm.tmExternalLeading;

		//get the printable area
		CRect rcPage(0, 0, printerDc.GetDeviceCaps(HORZRES), printerDc.GetDeviceCaps(VERTRES));

		//subtract some for margins
		rcPage.DeflateRect(rcPage.Width() / 30, rcPage.Height() / 30);

		//Text starts two lines after the page so there's room for
		//the header and a blank line.
		CRect rcText = rcPage;
		rcText.top += 2 * lineHeight;

		for (int copies = 0; copies < dlg.GetCopies(); ++copies) {
			int pageNum = 1;
			long numCharsDrawn = 0;

			while (numCharsDrawn < text.GetLength()) {
				VERIFY(printerDc.StartPage() > 0);

				//have to reselect the font and set the map mode in
				//every page for windows 95
				printerDc.SetMapMode(MM_TEXT);
				printerDc.SelectObject(&font);

				//draw title
				printerDc.DrawText(title, rcPage, DT_CENTER|DT_TOP|DT_NOPREFIX);

				//draw page number
				CString pageString;
				pageString.Format(_T("%d"), pageNum);
				printerDc.DrawText(pageString, rcPage, DT_RIGHT|DT_TOP|DT_NOPREFIX);
				++pageNum;

				//draw text for current page
				VERIFY(DrawTextEx(printerDc, (LPTSTR)(LPCTSTR)text + numCharsDrawn, text.GetLength() - numCharsDrawn, &rcText, flags, &params) != 0);
				ASSERT(params.uiLengthDrawn > 0);//could result in infinite loop
				numCharsDrawn += params.uiLengthDrawn;

				VERIFY(printerDc.EndPage() > 0);
			}
		}

		printerDc.SelectObject(oldFont);

		VERIFY(printerDc.EndDoc() > 0);
	}
}

void CLiteEditDlg::UpdateModified() {
	if (mHasModifiedMarker != IsFileModified())
		UpdateTitle();
}

void CLiteEditDlg::UpdateTitle() {
	mHasModifiedMarker = IsFileModified();
	CString title = GetFileName();//??? use path elipses

	if (mHasModifiedMarker)
		title += '*';

	if (mEdit.GetReadOnly())
		title += TEXT(" [Read Only]");

	SetWindowText(title);
}

void CLiteEditDlg::DecorateAllMenus(bool rebuildToolsMenu) {
	gConfigData.mAccelTable.DecorateMenu(*GetMenu());
	gConfigData.mAccelTable.DecorateMenu(mEdit.GetContextMenu());

	if (rebuildToolsMenu)
		RebuildToolsMenu();
	else
		gConfigData.mCommands.mAccelTable.DecorateMenu(*GetMenu());
}

//calculates size of mEdit and mStatusBar
void CLiteEditDlg::ResizeControls() {
	//note that mStatusBar's height is set autoatically

	CRect rc;
	GetClientRect(rc);
	CRect newEditRc = rc;
	CRect newStatusRc(0, 0, 0, 0);
	if (::IsWindow(mStatusBar.m_hWnd))
		mStatusBar.GetClientRect(newStatusRc);
	newEditRc.bottom = Max(newEditRc.top, newEditRc.bottom - (long)newStatusRc.Height());
	newStatusRc.OffsetRect(rc.left, rc.bottom - newStatusRc.Height());
	newStatusRc.right = newStatusRc.left + rc.Width();

	if (::IsWindow(mStatusBar.m_hWnd))
		mStatusBar.MoveWindow(newStatusRc);

	if (mEdit.IsWindow()) {
		//I only want to invalidate what needs to be redrawn because there would be
		//a lot of flickering otherwise
		CRect oldRc;
		mEdit.GetClientRect(oldRc);
		mEdit.MoveWindow(newEditRc, FALSE);
		MapWindowPoints(CWnd::FromHandle(mEdit.m_hWnd), &newEditRc);

		CRgn oldRgn;
		oldRgn.CreateRectRgnIndirect(oldRc);
		CRgn newRgn;
		newRgn.CreateRectRgnIndirect(newEditRc);
		newRgn.CombineRgn(&newRgn, &oldRgn, RGN_DIFF);
		mEdit.InvalidateRgn(newRgn);
		mEdit.SendMessage(WM_NCPAINT, 1);//redraw non-client area
	}
}

//updates the pane that displays the row and column number of the caret
void CLiteEditDlg::UpdatePosPane() {
	CString text;

	long line, col;
	mEdit.GetLineAndColNum(line, col);
	text.Format(TEXT("Ln %d, Col %d"), line, col);

	mStatusBar.SetText(text, SBI_POS, 0);
}

//updates the pane that displays the clipboard
void CLiteEditDlg::UpdateClipboardPane(bool clipboardChanged) {
	CString text, clipboard;
	StringVector files;

	if (GetClipboardString(m_hWnd, text, true)) {
		clipboard = text;

		//I delete most of the characters from the text because
		//mStatusBar.SetText() is slow for large text. Interestingly,
		//a status bar doesn't seem to support displaying more than 127
		//characters anyway!
		const long MostCharsNeeded = 500;
		if (text.GetLength() > MostCharsNeeded)
			text.Delete(MostCharsNeeded, text.GetLength() - MostCharsNeeded);

		text.Replace(TEXT("\n"), TEXT("\\n"));
		text.Replace(TEXT("\t"), DupChar(' ', mEdit.GetTabWidth()));
	} else if (GetClipboardFiles(m_hWnd, files)) {
		text = files.size() == 1 ? _T("File: ") : _T("Files: ");
		text += files.Concat(_T("; "));
	} else
		text = TEXT("<No text is in the clipboard>");

	mStatusBar.SetText(text, SBI_CLIPBOARD, 0);

	if (clipboardChanged && mCurrentClipboard != clipboard) {
		gConfigData.mLastClipboard = mCurrentClipboard;
		mCurrentClipboard = clipboard;
	}
}

//Edit>>Advanced>>Insert Time/Date command.
void CLiteEditDlg::InsertTimeDate() {
	CString string = GetTimeDateString(gConfigData.mTimeDateFormat);
	mEdit.ReplaceSel(string);
}

//Edit>>Advanced>>UPPER CASE, lower case, and Title Case commands.
void CLiteEditDlg::ChangeSelCase(CaseKind kind) {
	CString sel = mEdit.GetSelText();

	switch (kind) {
	case ckLower :
		sel.MakeLower();
		break;
	case ckUpper :
		sel.MakeUpper();
		break;
	case ckTitle :
		MakeTitleCase(sel);
		break;
	case ckToggle :
		MakeToggleCase(sel);
		break;
	}

	mEdit.ReplaceSel(sel, true);//true to select replacment
}

//File>>Recent>>[recent file] and Tool>>[tool] commands.
bool CLiteEditDlg::ProcessDynamicCommand(UINT id) {
	CMenu* recentMenu = GetMenu()->GetSubMenu(MP_FILE)->GetSubMenu(MP_RECENT);
	int pos = FindMenuItemPosById(*recentMenu, id);

	if (0 <= pos && pos < gConfigData.mRecentFileList.GetItems().size()) {
		if (PromptToSave()) {
			//must copy file name because LoadFile() will modify
			//the recent file list which may change the file name unexpectedly
			CString fileName = gConfigData.mRecentFileList.GetItems()[pos];
			LoadFile(fileName, false);
		}

		return true;
	}

	Command* command = gConfigData.mCommands.FindCommand(id);
	if (command != NULL) {
		ExecuteCommand(*command);
		return true;
	}

	return false;
}

void CLiteEditDlg::ExecuteCommand(const Command& cmd) {
	bool cancel = cmd.mSaveFile && !Save();

	if (!cancel)
		gConfigData.Execute(cmd, GetFileName(), m_hWnd, mEdit);
}

void CLiteEditDlg::ShowFindOrReplaceDialog(bool find) {
	if (IsWindow(mFindRelaceDlgPtr->GetSafeHwnd()))
		VERIFY(mFindRelaceDlgPtr->DestroyWindow());

	gConfigData.mFindReplaceData.selectionOnly = false;
	CString curWord;

	//If there's a selection use that as the current word, unless the selection spans
	//lines in which case there current word will be empty. If there is no selection,
	//use the word at the caret as the current word.
	//selectionOnly will be set to true if the selection spans lines.

	if (mEdit.HasSel()) {
		curWord = mEdit.GetSelText();
		if (curWord.Find('\n') >= 0) {//if selection spans lines
			curWord = EmptyStr;

			// Don't enable selectionOnly when read only to prevent confusing the
			// user by displaying a Replace dialog where everything is disabled
			// until you uncheck search selection only. For a read only file, in the
			// Replace dialog you can only use Find. However, Find is always disabled
			// when using search selection only. When not using a read only file
			// since Replace All becomes enabled when you enter find text, there
			// shouldn't be confusion enabling search selection only.
			if (!mEdit.GetReadOnly())
				gConfigData.mFindReplaceData.selectionOnly = true;
		}
	} else {
		long start, end;
		mEdit.GetWordAt(mEdit.GetSelCaret(), start, end);
		curWord = mEdit.GetTextRange(start, end);
		curWord.TrimLeft(_T("\r\n"));
		curWord.TrimRight(_T("\r\n"));
	}

	mFindRelaceDlgPtr = auto_ptr<FindReplaceDlg>(new FindReplaceDlg(find, curWord, mEdit.GetReadOnly()));
	mFindRelaceDlgPtr->Show(this);
}

void CLiteEditDlg::GetFindReplaceDlgOutOfTheWay() {
	if (IsWindow(mFindRelaceDlgPtr->GetSafeHwnd()) &&
		mFindRelaceDlgPtr->IsWindowVisible() &&
		mEdit.HasSel()) {
		long start, end;
		mEdit.GetSelRange(start, end);
		CPoint startPt = mEdit.PosFromChar(start);
		CPoint endPt = mEdit.PosFromChar(start);
		endPt.y += mEdit.GetLineHeight();
		mEdit.ClientToScreen(&startPt);
		mEdit.ClientToScreen(&endPt);

		CRect rc;
		mFindRelaceDlgPtr->GetWindowRect(&rc);

		long amountAbove = startPt.y - rc.bottom;
		long amountBelow = rc.top - endPt.y;

		if (amountAbove < 0 && amountBelow < 0) {//if dialog needs to be moved
			//we'll move the find window to middle of space above or space below,
			//depending which has more room

			CRect rcMonitor = GetMonitorRect(m_hWnd);
			long halfHeight = rc.Height() / 2;
			//use averages to calculate middle of space above and below
			long moveUpToY = (rcMonitor.top + startPt.y) / 2 - halfHeight;
			long moveDownToY = (rcMonitor.bottom + endPt.y) / 2 - halfHeight;

			if (moveUpToY - rcMonitor.top > rcMonitor.bottom - moveDownToY - rc.Height())//if there's more room to move up
				rc.OffsetRect(0, moveUpToY - rc.top);
			else
				rc.OffsetRect(0, moveDownToY - rc.top);

			mFindRelaceDlgPtr->MoveWindow(&rc);
		}
	}
}

void CLiteEditDlg::DoFindOrReplace() {
	//dlg can be either this window or the find/replace dialog. dlg should be the parent
	//of all dialogs displayed in this method.
	CWnd* dlg = mFindRelaceDlgPtr->SafeIsVisible() ? (CWnd*)mFindRelaceDlgPtr.get() : (CWnd*)this;
	FindReplaceData& data = gConfigData.mFindReplaceData;

	// need ffIgnoreCRs to find text that spans wrapped lines
	ASSERT((data.flags & ffIgnoreCRs) != 0);

	const CString& text = mEdit.GetText();
	bool matchCase = (data.flags & ffMatchCase) != 0;
	bool searchDown = (data.flags & ffReverse) == 0;

	if (data.replaceAll && !mEdit.GetReadOnly()) {
		WaitCursor wc;
		long numReplacments = 0;

		if (data.selectionOnly) {
			CString selText = mEdit.GetSelText();
			numReplacments = ReplaceSubstring(selText, data.findStr, data.replaceStr, data.flags);
			if (numReplacments > 0)
				mEdit.ReplaceSel(selText, true);
		} else {
#if 1
			long fvl = mEdit.GetFirstVisibleLine();
			long fvp = mEdit.GetFirstVisiblePixel();
			long sel[2];
			mEdit.GetSel(sel[0], sel[1]);

			const CString origText = mEdit.GetText();
			CString text = origText;
			numReplacments = ReplaceSubstring(text, data.findStr, data.replaceStr, data.flags, 0, -1, sel, 2);

			if (numReplacments > 0) {
				long diffStart, diff1Len, diff2Len;
				DiffStrings(origText, text, diffStart, diff1Len, diff2Len);
				CString replacmentString = text.Mid(diffStart, diff2Len);

				//adjustments for ModifyText - remove CRs from replacmentString and fix up
				//sel for those removed CRs
				RemoveCR(replacmentString);
				ConvertCRLFIndicesToLFIndices(text, sel, 2, diffStart, diffStart + diff2Len);

				mEdit.ModifyText(diffStart, diff1Len, replacmentString, sel[0], sel[1], skNone);

				mEdit.SetFirstVisibleLine(fvl);
				mEdit.SetFirstVisiblePixel(fvp);
			}
#else
			//Old replacement code that did one replace at a time in the text.
			//It creates an undo item for each replacment, but is faster when
			//there's only a few replacments.

			long anchor = mEdit.GetSelAnchor();
			long activeEnd = mEdit.GetSelActiveEnd();
			long pos = 0;
			while (pos < text.GetLength()) {
				pos = FindSubstring(text, data.findStr, data.flags, pos);
				if (pos < 0)
					break;

				bool modify = mEdit.ModifyText(pos, data.findStr.GetLength(), data.replaceStr);
				if (modify) {
					long end = pos + data.findStr.GetLength();
					long lengthDiff = data.replaceStr.GetLength() - data.findStr.GetLength();
					if (anchor >= end)
						anchor += lengthDiff;
					if (activeEnd >= end)
						activeEnd += lengthDiff;

					pos += data.replaceStr.GetLength();
					++numReplacments;
				} else
					break;
			}

			mEdit.SetSel(anchor, activeEnd, skSelection);
#endif
		}

		CString msg;
		msg.Format(TEXT("%d replacments made."), numReplacments);
		dlg->MessageBox(msg, TEXT("Replace All"), MB_OK|MB_ICONINFORMATION);
	} // end replace all

	if (data.replaceCurrent && !mEdit.GetReadOnly())
		//equivalent to if mEdit.GetSelText() == data.findStr using data.flags for comparison
		if (FindSubstring(mEdit.GetSelText(), data.findStr, data.flags) == 0)
			mEdit.ReplaceSel(data.replaceStr, true);

	if (data.findNext) {
		long end = searchDown ? mEdit.GetSelEnd() : mEdit.GetSelStart();
		long start = Min(end, (long)text.GetLength());
		long findEnd = -1;
		long pos = text.IsEmpty() ? -1 : CyclicFindSubstring(text, data.findStr, data.flags, start, end, &findEnd);

		CString findDisplaySring = data.findStr;
		if (data.escapedChars)
			findDisplaySring = EscapeChars(findDisplaySring);

		if (pos >= 0) {
			//reset mInitFindPos if the search data has changed
			bool changedDirection;
			if (data.Changed(mLastSearchData, changedDirection)) {
				mInitFindPos = -1;
				mLastFindPos = -1;
				mRepeatingSearch = false;
				mLastSearchData = data;
			}

			data.newSearch = false;

			//the next find is the same as the first find, so the user has searched
			//through all the text
			if (!mRepeatingSearch && !changedDirection && mInitFindPos == pos) {
				dlg->MessageBox(TEXT("You have finished searching through all the text for '") + findDisplaySring + TEXT("'."), TEXT("Finished Search"), MB_OK|MB_ICONINFORMATION);
				mRepeatingSearch = true;
			}
			//check if the user reached the starting point searching in opposite the
			//original direction
			else if (!mRepeatingSearch && changedDirection && mLastFindPos == mInitFindPos && mLastFindPos >= 0) {
				dlg->MessageBox(TEXT("You have reached the starting point of the search."), TEXT("Search"), MB_OK|MB_ICONINFORMATION);
				mRepeatingSearch = true;
			} else {
				mRepeatingSearch = false;

				mLastFindPos = pos;
				if (mInitFindPos < 0)
					mInitFindPos = mLastFindPos;

				mEdit.SetSel(pos, findEnd, skCenterSelection);
			}
		} else
			dlg->MessageBox(TEXT("There are no occurrences of '") + findDisplaySring + TEXT("'."), TEXT("Finished Search"), MB_OK|MB_ICONINFORMATION);
	}

	GetFindReplaceDlgOutOfTheWay();
}

void CLiteEditDlg::DoFindNextOrPrev(bool next) {
	FindReplaceData& data = gConfigData.mFindReplaceData;

	// User would probably not expect Find Next/Previous to replace after find dialog
	// has been closed.
	if (!mFindRelaceDlgPtr->SafeIsVisible())
		data.replaceAll = data.replaceCurrent = false;

	if (next)
		data.flags &= ~ffReverse;
	else
		data.flags |= ffReverse;

	// If findStr is empty, DoFindOrReplace will never find anything.
	// If findNext is false, then DoFindOrReplace would do nothing.
	if (!data.findStr.IsEmpty() && data.findNext)
		DoFindOrReplace();
	else
		ShowFindOrReplaceDialog(true);
}

void CLiteEditDlg::RebuildToolsMenu() {
	CMenu* toolsMenu = GetMenu()->GetSubMenu(MP_TOOLS);
	mIdCreator.ReleaseSubMenuItemsIds(*toolsMenu);

	HMENU newToolsMenu = LoadSubMenu(IDR_MENU, MP_TOOLS);
	gConfigData.mCommands.AddCommandsToMenu(newToolsMenu, GetFileExt(GetFileName()), mIdCreator);
	SetSubMenu(*GetMenu(), MP_TOOLS, newToolsMenu);
}

void CLiteEditDlg::DoGoto() {
	CGotoDlg dlg(mEdit.GetLineCount());
	if (dlg.DoModal() == IDOK)
		mEdit.GotoLfLine(dlg.GetLineNumber() - 1);
}

void CLiteEditDlg::DisplayToolsDlg() {
	ToolsDlg dlg(GetFileName());
	if (dlg.DoModal() == IDOK)
		RebuildToolsMenu();
}

void CLiteEditDlg::DisplayFontDlg() {
	LOGFONT lf;
	MemClear(lf);
	CFont& font = gConfigData.mFont;
	GetLogFont(font, lf);
	CFontDialog dlg(&lf, CF_FORCEFONTEXIST|CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS, NULL, this);

	if (dlg.DoModal() == IDOK) {
		dlg.GetCurrentFont(&lf);
		VERIFY(font.DeleteObject());
		font.CreateFontIndirect(&lf);
		mEdit.SetFont(font);
	}
}

void CLiteEditDlg::DisplayEditorOptionsDlg() {
	EditorOptionsDlg dlg;
	if (dlg.DoModal() == IDOK) {
		gConfigData.SetEdit(mEdit);
		UpdateClipboardPane(false);//tab width may have changed
	}
}

void CLiteEditDlg::DisplaySyntaxColoringDlg() {
	SyntaxColoringDlg dlg(mEdit.GetLanguage()->GetName());
	//Apply config data even if user hits cancel because the user could have
	//applied changes (i.e. Apply button) and then cancelled the dialog.
	dlg.DoModal();
	WaitCursor wc;
	gConfigData.SetEdit(mEdit);
	SetLangFromFileExt();
}

void CLiteEditDlg::DisplayHotKeysDlg() {
	//make File>>Recent empty so users can't configure hotkeys for
	//recent menu items
	CMenu* fileMenu = GetMenu()->GetSubMenu(MP_FILE);
	SetSubMenu(*fileMenu, MP_RECENT, CreatePopupMenu());

	HotKeysDlg dlg(*GetMenu());
	if (dlg.DoModal() == IDOK)
		DecorateAllMenus(false);
}

void CLiteEditDlg::ToggleWordWrap() {
	UINT state = GetMenu()->GetMenuState(ID_OPTIONS_WORDWRAP, MF_BYCOMMAND);
	bool wordWrap = (state & MF_CHECKED) != 0;

	wordWrap = !wordWrap;

	mEdit.SetWordWrap(wordWrap);
	gConfigData.mWordWrap = wordWrap;
}

/////////////////////////////////////////////////////////////////////////////
// CLiteEditDlg message handlers

BOOL CLiteEditDlg::OnCommand(WPARAM wParam, LPARAM lParam) {
	WORD id = LOWORD(wParam), src = HIWORD(wParam);
	bool handled = true;

	if (src == 0 || src == 1) {//if from menu or accelerator
		switch (id) {
		case ID_FILE_NEW :
			NewFile();
			break;
		case ID_FILE_OPEN :
			OpenFile();
			break;
		case ID_FILE_SAVE :
			Save();
			break;
		case ID_FILE_SAVEAS :
			SaveAs();
			break;
		case ID_FILE_PRINT :
			Print();
			break;
		case ID_FILE_EXIT :
			if (PromptToSave())
				PostQuitMessage(0);
			break;
		case ID_EDIT_UNDO :
			mEdit.Undo();
			break;
		case ID_EDIT_REDO :
			mEdit.Redo();
			break;
		case ID_EDIT_CUT :
			mEdit.Cut();
			break;
		case ID_EDIT_COPY :
			mEdit.Copy();
			break;
		case ID_EDIT_PASTE :
			mEdit.Paste();
			break;
		case ID_EDIT_DELETE :
			mEdit.ReplaceSel(EmptyStr);
			break;
		case ID_EDIT_SELECTALL :
			mEdit.SelectAll();
			break;
		case ID_EDIT_ADVANCED_LASTCLIPBOARD :
			gConfigData.LastClipboard(m_hWnd);
			break;
		case ID_EDIT_ADVANCED_SWITCHCLIPBOARD :
			gConfigData.SwitchClipboard(m_hWnd);
			break;
		case ID_EDIT_ADVANCED_ENCODING_ASCII :
			mFileInfo.fileFormat = ffAscii;
			UpdateModified();
			break;
		case ID_EDIT_ADVANCED_ENCODING_UTF16 :
			mFileInfo.fileFormat = ffUTF16;
			UpdateModified();
			break;
		case ID_EDIT_ADVANCED_ENCODING_UTF8 :
			mFileInfo.fileFormat = ffUTF8;
			UpdateModified();
			break;
		case ID_EDIT_ADVANCED_ENCODING_UNKNOWN :
			// Should only be able to set to binary when the disk file is still binary.
			// This should only happen if you open a binary file, change the in memory
			// format, then change in the in memory format back to binary.
			ASSERT(mFileInfo.fileFormatOnDisk == ffBinary);
			mFileInfo.fileFormat = ffBinary;
			UpdateModified();
			break;
		case ID_EDIT_ADVANCED_NEW_LINE_TYPE_CRLF :
			mFileInfo.newLineType = nlCRLF;
			UpdateModified();
			break;
		case ID_EDIT_ADVANCED_NEW_LINE_TYPE_LF :
			mFileInfo.newLineType = nlLF;
			UpdateModified();
			break;
		case ID_EDIT_ADVANCED_NEW_LINE_TYPE_CR :
			mFileInfo.newLineType = nlCR;
			UpdateModified();
			break;
		case ID_EDIT_ADVANCED_INSERTDATETIME :
			InsertTimeDate();
			break;
		case ID_EDIT_ADVANCED_EDIT_TIMEDATE_FORMAT:
			TimeDateFormatDlg().DoModal();
			break;
		case ID_EDIT_ADVANCED_LOWERCASE :
			ChangeSelCase(ckLower);
			break;
		case ID_EDIT_ADVANCED_UPPERCASE :
			ChangeSelCase(ckUpper);
			break;
		case ID_EDIT_ADVANCED_TITLECASE :
			ChangeSelCase(ckTitle);
			break;
		case ID_EDIT_ADVANCED_TOGGLECASE :
			ChangeSelCase(ckToggle);
			break;
		case ID_SEARCH_FIND :
			ShowFindOrReplaceDialog(true);
			break;
		case ID_SEARCH_FINDNEXT :
			DoFindNextOrPrev(true);
			break;
		case ID_SEARCH_FINDPREVIOUS :
			DoFindNextOrPrev(false);
			break;
		case ID_SEARCH_REPLACE :
			ShowFindOrReplaceDialog(false);
			break;
		case ID_SEARCH_GOTO :
			DoGoto();
			break;
		case ID_BOOKMARKS_TOGGLE :
			mEdit.ToggleBookmark();
			break;
		case ID_BOOKMARKS_NEXT :
			mEdit.NextBookmark(true);
			break;
		case ID_BOOKMARKS_PREVIOUS :
			mEdit.NextBookmark(false);
			break;
		case ID_BOOKMARKS_CLEARALL :
			mEdit.ClearAllBookmarks();
			break;
		case ID_TOOLS_EDIT :
			DisplayToolsDlg();
			break;
		case ID_OPTIONS_FONT :
			DisplayFontDlg();
			break;
		case ID_OPTIONS_EDITOR :
			DisplayEditorOptionsDlg();
			break;
		case ID_OPTIONS_SYNTAXCOLORING :
			DisplaySyntaxColoringDlg();
			break;
		case ID_OPTIONS_HOTKEYS :
			DisplayHotKeysDlg();
			break;
		case ID_OPTIONS_WORDWRAP :
			ToggleWordWrap();
			break;
		case ID_HELP_CONTENTS :
			gConfigData.ExecuteHelp(hfContents, m_hWnd);
			break;
		case ID_HELP_ABOUT :
			AboutDlg().DoModal();
			break;
		default :
			handled = ProcessDynamicCommand(id);
		}
	} else
		handled = false;

	return handled ? 1 : CBase::OnCommand(wParam, lParam);
}

BOOL CLiteEditDlg::OnInitDialog()
{
	//process this command line early instead of in ProcessCommandLine() because
	//Lite Edit exits after displaying the help and I don't want the application
	//to window show before exiting
	if (mCmdLineInfo.mShowUsage)
		ShowUsage();

	CDialog::OnInitDialog();

	SetIcon(mLargeIcon, TRUE);			// Set big icon
	SetIcon(mSmallIcon, FALSE);			// Set small icon

	gConfigData.InitDefaultValues();

	mIsMaximized = false;
	mInitFindPos = mLastFindPos = -1;
	mRepeatingSearch = false;
	mPanePosValid = false;
	mCheckExternalModify = false;

	DWORD style = WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL;
	DWORD exStyle = WS_EX_CLIENTEDGE;
	mEdit.Create(m_hWnd, CRect(0, 0, 0, 0), style, exStyle, ID_EDIT_CTRL);

	int widths[2] = {100, -1};
	VERIFY(mStatusBar.Create(WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, CRect(0, 0, 0, 0), this, 0));
	VERIFY(mStatusBar.SetParts(sizeof(widths) / sizeof(int), widths));

	ResizeControls();

	mNextClipViewer = SetClipboardViewer();

	gConfigData.Load();

	gConfigData.SetEdit(mEdit);

	HMENU editMenu = LoadSubMenu(IDR_MENU, 1);
	mEdit.SetContextMenu(editMenu);

	DecorateAllMenus(true);

	if (gConfigData.mMaximized || !gConfigData.mWinPos.IsRectEmpty()) {
		WINDOWPLACEMENT winPlacement;
		InitLengthed(winPlacement);

		winPlacement.showCmd = gConfigData.mMaximized ? SW_MAXIMIZE : SW_SHOWNORMAL;
		winPlacement.rcNormalPosition = gConfigData.mWinPos;

		//makes sure window is maximized in the correct window for multi monitor
		winPlacement.ptMaxPosition = gConfigData.mWinPos.TopLeft();

		VERIFY(SetWindowPlacement(&winPlacement));
	}

	DragAcceptFiles();

	ProcessCommandLine();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CLiteEditDlg::OnDestroy() {
	//remove Lite Edit from the clipboard viewer chain
	ChangeClipboardChain(mNextClipViewer);

	WINDOWPLACEMENT winPlacement;
	InitLengthed(winPlacement);
	VERIFY(GetWindowPlacement(&winPlacement));

	gConfigData.mMaximized = winPlacement.showCmd == SW_MAXIMIZE;
	gConfigData.mWinPos = winPlacement.rcNormalPosition;

	gConfigData.Save();

	SafeDestroyIcon(mSmallIcon);
	SafeDestroyIcon(mLargeIcon);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLiteEditDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, mLargeIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

void CLiteEditDlg::OnChangeCbChain(HWND hWndRemove, HWND hWndAfter) {
	if (mNextClipViewer == hWndRemove)
		mNextClipViewer = hWndAfter;
	::SendMessage(mNextClipViewer, WM_CHANGECBCHAIN, (WPARAM)hWndRemove, (LPARAM)hWndAfter);
}

void CLiteEditDlg::OnDrawClipboard() {
	UpdateClipboardPane(true);
	::SendMessage(mNextClipViewer, WM_DRAWCLIPBOARD, 0, 0);
}

HCURSOR CLiteEditDlg::OnQueryDragIcon()
{
	return (HCURSOR) mLargeIcon;
}

void CLiteEditDlg::OnSize(UINT nType, int cx, int cy) {
	CBase::OnSize(nType, cx, cy);
	ResizeControls();

	//if maximizing or used to be maximized and not minimized
	if (nType == SIZE_MAXIMIZED || (mIsMaximized && nType != SIZE_MINIMIZED))
		mEdit.RewrapAllLinesIfNecessary();

	mIsMaximized = nType == SIZE_MAXIMIZED;
}

LRESULT CLiteEditDlg::OnExitSizeMove(WPARAM wParam, LPARAM lParam) {
	//rewrapping is too slow to do on every size change, so we only rewrap
	//when we finish resizing (or on maximize or restore, see OnSize())
	mEdit.RewrapAllLinesIfNecessary();
	return 0;
}

void CLiteEditDlg::OnClose() {
	if (PromptToSave())
		CBase::OnClose();
}

void CLiteEditDlg::OnInitMenu(CMenu* menu) {
	ATLASSERT(menu != NULL && IsMenu(*menu));

	const bool hasSel = mEdit.HasSel();
	const bool hasText = mEdit.GetTextLength() > 0;
	const bool isWriteable = mEdit.IsWriteable();

	//enable menu items only in the application menu
	if (menu == GetMenu()) {
		SetMenuItemEnabled(*menu, ID_FILE_SAVE, CanSave());
		
		//put a check mark by the wordwrap options if necessary
		UINT state = mEdit.GetWordWrap() ? MF_CHECKED : MF_UNCHECKED;
		GetMenu()->CheckMenuItem(ID_OPTIONS_WORDWRAP, MF_BYCOMMAND | state);

		//*** dynamically create the File>>Recent menu ***

		CMenu* fileMenu = menu->GetSubMenu(MP_FILE);
		//release ids of menu that is about to be destroyed
		mIdCreator.ReleaseSubMenuItemsIds(*fileMenu->GetSubMenu(MP_RECENT));

		HMENU recentSubMenu = gConfigData.mRecentFileList.CreateMenu(mIdCreator);
		ASSERT(IsMenu(recentSubMenu));
		SetSubMenu(*fileMenu, MP_RECENT, recentSubMenu);
		bool enableRecent = GetMenuItemCount(recentSubMenu) > 0;
		SetMenuItemEnabled(*fileMenu, MP_RECENT, enableRecent, MF_BYPOSITION);

		// Make sure caret is always visible when main menu is showing since some items
		// act on current line. The control already does this for the context menu.
		mEdit.ForceShowCaret();
	}

	//enable menu items in both the application menu and context menu
	if (menu == GetMenu() || *menu == mEdit.GetContextMenu()) {
		SetMenuItemEnabled(*menu, ID_EDIT_UNDO, mEdit.CanUndo());
		SetMenuItemEnabled(*menu, ID_EDIT_REDO, mEdit.CanRedo());
		SetMenuItemEnabled(*menu, ID_EDIT_CUT, mEdit.CanCut());
		SetMenuItemEnabled(*menu, ID_EDIT_COPY, mEdit.CanCopy());
		SetMenuItemEnabled(*menu, ID_EDIT_PASTE, mEdit.CanPaste());
		SetMenuItemEnabled(*menu, ID_EDIT_DELETE, hasSel && isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_SELECTALL, hasText);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_LASTCLIPBOARD, gConfigData.CanLastClipboard());
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_SWITCHCLIPBOARD, gConfigData.CanSwitchClipboard());
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_INSERTDATETIME, isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_LOWERCASE, hasSel && isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_UPPERCASE, hasSel && isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_TITLECASE, hasSel && isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_TOGGLECASE, hasSel && isWriteable);

		NewLineType newLineType = gConfigData.CalcNewLineType(mFileInfo.newLineType);
		menu->CheckMenuItem(ID_EDIT_ADVANCED_NEW_LINE_TYPE_CRLF, newLineType == nlCRLF ? MF_CHECKED : MF_UNCHECKED);
		menu->CheckMenuItem(ID_EDIT_ADVANCED_NEW_LINE_TYPE_LF, newLineType == nlLF ? MF_CHECKED : MF_UNCHECKED);
		menu->CheckMenuItem(ID_EDIT_ADVANCED_NEW_LINE_TYPE_CR, newLineType == nlCR ? MF_CHECKED : MF_UNCHECKED);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_NEW_LINE_TYPE_CRLF, newLineType == nlCRLF || isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_NEW_LINE_TYPE_LF, newLineType == nlLF || isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_NEW_LINE_TYPE_CR, newLineType == nlCR || isWriteable);

		menu->CheckMenuItem(ID_EDIT_ADVANCED_ENCODING_ASCII, mFileInfo.fileFormat == ffAscii ? MF_CHECKED : MF_UNCHECKED);
		menu->CheckMenuItem(ID_EDIT_ADVANCED_ENCODING_UTF16, mFileInfo.fileFormat == ffUTF16 ? MF_CHECKED : MF_UNCHECKED);
		menu->CheckMenuItem(ID_EDIT_ADVANCED_ENCODING_UTF8, mFileInfo.fileFormat == ffUTF8 ? MF_CHECKED : MF_UNCHECKED);
		menu->CheckMenuItem(ID_EDIT_ADVANCED_ENCODING_UNKNOWN, mFileInfo.fileFormat == ffBinary ? MF_CHECKED : MF_UNCHECKED);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_ENCODING_ASCII, mFileInfo.fileFormat == ffAscii || isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_ENCODING_UTF16, mFileInfo.fileFormat == ffUTF16 || isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_ENCODING_UTF8, mFileInfo.fileFormat == ffUTF8 || isWriteable);
		SetMenuItemEnabled(*menu, ID_EDIT_ADVANCED_ENCODING_UNKNOWN, mFileInfo.fileFormatOnDisk == ffBinary || mFileInfo.fileFormat == ffBinary); // can only set to unknown when disk file is unknown; also show enabled if unknown is checked
	}
}

void CLiteEditDlg::OnEditChange() {
	UpdateModified();
}

void CLiteEditDlg::OnEditSelChange() {
	mPanePosValid = false;

	//if selected has changed because of anything but a find, reset the starting
	//find position
	if (mLastFindPos != mEdit.GetSelStart())
		mInitFindPos = -1;
}

LPARAM CLiteEditDlg::OnFindReplace(WPARAM wParam, LPARAM lParam) {
	DoFindOrReplace();
	return 0;
}

void CLiteEditDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) {
	if (nState != WA_INACTIVE &&
		!bMinimized &&
		!mCheckExternalModify)
	{
		AutoBool autoCheckExternalModify(mCheckExternalModify, true);

		if (mFileInfo.exists) {
			DWORD attrs = GetFileAttributes(GetFileName());

			if (attrs == -1)//file doesn't exist. must have been deleted while being edited.
				mFileInfo.exists = false;
			else {
				bool readOnly = (attrs & FILE_ATTRIBUTE_READONLY) != 0;
				if (readOnly != mEdit.GetReadOnly()) {
					mEdit.SetReadOnly(readOnly);
					UpdateTitle();

					if (IsWindow(mFindRelaceDlgPtr->GetSafeHwnd()))
						mFindRelaceDlgPtr->SetReadOnly(readOnly);
				}

				FILETIME ft = GetFileModifyTime(GetFileName());
				if (CompareFileTime(&ft, &mFileInfo.modifyTime) > 0) {
					CString msg;
					msg.Format(TEXT("File '%s' has been modified externally."), GetFileName());

					if (IsFileModified())
						msg += _T("\n\nDo you want to reload the file and lose your changes?");
					else
						msg += _T("\n\nDo you want to reload the file?");
					
					if (MessageBox(msg, TEXT("Reload File"), MB_YESNO|MB_ICONQUESTION) == IDYES) {
						//reload file preserving line number and line offset

						long line = mEdit.GetCaretLine();
						long offset = mEdit.GetSelCaret() - mEdit.CharIndexOfLine(line);
						long fvl = mEdit.GetFirstVisibleLine();
						long fvp = mEdit.GetFirstVisiblePixel();

						LoadFile(GetFileName(), false);

						mEdit.SetFirstVisibleLine(fvl);
						mEdit.SetFirstVisiblePixel(fvp);

						line = Min(line, mEdit.GetLineCount() - 1L);
						long pos = mEdit.CharIndexOfLine(line) + Min(offset, mEdit.GetLineLength(line));
						mEdit.SetSelCaret(pos);
					} else
						//To keep from prompting again.
						//Note that I call GetFileModifyTime() again instead of
						//assigning ft to modifyTime because the file could have
						//been modified again since ft was set and I don't want to
						//prompt to reload more than once.
						mFileInfo.modifyTime = GetFileModifyTime(GetFileName());
				}
			}
		}

		if (gConfigData.ReloadIfModified(mEdit))
			DecorateAllMenus(true);
	}

	CDialog::OnActivate(nState, pWndOther, bMinimized);
}

//Overridden to keep error message from coming up. OnCommand() will display the help.
BOOL CLiteEditDlg::OnHelpInfo(HELPINFO* pHelpInfo) {
	return TRUE;
}

//I can get OnHelp() here if in a tab dialog you press F1 while focus in on a control
//that is not on a tab (e.g. OK, Cancel, and Help buttons). Since I don't want an
//error message to come up, I'll just show the main help page.
void CLiteEditDlg::OnHelp() {
	gConfigData.ExecuteHelp(hfContents, m_hWnd);	
}

BOOL CLiteEditDlg::OnQueryEndSession() {
	return PromptToSave();
}

void CLiteEditDlg::OnDropFiles(HDROP hDropInfo) {
	if (PromptToSave()) {
		CString path = DragQueryFile(hDropInfo, 0);
		LoadFile(path, false);
	}
}

LRESULT CLiteEditDlg::OnKickIdle(WPARAM wParam, LPARAM lParam) {
	//modifying the status bar text is so slow it has to
	//be done on idle
	if (!mPanePosValid) {
		UpdatePosPane();
		mPanePosValid = true;
	}

	return 0;
}
