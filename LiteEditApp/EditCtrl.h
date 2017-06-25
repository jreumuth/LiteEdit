#ifndef EditCtrlH
#define EditCtrlH

#include "Utility.h"
#include "CodeColoring.h"
#include <memory>
#include <limits>

const WORD ECN_CHANGE = 1;
const WORD ECN_SELCHANGE = 2;

#if 0
const WORD ECN_ = ;
#endif

//skCaret - Bring caret into view, e.g. changing selection with arrow keys
//skSelection - Bring caret into view, and as much of the selection as possible
//skCenterSelection - same as skSelection but center caret vertically (for now,
// this doesn't bother to center horizontally).
enum ScrollKinds {skNone, skCaret, skSelection, skCenterSelection};

//The text EditCtrl stores internally (i.e. m_text) uses lines feeds,
//or '\n', for a hard carriage returns (e.g. the enter key) and
//it uses carriage returns, or '\r', for wrapped lines. This means:
//1. num lines feeds (LFs) + num carriage returns (CRs) + 1 = num lines
//2. if GetWordWrap() is false, then num CRs = 0
//3. all line indices, such as the the line number you pass to GetLine(),
//   specify a visible line index, or line number relative to LFs and CRs,
//   not a line number relative to only LFs
class EditCtrl : public CWindowImpl<EditCtrl> {
public:
	class CurDC : public CDC {
	private:
		const EditCtrl& m_ctrl;
		CFont* m_oldFont;
		bool m_release;

		//returns the maximum number of characters you can pass to
		//GetTabbedTextExtent() and TabbedTextOut()
		long GetMaxCharsForTabbedTextFunctions() const {
			//8192 is the most characters you can pass to the tabbed text functions
			//on windows 95/98. Also, only a WORD is used for the returned with,
			//so don't pass more characters than the width of which can be stored
			//in a WORD.
			return Min(8192L, 0xFFFFL / m_ctrl.GetMaxCharWidth());
		}

	public:
		CurDC(const EditCtrl& ctrl, HDC hdc = NULL) : m_ctrl(ctrl), m_release(hdc == NULL) {
			Attach(hdc == NULL ? ::GetDC(m_ctrl) : hdc);
			m_oldFont = SelectObject(CFont::FromHandle(m_ctrl.GetFont()));
		}

		~CurDC() {
			SelectObject(m_oldFont);
			if (m_release)
				::ReleaseDC(m_ctrl, *this);
			Detach();
		}

		int GetLineWidth(long line, long lastChar) const {
			return GetLineWidth((LPCTSTR)m_ctrl.GetLine(line), lastChar);
		}

		int GetLineWidth(const CString& line) const {
			return GetLineWidth((LPCTSTR)line, line.GetLength());
		}

		int GetLineWidth(LPCTSTR line, long length) const;
		int DrawText(int x, int y, const CString& text);
	};
	friend CurDC;

private:
	class LineData : public Obj {
	private:

	public:
		LineData() : m_hasBookmark(false) {Invalidate();}

		long m_width;
		CodeLinePtr m_codeLine;
		bool m_codeLineValid;
		CString m_line;
		bool m_lineValid;
		long m_charIndexOfLine;
		bool m_hasBookmark;

		//Line index only going by LF lines. Same as the line index if word wrap is off.
		long m_lfLineIndex;

		void Invalidate() {
			m_width = m_charIndexOfLine = m_lfLineIndex = -1;
			m_codeLineValid = m_lineValid = false;
		}

		void InvalidateCodeLine() {m_codeLineValid = false;}
	};
	PTR_DEF(LineData);

	typedef vector<LineDataPtr> Lines;

	struct UndoItem {
		CString added;
		CString deleted;
		long pos;

		UndoItem(long _pos, const CString& _deleted, const CString& _added) :
			pos(_pos), deleted(_deleted), added(_added) {}

		UndoItem(long _pos, TCHAR _deleted, TCHAR _added) :
			pos(_pos), deleted(CharToStr(_deleted)), added(CharToStr(_added)) {}
	};

	typedef vector<UndoItem> UndoStack;

	CString m_text;
	long m_maxLength;
	UndoStack m_undos, m_redos;
	bool m_undoOn;
	long m_undoLimit;
	bool m_modified;
	//Index of last item in m_undos which had zero change count.
	//It's used to set modify back to false when all edits are undone/redone.
	size_t m_unmodifyUndoSize;
	bool m_wordWrap;
	Lines m_lines;

	void UndoImpl(UndoStack& undos, UndoStack& redos, bool undo);
	long ValidateLength();
	void PushUndoItem(const UndoItem& item);
	void WrapLines(long pos, long& linesWrapped, long& linesUnwrapped, long& first, long& last, long* indicesToAdjust = NULL, long numIndicesToAdjust = 0, long numLinesToCheck = -1);
	long GetNumCharsThatFit(const CurDC& dc, LPCTSTR line, long lineLength, long width, long& fitWidth) const;

	void RewrapAllLines(long& selStart, long& selEnd);
	void UnwrapAllLines(long& selStart, long& selEnd);

	virtual LineDataPtr NewLineData() {return new LineData();}

	typedef CWindowImpl<EditCtrl> WndBase;
	CFont m_font;
	CHARRANGE m_sel;
	long m_firstVisibleLine;//for start of vertical scroll position
	long m_firstVisiblePixel;//for start of horizontal scroll position
	mutable long m_avgCharWidth, m_lineHeight, m_maxCharWidth;
	bool m_insert;
	bool m_hScroll;
	bool m_vScroll;
	vector<CodeLinePtr> m_visibleLines;
	bool m_readOnly;
	LanguagePtr m_lang;
	TokenColorsPtr m_tokenColors;
	long m_tabWidth;
	bool m_autoIndent;
	long m_gutterWidth;
	bool m_showBookmarks;
	bool m_showLineNumbers;
	CMenu m_contextMenu;
	bool m_greyedWhenReadOnly;
	bool m_useSystemSelectionColors;
	long m_widestLineIndex;
	long m_preChangeLineCount;
	long m_curOffset;
	//flag used to keep selecting using the left mouse button (i.e. OnMouseMove()) from
	//interfering with double clicking
	bool m_mouseDownFromSelectWord;
	bool m_mouseDownInControl;
	bool m_tabInsertsSpaces;
	bool m_showWhitespace;

	void ValidatePos(long& pos, long maxLen = -1) const;

	void SetCaretPos(const CPoint& pt) {
		if (HasFocus())
			VERIFY(::SetCaretPos(pt.x, pt.y));
	}

	int GetCharWidth(long pos) const;
	int GetCaretWidth() const;

	void CreateCaret();

	void ShowCaret() {
		if (HasFocus()) {
			WndBase::ShowCaret();
			SetCaretPos(PosFromChar(GetSelCaret()));
		}
	}

	void HideCaret() {
		if (HasFocus())
			VERIFY(WndBase::HideCaret());
	}

	LineDataPtr GetLineData(long line) const {
		ATLASSERT(0 <= line && line < m_lines.size());
		ATLASSERT(m_lines[line] != NULL);
		return m_lines[line];
	}

	COLORREF GetColor(TokenKinds kind) const {return m_tokenColors->Item(kind);}

	long GetLineWidth(long i, CurDC* dc = NULL) const;
	void InvalidateLineWidths();
	void ValidateWidestLine(long first = 0, long last = -1);
	long GetWidestLineWidth() const;
	long GetWidestLineWidthInefficiently() const;
	void ValidateCharIndexOfLine(long last) const;
	void ValidateLfLineIndex(long last) const;
	void ValidateCodeLine(long last) const;
	const CodeLinePtr& GetCodeLine(long i) const;
	bool IndentBlock(TCHAR indentChar, bool indent);
	void InvalidateTextRange(long start, long end);
	void InvalidateSel();
	void GetLineRange(long line, long& start, long& end) const;
	void GetHardLineRange(long line, long& start, long& end, long* endLine = NULL) const;
	void GetTextMetrics(TEXTMETRIC& tm) const;
	long GetAvgCharWidth() const;
	long GetMaxCharWidth() const;
	long GetCharHeight() const;
	void ScrollCaret(ScrollKinds sk);
	void UpdateHScroll();
	void UpdateVScroll();
	void GetLineSel(long line, long& start, long& end) const;
	void SelectWord(long pos);
	CString GetPreSpace(long line, long caretOffset) const;
	long GetNumSpacesBeforePos(long line, long caretOffset) const;
	void NotifyParent(WORD code) const;
	void CalcGutterWidth(bool* rewrapped = NULL);
	CRect GetGutterRect() const;
	void InvalidateGutter();
	void IncreaseSel(long amount);
	long FindCharIndexOfLine(long charIndex, long first, long last) const;
	CString GetTabSpaceString() const;
	void ReplaceTabsWithSpacesIfNecessary(CString& string) const;
	void WrapOrUnwrapAllLines(bool wrap);
	void CalcCachedTextMetricValues() const;
	void InvalidateCachedTextMetricValues() const;

	void SetStyle(DWORD style, bool val) {ModifyStyle(!val ? style : 0, val ? style : 0, SWP_FRAMECHANGED);}
	bool HasFocus() const {return GetFocus() == m_hWnd;}

	void DeleteSel() {
		ModifyText(GetSelStart(), GetSelCount(), EmptyStr);
	}

	void RemoveSel() {SetSelCaret(GetSelCaret());}

	long GetHScrollAmount() const {return GetAvgCharWidth();}

	void OnPreChange(long pos, long count, const CString& newText);
	void OnPostChange(long selStart, long selEnd, long first, long last, long charsAdded, long charsDeleted, long lfLinesAdded, long sizeChange, ScrollKinds scrollKind = skCaret);
	void OnLimitText() {MessageBeep(MB_OK);}

	long GetPrevWordPos() const {return ::GetPrevWordPos(GetText(), GetSelCaret(), true);}
	long GetNextWordPos() const {return ::GetNextWordPos(GetText(), GetSelCaret(), true);}

	template<class T>
	struct LineDataComparer {
		//returns < 0 if the value at line < value
		//returns 0 if the value at line == value
		//returns > 0 if the value at line > value
		virtual int Compare(long line, const T& value) const = 0;
	};

	template<class T>
	long BinaryFindLine(const LineDataComparer<T>& comparer, const T& value) const {
		long first = 0, last = GetLineCount() - 1;

		while (true) {
			// Is comparer never returning zero?
			// Is the line data cache wrong?
			ATLASSERT(first <= last);

			long mid = (first + last) / 2;
			int comp = comparer.Compare(mid, value);

			if (comp == 0)
				return mid;
			else if (comp < 0)
				first = mid + 1;
			else
				last = mid - 1;
		}
	}

	struct CharIndexOfLineComparer : LineDataComparer<long> {
		const EditCtrl& m_editCtrl;

		CharIndexOfLineComparer(const EditCtrl& editCtrl) : m_editCtrl(editCtrl) {}

		virtual int Compare(long line, const long& value) const {
			return m_editCtrl.CharIndexOfLine(line) - value;
		}
	};

	long FindCharIndexOfLine(long charIndex) const {
		return BinaryFindLine(CharIndexOfLineComparer(*this), charIndex);
	}

	struct LfLineIndexComparer : LineDataComparer<long> {
		const EditCtrl& m_editCtrl;

		LfLineIndexComparer(const EditCtrl& editCtrl) : m_editCtrl(editCtrl) {}

		virtual int Compare(long line, const long& value) const {
			return m_editCtrl.GetLfLineIndex(line) - value;
		}
	};

	long FindLfLineIndex(long lfLine) const {
		return BinaryFindLine(LfLineIndexComparer(*this), lfLine);
	}


public:
	EditCtrl();
	bool Create(HWND parent, RECT& rc, DWORD style, DWORD exStyle, UINT id = 0, LanguagePtr lang = NULL, TokenColorsPtr tokenColors = NULL);

	const CString& GetText() const {return m_text;}
	bool ModifyText(long pos, long count, const CString& newText, long selStart = -1, long selEnd = -1, ScrollKinds scrollKind = skCaret, bool modified = true);
	void DeleteChar(long pos);
	bool InsertChar(long pos, TCHAR ch);
	long GetTextLength() const {return GetText().GetLength();}
	long GetMaxLength() const {return m_maxLength == 0 ? std::numeric_limits<long>::max() : m_maxLength;}
	void SetMaxLength(long newMaxLength);

	void Undo() {UndoImpl(m_undos, m_redos, true);}
	void Redo() {UndoImpl(m_redos, m_undos, false);}
	void EmptyUndoBuffer() {m_undos.clear(); m_redos.clear(); m_unmodifyUndoSize = 0;}
	bool CanUndo() const {return !m_undos.empty() && IsWriteable();}
	bool CanRedo() const {return !m_redos.empty() && IsWriteable();}

	long GetUndoLimit() const {return m_undoLimit;}
	void SetUndoLimit(long newLimit);

	bool GetModified() const {return m_modified;}
	void SetModified(bool val);

	void SetText(const CString& newText, bool modified = true);
	CString GetTextRange(long start, long end) const {return GetText().Mid(start, end - start);}
	CString GetTextEx(NewLineType type) const;
	void ReplaceSel(const CString& newSel, bool selectReplacement = false);
	TCHAR GetChar(long index) const {return 0 <= index && index < GetTextLength() ? GetText().GetAt(index) : NULL;}

	const CString& GetLine(long i) const;
	CString GetHardLine(long i) const;
	long GetLineLength(long line) const;
	long CharIndexOfLine(long line) const;
	void LineFromChar(long& charIndex, long& lineNum, long pos) const;
	long LineFromChar(long pos) const;
	long GetLfLineIndex(long line) const;
	long GetLineIndexFromLfLineIndex(long lfLine) const;
	bool DoesLineWrap(long line) const;
	bool IsWrappedLine(long line) const {return line > 0 && DoesLineWrap(line - 1);}
	void SetFirstVisibleLine(long fvl);
	long GetFirstVisibleLine() const {return m_firstVisibleLine;}
	long GetLastVisibleLine(bool completelyVisible) const;
	long GetVisibleLineCount(bool completelyVisible) const {return GetLastVisibleLine(completelyVisible) - GetFirstVisibleLine() + 1;}
	long GetMaxVisibleLineCount(bool completelyVisible) const;
	void SetFirstVisiblePixel(long fvp);
	long GetFirstVisiblePixel() const {return m_firstVisiblePixel;}
	long GetLineCount() const {return m_lines.size();}
	long GetLfLineCount() const {return m_lines.empty() ? 0 : GetLfLineIndex(GetLineCount() - 1) + 1;}
	void GetLineAndColNum(long& lineNum, long& colNum) const;

	//anchor/active end sel methods
	void GetSel(long& anchor, long& end) const {anchor = m_sel.cpMin; end = m_sel.cpMax;}
	void SetSel(long anchor, long end, ScrollKinds sk = skCaret);
	void GetSelNoCRs(long& anchor, long& end) const;
	void SetSelNoCRs(long anchor, long end, ScrollKinds sk = skCaret);
	long GetSelAnchor() const {return m_sel.cpMin;}
	long GetSelActiveEnd() const {return m_sel.cpMax;}

	//range sel methods
	void GetSelRange(long& start, long& end) const;
	long GetSelStart() const {return Min(m_sel.cpMin, m_sel.cpMax);}
	long GetSelEnd() const {return Max(m_sel.cpMin, m_sel.cpMax);}

	//misc sel methods
	bool HasSel() const {return m_sel.cpMin != m_sel.cpMax;}
	long GetSelCount() const {return abs(m_sel.cpMin - m_sel.cpMax);}
	void SetSelCaret(long pos, bool scrollCaret = true) {SetSel(pos, pos, scrollCaret ? skCaret : skNone);}
	long GetSelCaret() const {return GetSelActiveEnd();}
	void SelectAll() {SetSel(0, GetTextLength());}
	CString GetSelText() const;

	long GetCaretLine() const;
	void GotoLine(long line);
	void GotoLfLine(long line);
	void GotoLineAndColNum(long realLine, long realCol = 0);
	void RecolorAll();
	void GetWordAt(long pos, long& start, long& end);

	CPoint PosFromChar(long pos) const;
	long CharFromPos(const CPoint& pt) const;

	long GetClientWidth() const;
	long GetClientHeight() const;

	long GetLineHeight() const;

	HFONT GetFont() const {return m_font;}
	void SetFont(const CFont& font);
	const CFont& GetFontObject() const {return m_font;}

	bool GetHScroll() const {return m_hScroll;}
	void SetHScroll(bool val);

	bool GetVScroll() const {return m_vScroll;}
	void SetVScroll(bool val);

	bool CanCut() const;
	bool CanCopy() const;
	bool CanPaste() const;
	void Cut();
	void Copy() const;
	void Paste();

	void ToggleBookmark();
	void NextBookmark(bool next);
	void ClearAllBookmarks();

	bool HasBookmark(long line) const {return GetLineData(line)->m_hasBookmark;}
	void SetBookmark(long line, bool val) {GetLineData(line)->m_hasBookmark = val;}
	void AddBookmarks(const vector<long>& bookmarks);

	bool GetInsert() const {return m_insert;}
	void SetInsert(bool b);

	bool GetReadOnly() const {return m_readOnly;}
	void SetReadOnly(bool b);

	bool IsWriteable() const {return !GetReadOnly() && IsWindowEnabled();}

	COLORREF GetTextColor() const {return GetColor(tkPlainText);}
	COLORREF GetBackColor() const {return !IsWindowEnabled() || (GetReadOnly() && GetShowGreyedWhenReadOnly()) ? GetSysColor(COLOR_3DFACE) : GetColor(tkBackground);}
	COLORREF GetSelForeColor() const;
	COLORREF GetSelBackColor() const;

	long GetTabWidth() const {return m_tabWidth;}
	void SetTabWidth(long newTabWidth);

	long GetTabWidthInPixels() const {return m_tabWidth * GetAvgCharWidth();}

	bool GetAutoIndent() const {return m_autoIndent;}
	void SetAutoIndent(bool b) {m_autoIndent = b;}

	bool GetShowBookmarks() const {return m_showBookmarks;}
	void SetShowBookmarks(bool val);

	bool GetShowLineNumbers() const {return m_showLineNumbers;}
	void SetShowLineNumbers(bool val);

	bool GetShowGreyedWhenReadOnly() const {return m_greyedWhenReadOnly;}
	void SetShowGreyedWhenReadOnly(bool val);

	bool GetUseSystemSelectionColors() const {return m_useSystemSelectionColors;}
	void SetUseSystemSelectionColors(bool val);

	const LanguagePtr GetLanguage() const {return m_lang;}
	void SetLanguage(LanguagePtr lang);

	const TokenColorsPtr GetTokenColors() const {return m_tokenColors;}
	void SetTokenColors(TokenColorsPtr clrs);

	HMENU GetContextMenu() const {return m_contextMenu.m_hMenu;}
	void SetContextMenu(HMENU menu);

	bool GetTabInsertsSpaces() const {return m_tabInsertsSpaces;}
	void SetTabInsertsSpaces(bool val) {m_tabInsertsSpaces = val;}

	bool GetShowWhitespace() const {return m_showWhitespace;}
	void SetShowWhitespace(bool val) {m_showWhitespace = val; Invalidate();}

	bool GetWordWrap() const {return m_wordWrap;}
	void SetWordWrap(bool val);

	void RewrapAllLinesIfNecessary() {
		if (GetWordWrap())
			WrapOrUnwrapAllLines(true);
	}

	void ForceShowCaret() {
		// For some reason, hiding and showing the caret isn't sufficient
		// to force the caret to show.
		DestroyCaret();
		CreateCaret();
	}

DECLARE_WND_CLASS(_T("EditCtrl"));

BEGIN_MSG_MAP(EditCtrl)
	MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
	MESSAGE_HANDLER(WM_CHAR, OnChar)
	MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
	MESSAGE_HANDLER(WM_SETTEXT, OnSetText)
	MESSAGE_HANDLER(WM_GETTEXT, OnGetText)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
	MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
	MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
	MESSAGE_HANDLER(WM_CUT, OnCut)
	MESSAGE_HANDLER(WM_COPY, OnCopy)
	MESSAGE_HANDLER(WM_PASTE, OnPaste)
	MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
	MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
	MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
	MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
	MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkGnd)
	MESSAGE_HANDLER(WM_ENABLE, OnEnable)
	MESSAGE_HANDLER(WM_NCCALCSIZE, OnNcCalcSize)
	MESSAGE_HANDLER(WM_NCPAINT, OnNcPaint)
	MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_TIMER, OnTimer)
#if 0
	MESSAGE_HANDLER(WM_, On)
#endif

	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()

	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSetText(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnGetText(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCut(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCopy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaste(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEraseBkGnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnEnable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNcCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

#if 0
	LRESULT On(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
#endif
};

#endif