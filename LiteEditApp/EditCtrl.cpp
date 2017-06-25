#include "StdAfx.h"
#include "EditCtrl.h"
#include <algorithm>
#include <locale>
#include <limits>
#include "Windowsx.h"

const UINT ID_SCROLLSELECT = 1;
const long BOOKMARK_WIDTH = 20;
const long SPACE_BETWEEN_GUTTER_AND_CLIENT_AREA = 1;//space between client area and where gutter appears to start
const COLORREF COLOR_BOOKMARK = RGB(0x0, 0xFF, 0xFF);
const COLORREF COLOR_LINENUMBER = RGB(0x0, 0x60, 0x0);
const COLORREF COLOR_GUTTER = RGB(230, 230, 230);

// class to simplify passing selection indices as an array
class SelArrayConv {
private:
	long m_array[2];
	long& index1;
	long& index2;

public:
	SelArrayConv(long& i1, long& i2) : index1(i1), index2(i2) {
		m_array[0] = index1;
		m_array[1] = index2;
	}

	~SelArrayConv() {
		index1 = m_array[0];
		index2 = m_array[1];
	}

	operator long*() {
		return m_array;
	}
};

int EditCtrl::CurDC::GetLineWidth(LPCTSTR line, long length) const {
	ATLASSERT(0 <= length && length <= (long)lstrlen(line));
	int tabLength = m_ctrl.GetTabWidthInPixels();
	int width = 0;
	long startChar = 0;
	const long MaxChars = GetMaxCharsForTabbedTextFunctions();

	//must use a loop because of GetMaxCharsForTabbedTextFunctions() restriction
	while (startChar < length) {
		long numChars = Min(length - startChar, MaxChars);
		DWORD sz = ::GetTabbedTextExtent(*this, line + startChar, numChars, 1, &tabLength);
		ATLASSERT(sz != 0 || length == 0);//call failed if returned 0 for non-empty string
		startChar += numChars;
		width += LOWORD(sz);
	}

	return width;
}

//draws text with tabs correctly and honors the show white space option.
//returns total width of drawn text.
int EditCtrl::CurDC::DrawText(int x, int y, const CString& text) {
	int tabLength = m_ctrl.GetTabWidthInPixels();
	int originalX = x;

	if (m_ctrl.GetShowWhitespace()) {
		//when showing whitespace, I draw one character of the text
		//at a time and if I draw a space or tab, I draw a dot or line,
		//respectively, over the whitespace

		for (long i = 0; i < text.GetLength(); ++i) {
			int width = TabbedTextOut(x, y, (LPCTSTR)text + i, 1, 1, &tabLength, 0).cx;

			TCHAR ch = text.GetAt(i);
			if (ch == ' ')
				SetPixel(x + width / 2, y + m_ctrl.GetLineHeight() / 2, GetTextColor());
			else if (ch == '\t') {
				//draw line
				CPoint pts[2];
				pts[0].x = x + 2;
				pts[0].y = y + m_ctrl.GetLineHeight() / 2;
				pts[1].x = x + width - 2;
				pts[1].y = pts[0].y;
				Polyline(pts, 2);
			}

			x += width;
		}
	} else {
		long startChar = 0;
		const long MaxChars = GetMaxCharsForTabbedTextFunctions();

		//must use a loop because of GetMaxCharsForTabbedTextFunctions() restriction
		while (startChar < text.GetLength()) {
			long numChars = Min(text.GetLength() - startChar, MaxChars);
			//TabbedTextOut sometimes draws an extra background pixel to the left or right
			//when using a non-monospace font or on windows 7. DrawText does not have this
			//apparent bug, but I must use TabbedTextOut for its tabbing capability.
			//I have to work around this in InvalidateTextRange.
			DWORD sz = ::TabbedTextOut(*this, x, y, (LPCTSTR)text + startChar, numChars, 1, &tabLength, 0);
			ATLASSERT(sz != 0 || text.IsEmpty());//call failed if returned 0 for non-empty string
			startChar += numChars;
			x += LOWORD(sz);
		}
	}

	return x - originalX;//total width of drawn text
}

void EditCtrl::UndoImpl(UndoStack& undos, UndoStack& redos, bool undo) {
	if ((undo && CanUndo()) || (!undo && CanRedo())) {
		AutoBool ab(m_undoOn, false);

		UndoItem item = undos.back();//make a copy so it can be used after erased from undos

		redos.push_back(item);
		undos.erase(&undos.back());

		const CString& add = undo ? item.deleted : item.added;
		const CString& del = undo ? item.added : item.deleted;

		//m_unmodifyUndoSize stores the size of m_undos when modified is set to false.
		//So we set modified to false iff m_undos.size() = m_unmodifyUndoSize.
		//See SetModified().
		bool modify = m_undos.size() != m_unmodifyUndoSize;

		long pos = item.pos;

		//undo items always store LF positions so the indices don't
		//get invalidated when text is unwrapped or rewrapped
		if (GetWordWrap())
			pos = ConvertLFPosToCRLFPos(m_text, pos);

		long delEnd = pos + del.GetLength();

		if (GetWordWrap())
			delEnd = ConvertLFPosToCRLFPos(m_text, delEnd, pos);

		ModifyText(pos, delEnd - pos, add, -1, -1, skCaret, modify);

		m_undoOn = true;
	}
}

long EditCtrl::ValidateLength() {
	long cutAmount = GetTextLength() - GetMaxLength();

	if (cutAmount > 0) {
		VERIFY(ModifyText(GetTextLength() - cutAmount, cutAmount, EmptyStr));
		return cutAmount;
	} else
		return 0;
}

void EditCtrl::PushUndoItem(const UndoItem& item) {
	ATLASSERT(m_undoOn);

	if (m_undoLimit == 0) {
		m_undos.clear();
		m_redos.clear();
	} else {
		//make sure m_undos doesn't exceed m_undoLimit
		if (m_undoLimit != -1 && m_undos.size() >= m_undoLimit) {
			long numItemsToRemove = m_undos.size() - m_undoLimit + 1;//+ 1 to make room for item to be added
			m_undos.erase(m_undos.begin(), m_undos.begin() + numItemsToRemove);

			// Must adjust this index since the undo items are getting shifted lower.
			// It's ok if this becomes negative because there shouldn't be a way to
			// get back to the unmodified state if the unmodifying undo item gets removed.
			m_unmodifyUndoSize -= numItemsToRemove;
		}

		m_undos.push_back(item);
		m_redos.clear();
	}
}

static long GetLineStartPos(const CString& text, long pos) {
	if (pos > 0)
		return ReverseFindOneOf(text, "\r\n", pos - 1) + 1;
	else
		return 0;
}

static bool EatNewLines(const CString& text, long& pos, long& curLine) {
	while (pos < text.GetLength() && text.GetAt(pos) == '\n') {
		++pos;
		++curLine;
	}

	return pos >= text.GetLength();
}

//pos is where to begin trying to wrap. This function inserts or
//deleted soft line breaks to wrap text so that it fits within
//the control.
//linesWrapped is the number of soft line breaks inserted, and
//linesUnwrapped is the number of soft line breaks deleted.
//first and last are in/out parameter for the first and last line
//modified. On input, first should be the line at pos.
//Indices in indicesToAdjust are assumed to be in CRLF indices with
//respect to the existing text before wrapping.
//Note: Currently word wrap is turned off when opening binary files for performance.
//Consider removing that if word wrap is optimized.
void EditCtrl::WrapLines(long pos, long& linesWrapped, long& linesUnwrapped, long& first, long& last, long* indicesToAdjust/* = NULL*/, long numIndicesToAdjust/* = 0*/, long numLinesToCheck/* = -1*/) {
	ATLASSERT(GetWordWrap());

	linesWrapped = linesUnwrapped = 0;

	const long controlWidth = GetClientWidth();

	long curLine = first;

	if (numLinesToCheck < 0)
		//check all lines
		numLinesToCheck = (long)m_lines.size();

	long lineStartPos = GetLineStartPos(m_text, pos);

	//start checking on the previous line so I will unwrap this
	//line if necessary
	if (lineStartPos > 0) {
		lineStartPos = GetLineStartPos(m_text, lineStartPos - 1);
		++numLinesToCheck;
		--curLine;
		ATLASSERT(curLine >= 0);//pos and first not consistent?
	}

	if (EatNewLines(m_text, lineStartPos, curLine))
		return;

	CurDC dc(*this);

	long lastWrapped = -1;

	long lineEndPos = FindOneOf(m_text, "\r\n", lineStartPos);
	long nextLineStartPos = 0, nextLineEndPos = 0;
	if (lineEndPos < 0)
		lineEndPos = m_text.GetLength();

	long fitWidth = 0;

	while (numLinesToCheck > 0) {
		--numLinesToCheck;

		const long lineLength = lineEndPos - lineStartPos;
		long numCharsThatFit = GetNumCharsThatFit(dc, (LPCTSTR)m_text + lineStartPos, lineLength, controlWidth, fitWidth);

		if (0 < numCharsThatFit && numCharsThatFit < lineLength) {
			// optimization: Insert allocates a new buffer one char longer than the
			// old buffer if the buffer isn't long enough. I allocate an extra 1024
			// to prevent allocation from hppening on every insert.
			if (m_text.GetLength() == m_text.GetAllocLength())
				m_text.GetBuffer(m_text.GetAllocLength() + 1024);

			// unimplemented optimizatio'n:
			// In one bad but still typical case for word wrap, around 80%-90% of the
			// time spent in word wrap was in m_text.Insert.
			// One way to optimize it is keep track of all the '\r' to insert and insert
			// them all once at the end. We should only have to allocate the buffer once
			// an d would no longer need the optimization above.
			long wrappedLineEndPos = lineStartPos + numCharsThatFit;
			m_text.Insert(wrappedLineEndPos, '\r');

			//insert after curLine if possible so bookmarks will
			//stay on the first line of a wrapped line
			long insertOffset = Min(curLine + 1L, (long)m_lines.size());
			m_lines.insert(m_lines.begin() + insertOffset, NewLineData());

			++linesWrapped;

			lastWrapped = curLine + 1L;

			//must check one more line now
			++numLinesToCheck;

			AdjustIndices(wrappedLineEndPos, indicesToAdjust, numIndicesToAdjust, 1);

			nextLineStartPos = wrappedLineEndPos + 1;
		} else
			nextLineStartPos = lineEndPos + 1;

		if (nextLineStartPos >= m_text.GetLength())
			break;

		//calculate the end of the next line by finding the first next new line character
		//after nextLineStartPos
		nextLineEndPos = FindOneOf(m_text, "\r\n", nextLineStartPos);
		if (nextLineEndPos < 0)
			nextLineEndPos = m_text.GetLength();

		//unwrap line if possible
		if (numCharsThatFit == lineLength && lineEndPos < m_text.GetLength() && m_text.GetAt(lineEndPos) == '\r') {
			//check if the character before the \r is no longer a space, e.g. the space
			//got deleted
			bool unwrap = lineEndPos > 0 && !_istspace(m_text.GetAt(lineEndPos-1));

			//check if anything from the the next line would fit on this line
			if (!unwrap) {
				const long nextLineLength = nextLineEndPos - nextLineStartPos;
				const long roomLeft = controlWidth - fitWidth;
				long numCharsFromNextLineThatFit = GetNumCharsThatFit(dc, (LPCTSTR)m_text + nextLineStartPos, nextLineLength, roomLeft, fitWidth);
				unwrap = numCharsFromNextLineThatFit > 0 && fitWidth <= roomLeft;
			}

			if (unwrap) {
				m_text.Delete(lineEndPos);
				m_lines.erase(m_lines.begin() + curLine + 1);
				++linesUnwrapped;

				//this can happen if I unwrap the first line
				if (curLine < first)
					first = curLine;

				lastWrapped = curLine;

				AdjustIndices(lineEndPos, indicesToAdjust, numIndicesToAdjust, -1);

				ATLASSERT(lineEndPos <= m_text.GetLength());

				if (lineEndPos == m_text.GetLength())
					nextLineEndPos = lineEndPos;
				else {
					nextLineEndPos = FindOneOf(m_text, "\r\n", lineEndPos);

					if (nextLineEndPos < 0)
						nextLineEndPos = m_text.GetLength();
				}

				//the line start here again since the new line was deleted
				nextLineStartPos = lineStartPos;
				--curLine;

				//since we unwrapped the next line, we must always check if it needs to be
				//rewrapped
				if (numLinesToCheck == 0)
					++numLinesToCheck;
			}
		}

		lineStartPos = nextLineStartPos;
		lineEndPos = nextLineEndPos;
		++curLine;
	}

	//because lines were unwrapped, the value that was passed in
	//for last could now be greater than the line count
	last = Min(last, (long)m_lines.size() - 1L);

	ATLASSERT(lastWrapped < (long)m_lines.size());

	last = Max(last, lastWrapped);

	//I have to invalidate all lines from first to last if wrapping
	//happened because insert/removing the CRs could potentially
	//invalidate all lines indexes between first and last
	if (lastWrapped >= 0)
		for (long i = first; i <= last; ++i)
			m_lines[i]->Invalidate();

	//verify I added or removed enough lines from m_lines to be consistent with
	//changes
	ATLASSERT(GetNumNewLines(m_text) + 1L == (long)m_lines.size());
}

long EditCtrl::GetNumCharsThatFit(const CurDC& dc, LPCTSTR line, long lineLength, long width, long& fitWidth) const {
	// use the average char width to make a good initial guess of how many chars that fit
	long result = Min(Max(width / GetAvgCharWidth(), 0L), lineLength);

	// I need to start where a word can wrap. It should be faster to start with the next
	// work break than the previous one for the common case where the whole line fits.
	result = GetNextWordBreakPos(line, lineLength, result);

	fitWidth = dc.GetLineWidth(line, result);
	long lastResult = -1, lastFitWidth = -1;

	// If result isn't long enough, start looking to the right, word by word.
	// If result is too long, start looking to the left, word by word.
	// If result fits exactly, we're already done.
	if (lineLength > 0)
		if (fitWidth < width) { // if result isn't long enough
			long nextFitWidth, nextResult;

			while (true) {
				// if we're already using the whole line and it fits,
				// or if result fits exactly
				if (result == lineLength || fitWidth == width)
					break;

				nextResult = GetNextWordBreakPos(line, lineLength, result);
				nextFitWidth = dc.GetLineWidth(line, nextResult);

				// if next word wouldn't fit
				if (nextFitWidth > width)
					break;

				result = nextResult;
				fitWidth = nextFitWidth;
			}
		} else if (fitWidth > width) { // if result is too long
			long nextFitWidth, nextResult;

			while (true) {
				nextResult = GetPrevWordBreakPos(line, result);
				nextFitWidth = dc.GetLineWidth(line, nextResult);

				// if nothing will fit, stop.
				// if no word can fit, we let the first word extend past the edge of
				// the control without wrapping (many other text editors break the
				// line on a non-breaking character).
				if (nextResult == 0)
					break;

				result = nextResult;
				fitWidth = nextFitWidth;

				// if found left most word that fits
				if (fitWidth <= width)
					break;

			}
		}

	return result;
}

void EditCtrl::RewrapAllLines(long& selStart, long& selEnd) {
	WaitCursor wc;

	OnPreChange(0, 0, EmptyStr);

	long first = 0, last = 0;
	long linesWrapped = 0, linesUnwrapped = 0;

	WrapLines(0, linesWrapped, linesUnwrapped, first, last, SelArrayConv(selStart, selEnd), 2);

	long sizeChange = linesWrapped - linesUnwrapped;
	OnPostChange(selStart, selEnd, first, last, linesWrapped, linesUnwrapped, 0, sizeChange);
}

void EditCtrl::UnwrapAllLines(long& selStart, long& selEnd) {
	WaitCursor wc;

	OnPreChange(0, 0, EmptyStr);

	vector<long> indices;
	indices.push_back(selStart);
	indices.push_back(selEnd);

	//Save bookmark line indices and clear out existing bookmarks.
	//This method will add them back after it adjusts the line indices.
	for (size_t i = 0; i < m_lines.size(); ++i)
		if (HasBookmark(i)) {
			indices.push_back(CharIndexOfLine(i));
			SetBookmark(i, false);
		}

	long linesRemoved = RemoveCR(m_text, indices.begin(), indices.size());

	selStart = indices[0];
	selEnd = indices[1];

	m_lines.erase(m_lines.end() - linesRemoved, m_lines.end());

	size_t size = m_lines.size();
	ATLASSERT(size > 0);

	for (i = 0; i < size; ++i)
		m_lines[i]->Invalidate();

	//verify I added or removed enough lines from m_lines to be consistent with
	//changes
	ATLASSERT(GetNumNewLines(m_text) + 1L == (long)m_lines.size());

	//get the bookmark lines from the bookmark line indices
	vector<long> bookmarks;
	for (i = 2; i < indices.size(); ++i)
		bookmarks.push_back(LineFromChar(indices[i]));

	AddBookmarks(bookmarks);

	long sizeChange = -linesRemoved;
	OnPostChange(selStart, selEnd, 0, (long)size - 1L, 0, linesRemoved, 0, sizeChange);
}

//IMPORTANT NOTE: ModifyText() is the only function where m_lines
//can be inconsistent with m_text. Do not call a function that
//assumes m_lines and m_text are consistent when they may not be
//inside ModifyText(). Many functions, such as LineFromChar(),
//assume this.
//
//Other notes:
//1. If you pass in values for selStart and selEnd, they are in wrapped indices
//assuming the replacement of count characters with _newText has been done but
//wrapping has not been done to recalculate the CRs. In other words, selStart and
//selEnd are in CRLF indices using the CRs and LFs of the existing text.
//ModifyText adjusts selStart and selEnd for any wrapping ModifyText does.
//2. If you pass skNone to scrollKind, you should set the first visible line and first
// visible pixel to make sure they are valid.
bool EditCtrl::ModifyText(long pos, long count, const CString& _newText, long selStart/* = -1*/, long selEnd/* = -1*/, ScrollKinds scrollKind/* = skCaret*/, bool modified/* = true*/) {
	ATLASSERT(0 <= pos && pos <= GetTextLength());
	ATLASSERT(count >= 0);

	if (count <= 0 && (_newText.IsEmpty() || GetTextLength() == GetMaxLength())) {
		if (!_newText.IsEmpty())
			OnLimitText();

		return false;
	}

	//make sure newText won't make text longer than MaxLength()
	long newLen = GetTextLength() + _newText.GetLength() - count;
	CString newText = newLen > GetMaxLength() ? _newText.Left(GetMaxLength() - GetTextLength() + count) : _newText;

	//show a wait cursor for a change that may not be fast
	bool bigChange = count + newText.GetLength() >= 20000;
	WaitCursor wc(bigChange);

	// I assume newText has no wrapped new lines
	ASSERT(GetNumOccurrences(newText, '\r') == 0);

	OnPreChange(pos, count, newText);

	long lfLinesAdded = 0;
	long crsInDeletedText = 0;

	long numCRs, numLFs;
	GetNumNewLines(m_text, numCRs, numLFs, 0, pos);

	CString deletedText = m_text.Mid(pos, count);

	if (GetWordWrap())
		crsInDeletedText = RemoveCR(deletedText);

	bool modifiesText = newText != deletedText;

	if (m_undoOn && modifiesText) {
		//so unwrapping or rewrapping won't invalidate the text and
		//indices in undo items, undo items always store text and
		//indices as if text was not wrapped

		PushUndoItem(UndoItem(pos - numCRs, deletedText, newText));
	}

	long first = numCRs + numLFs;

	long numLinesDeleted = GetNumNewLines(m_text, pos, pos + count);
	m_text.Delete(pos, count);

	long numLinesAdded = GetNumNewLines(newText);

	// optimization: m_text.Insert only allocates just enough memory for new text.
	// It makes sense to allocate more than that to avoid unnecessary reallocations.
	long newTextLen = m_text.GetLength() + newText.GetLength();
	if (newTextLen > m_text.GetAllocLength())
		m_text.GetBuffer(newTextLen + 1024);

	m_text.Insert(pos, newText);

	// newText cannot contain any CRs
	lfLinesAdded = numLinesAdded - (numLinesDeleted - crsInDeletedText);

	long last = first;

	if (numLinesDeleted > 0)
		m_lines.erase(&m_lines[first], &m_lines[first+numLinesDeleted]);

	m_lines[first]->Invalidate();

	if (numLinesAdded > 0) {
		//Reserve enough plus some grow room to decrease the
		//number of times the m_lines vector must be internally
		//reallocated. Profiling has shown this to be an important
		//optimization.
		if (m_lines.size() + numLinesAdded > m_lines.capacity())
			m_lines.reserve(2 * (m_lines.size() + numLinesAdded));

		last += numLinesAdded;

		for (long i = first + 1; i <= last; ++i)
			m_lines.insert(&m_lines[i], NewLineData());
	}

	//verify I added or removed enough lines from m_lines to be consistent with
	//changes
	ATLASSERT(GetNumNewLines(m_text) + 1L == (long)m_lines.size());

	//put selection at end of modification if no selection specified
	if (selStart < 0)
		selStart = pos + newText.GetLength();
	if (selEnd < 0)
		selEnd = selStart;

	long numLinesWrapped = 0, numLinesUnwrapped = 0;

	if (GetWordWrap()) {
		long numLinesToCheck = numLinesAdded + 1;//+ 1 to check this line also
		long lastModified = last;

		WrapLines(pos, numLinesWrapped, numLinesUnwrapped, first, lastModified, SelArrayConv(selStart, selEnd), 2, numLinesToCheck);

		//last is the last line modified which is the max of the last line added and the
		//last line modified by word wrap
		long lastLineAdded = last + numLinesWrapped - numLinesUnwrapped;
		last = Max(lastModified, lastLineAdded);
	}

	//Don't mark as modified if changes weren't actually made.
	//I can mark as unmodified if changes were made or not.
	if (modifiesText || !modified)
		SetModified(modified);

	long charsAdded = newText.GetLength() + numLinesWrapped;
	long charsDeleted = count + numLinesUnwrapped;
	long sizeChange = charsAdded - charsDeleted;

	OnPostChange(selStart, selEnd, first, last, charsAdded, charsDeleted, lfLinesAdded, sizeChange, scrollKind);
	return true;
}

void EditCtrl::DeleteChar(long pos) {
	ModifyText(pos, 1, CString());
}

bool EditCtrl::InsertChar(long pos, TCHAR ch) {
	return ModifyText(pos, 0, CharToStr(ch));
}

void EditCtrl::SetMaxLength(long newMaxLength) {
	if (newMaxLength < 0)
		newMaxLength = 0;

	m_maxLength = newMaxLength;
	ValidateLength();
}

//m_undos will not be modified, even if newLimit is < m_undoLimit
void EditCtrl::SetUndoLimit(long newLimit) {
	ATLASSERT(newLimit >= -1);
	m_undoLimit = newLimit;
}

void EditCtrl::SetModified(bool val) {
	m_modified = val;

	if (!m_modified && m_undoOn)
		m_unmodifyUndoSize = m_undos.size();
}

void EditCtrl::ValidatePos(long& pos, long maxLen/* = -1*/) const {
	pos = Min(pos, maxLen == -1 ? GetTextLength() : maxLen);
	pos = Max(pos, 0L);
}

int EditCtrl::GetCharWidth(long pos) const {
	int width = 1;
	long offset, line;
	CurDC dc(*this);
	LineFromChar(offset, line, pos);

	if (pos - offset < GetLineLength(line)) {
		TCHAR ch = GetText().GetAt(pos);
		//I can't use GetTextExtent() to calculate width of tab char since tab can
		//vary in width
		if (ch == '\t')
			width =
				dc.GetLineWidth(line, pos - offset + 1) -
				dc.GetLineWidth(line, pos - offset);
		else
			width = dc.GetTextExtent(CString(ch)).cx;
	} else
		width = GetAvgCharWidth();

	return width;
}

int EditCtrl::GetCaretWidth() const {
	return m_insert ? 2 : GetCharWidth(GetSelCaret());
}

void EditCtrl::CreateCaret() {
	if (HasFocus()) {
		VERIFY(CreateSolidCaret(GetCaretWidth(), GetCharHeight()));
		ShowCaret();
	}
}

long EditCtrl::GetLineWidth(long i, CurDC* dc/* = NULL*/) const {
	LineDataPtr lineData = GetLineData(i);

	if (lineData->m_width == -1)
		lineData->m_width = dc == NULL ?
			CurDC(*this).GetLineWidth(i, GetLineLength(i)) :
			dc->GetLineWidth(i, GetLineLength(i));

	return m_lines[i]->m_width;
}

void EditCtrl::InvalidateLineWidths() {
	for (long i = 0; i < GetLineCount(); ++i)
		GetLineData(i)->m_width = -1;

	ValidateWidestLine();
}

void EditCtrl::ValidateWidestLine(long first/* = 0*/, long last/* = -1*/) {
	CurDC dc(*this);

	if (last < 0)
		last = GetLineCount() - 1;

	for (long i = first; i <= last; ++i)
		if (GetLineWidth(i, &dc) > GetLineWidth(m_widestLineIndex, &dc))
			m_widestLineIndex = i;
}

long EditCtrl::GetWidestLineWidth() const {
	return GetLineWidth(m_widestLineIndex);
}

//this is used in the debug build to make sure m_widestLineIndex is
//validated correctly
long EditCtrl::GetWidestLineWidthInefficiently() const {
	long width = 0;
	for (long i = 0; i < GetLineCount(); ++i)
		width = Max(width, GetLineWidth(i));

	return width;
}

// validates m_charIndexOfLine in LineData from the first invalid line to the specified line
void EditCtrl::ValidateCharIndexOfLine(long last) const {
	long first = last + 1;

	// find first invalid line
	while (first > 0 && GetLineData(first - 1)->m_charIndexOfLine < 0)
		--first;

	long lastCharIndex = first == 0 ? 0 : CharIndexOfLine(first - 1);

	// validate all lines from first to last
	for (long i = first; i <= last; ++i) {
		long& charIndex = GetLineData(i)->m_charIndexOfLine;

		if (i == 0)
			charIndex = 0;
		else
			charIndex = lastCharIndex + GetLineLength(i-1) + 1;//+1 for new line

		lastCharIndex = charIndex;
	}
}

// validates all m_lfLineIndex in LineData from the first invalid line to the specified line
void EditCtrl::ValidateLfLineIndex(long last) const {
	long first = last + 1;

	// find first invalid line
	while (first > 0 && GetLineData(first - 1)->m_lfLineIndex < 0)
		--first;

	long curLfLineIndex = first == 0 ? 0 : GetLfLineIndex(first - 1);

	// validate all lines from first to last
	for (long i = first; i <= last; ++i) {
		long& lfLineIndex = GetLineData(i)->m_lfLineIndex;

		if (i > 0 && !DoesLineWrap(i - 1))
			++curLfLineIndex;

		lfLineIndex = curLfLineIndex;
	}
}

// validates code lines in LineData from the first invalid line to the specified line
void EditCtrl::ValidateCodeLine(long last) const {
	long first = last + 1;

	// find first invalid line
	while (first > 0 && !GetLineData(first - 1)->m_codeLineValid)
		--first;

	static const Context defaultContext;
	const Context* context = first == 0 ? &defaultContext : &GetCodeLine(first - 1)->GetEndingContext();

	// validate all lines from first to last
	for (long i = first; i <= last; ++i) {
		LineDataPtr lineData = GetLineData(i);
		lineData->m_codeLineValid = true;
		lineData->m_codeLine = m_lang->NewCodeLine(GetLine(i), *context, DoesLineWrap(i));
		context = &lineData->m_codeLine->GetEndingContext();
	}
}

const CodeLinePtr& EditCtrl::GetCodeLine(long i) const {
	LineDataPtr lineData = GetLineData(i);

	if (!lineData->m_codeLineValid)
		ValidateCodeLine(i);

	return m_lines[i]->m_codeLine;
}

bool EditCtrl::IndentBlock(TCHAR indentChar, bool indent) {
	long selStart, selEnd;
	GetSelRange(selStart, selEnd);
	long startPos, endPos, startLine, endLine;
	LineFromChar(startPos, startLine, selStart);
	LineFromChar(endPos, endLine, selEnd);
	if (startLine == endLine)
		return false;

	//if any part of the line is selected, the line will be
	//indented, so select the whole thing
	if (selEnd > endPos)
		endPos += GetLineLength(endLine);

	--endPos;//don't get the final new line

	CString block = GetTextRange(startPos, endPos);

	//ModifyText() expects the newText to not have CRs and
	//for the selection passed in to not account for wrapped
	//after the starting point of modification.
	if (GetWordWrap())
		RemoveCR(block);

	bool modified = true;
	CString indentStr = CString('\n') + indentChar;
	if (indent) {
		block.Replace(TEXT("\n"), indentStr);
		block.Insert(0, indentChar);
	} else {
		modified = block.Replace(indentStr, TEXT("\n")) > 0;
		if (block.GetAt(0) == indentChar) {
			block.Delete(0);
			modified = true;
		}
	}

	if (modified) {
		ReplaceTabsWithSpacesIfNecessary(block);
		// + 1 since the last character on the line isn't included in block
		long selEnd = startPos + block.GetLength() + 1;
		ModifyText(startPos, endPos - startPos, block, startPos, selEnd);
	}

	return true;
}

void EditCtrl::InvalidateTextRange(long start, long end) {
	if (start == end)
		return;

	long first, last, startOffset, endOffset;

	LineFromChar(startOffset, first, start);
	LineFromChar(endOffset, last, end);

	//only bother to invalidate the visible lines
	long firstVisible = Max(first, GetFirstVisibleLine());
	long lastVisible = Min(last, GetLastVisibleLine(false));

	CRect rc(0, (firstVisible - GetFirstVisibleLine()) * GetLineHeight(), 0, 0);
	rc.bottom = rc.top + GetLineHeight();
	long fvp = GetFirstVisiblePixel();

	CurDC dc(*this);

	for (long i = firstVisible; i <= lastVisible; ++i) {
		if (i == first && i == last) {
			rc.left = dc.GetLineWidth(i, start - startOffset) - fvp;
			rc.right = dc.GetLineWidth(i, end - endOffset) - fvp;
		} else if (i == first) {
			rc.left = dc.GetLineWidth(i, start - startOffset) - fvp;
			rc.right = GetLineWidth(i, &dc) - fvp;
		} else if (i == last) {
			rc.left = 0;
			rc.right = dc.GetLineWidth(i, end - endOffset) - fvp;
		} else {
			rc.left = 0;
			rc.right = GetLineWidth(i, &dc) - fvp;
		}

		//This is a work around for the behavior of TabbedTextOut to sometimes draw
		//an extra background pixel to the left or right when using a non-monospace
		//font or on windows 7. I verified TabbedTextOut does this and DrawText does
		//not by doing a print screen, pasting that into paint brush and zooming in.
		//Without this, shift or mouse select/deselect can sometimes leave 1 pixel
		//not invalidated.
		++rc.right;
		--rc.left;

		InvalidateRect(rc, TRUE);
		rc.OffsetRect(0, GetLineHeight());
	}
}

void EditCtrl::InvalidateSel() {
	long start, end;
	GetSelRange(start, end);
	InvalidateTextRange(start, end);
}

void EditCtrl::GetLineRange(long line, long& start, long& end) const {
	start = CharIndexOfLine(line);
	end = start + GetLineLength(line);
}

// Gets line without breaking on wrapped lines.
// endLine contains the line index of the last line in the range (i.e. the line that
// contains the end character).
void EditCtrl::GetHardLineRange(long line, long& start, long& end, long* endLine/* = NULL*/) const {
	long startLine = line;

	// get the first previous non-wrapped line
	while (IsWrappedLine(startLine))
		--startLine;

	start = CharIndexOfLine(startLine);

	// get the next non-wrapping line
	while (DoesLineWrap(line))
		++line;

	end = CharIndexOfLine(line) + GetLineLength(line);

	if (endLine != NULL)
		*endLine = line;
}

void EditCtrl::GetTextMetrics(TEXTMETRIC& tm) const {
	CurDC dc(*this);
	MemClear(tm);
	VERIFY(dc.GetTextMetrics(&tm));
}

long EditCtrl::GetAvgCharWidth() const {
	if (m_avgCharWidth == -1)
		CalcCachedTextMetricValues();

	return m_avgCharWidth;
}

long EditCtrl::GetMaxCharWidth() const {
	if (m_maxCharWidth == -1)
		CalcCachedTextMetricValues();

	return m_maxCharWidth;
}

long EditCtrl::GetCharHeight() const {
	TEXTMETRIC tm;
	GetTextMetrics(tm);
	return tm.tmHeight;
}

void EditCtrl::ScrollCaret(ScrollKinds sk) {
	if (sk != skNone) {
		long lineOffset, lineNum;
		LineFromChar(lineOffset, lineNum, GetSelCaret());

		//Scroll vertically. 
		if (sk == skCenterSelection) {
			//Center caret vertically.
			SetFirstVisibleLine(lineNum - GetMaxVisibleLineCount(false) / 2);
		} else {
			//Scroll as little as possible to make sure the line with the caret
			//is completely visible.
			if (lineNum < GetFirstVisibleLine())
				SetFirstVisibleLine(lineNum);
			else {
				if (GetLineCount() <= GetMaxVisibleLineCount(true))//make first line the first visible line is all lines are visible
					SetFirstVisibleLine(0);
				else {
					long last = GetLastVisibleLine(true);
					if (lineNum > last)
						//make lineNum the last visible line
						SetFirstVisibleLine(lineNum + GetFirstVisibleLine() - last);
				}
			}
		}

		//Scroll horizontally.
		if (sk == skSelection || sk == skCenterSelection) {
			//Scroll horizontally. We must make the caret visible. Also, we should make as much
			//of the selection visible as possible. We should scroll as little as possible to
			//do this.
			long start, end;
			GetLineSel(lineNum, start, end);
			CurDC dc(*this);
			long startPixel = dc.GetLineWidth(lineNum, start);//left end of selection
			long endPixel = dc.GetLineWidth(lineNum, end);//right end of selection
			bool caretOnLeft = GetSelActiveEnd() <= GetSelAnchor();

			const long fvp = GetFirstVisiblePixel();//the left side of the screen
			const long lvp = fvp + GetClientWidth();//the right side of the screen
			long scrollAmount = 0;

			if (start == end) {//if no selection
				//make sure caret itself is visible
				endPixel += GetCaretWidth();

				if (startPixel < fvp)
					scrollAmount = startPixel - fvp;
				else if (endPixel > lvp)
					scrollAmount = endPixel - lvp;
			} else {
				if (caretOnLeft) {
					if (startPixel < fvp)
						scrollAmount = startPixel - fvp;//scroll left
					else if (endPixel > lvp) {
						long amount1 = startPixel - fvp;
						long amount2 = endPixel - lvp;
						scrollAmount = Min(amount1, amount2);//scroll right
					}
				} else {
					if (endPixel > lvp)
						scrollAmount = endPixel - lvp;//scroll right
					else if (startPixel < fvp) {
						long amount1 = lvp - endPixel;
						long amount2 = fvp - startPixel;
						scrollAmount = -Min(amount1, amount2);//scroll left
					}
				}
			}

			SetFirstVisiblePixel(fvp + scrollAmount);
		} else if (sk == skCaret) {
			CurDC dc(*this);
			long curPixel = dc.GetLineWidth(lineNum, GetSelCaret() - lineOffset);
			const long hScrollCaretAmount = Min(15 * GetHScrollAmount(), GetClientWidth());

			if (curPixel < GetFirstVisiblePixel())
				SetFirstVisiblePixel(curPixel - hScrollCaretAmount);
			else if (curPixel >= GetFirstVisiblePixel() + GetClientWidth())
				SetFirstVisiblePixel(curPixel + hScrollCaretAmount - GetClientWidth());
		}
	}
}

void EditCtrl::UpdateHScroll() {
	SCROLLINFO si;
	InitSized(si);
	si.fMask = SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = GetWidestLineWidth() - 1;
	si.nPage = GetClientWidth();
	si.nPos = GetFirstVisiblePixel();
	SetScrollInfo(SB_HORZ, &si);
}

void EditCtrl::UpdateVScroll() {
	SCROLLINFO si;
	InitSized(si);
	si.fMask = SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = GetLineCount() - 1;
	si.nPage = GetMaxVisibleLineCount(true);
	si.nPos = GetFirstVisibleLine();
	SetScrollInfo(SB_VERT, &si);
}

//Returns line relative indices of selection if part of line is selected. Returns 0 for
//start and end if no part of the line is selected.
void EditCtrl::GetLineSel(long line, long& start, long& end) const {
	long selStart, selEnd;
	GetSelRange(selStart, selEnd);

	long lineStart, lineEnd;
	GetLineRange(line, lineStart, lineEnd);

	if (selEnd < lineStart || selStart > lineEnd) {
		start = end = 0;
	} else {
		start = Max(selStart, lineStart) - lineStart;
		end = Min(selEnd, lineEnd) - lineStart;
	}
}

void EditCtrl::SelectWord(long pos) {
	long start, end;
	GetWordAt(pos, start, end);
	SetSel(start, end);
}

CString EditCtrl::GetPreSpace(long line, long caretOffset) const {
	ATLASSERT(0 <= line && line < GetLineCount());

	//if this line is wrapped, use the first previous non-wrapped
	//line for auto indention
	while (IsWrappedLine(line))
		--line;

	const CString& string = GetLine(line);

	LPCTSTR start = string, p = start;
	while (_istspace(*p))
		++p;

	long numChars = Min(long(p - start), caretOffset);
	return string.Left(numChars);
}

long EditCtrl::GetNumSpacesBeforePos(long line, long caretOffset) const {
	ATLASSERT(0 <= line && line < GetLineCount());

	const CString& string = GetLine(line);

	LPCTSTR start = string, p = start + caretOffset - 1;
	LPCTSTR end = p;
	while (p >= start && _istspace(*p))
		--p;

	return end - p;
}

void EditCtrl::NotifyParent(WORD code) const {
	SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), code), (LPARAM)m_hWnd);
}

void EditCtrl::CalcGutterWidth(bool* rewrapped/* = NULL*/) {
	if (rewrapped != NULL)
		*rewrapped = false;

	long newGutterWidth = GetShowBookmarks() ? BOOKMARK_WIDTH : 0;

	if (GetShowLineNumbers()) {
		//make some extra space between line numbers and bookmark
		if (newGutterWidth > 0)
			newGutterWidth += 2;

		CurDC dc(*this);
		//I'm assuming no number is wider than 8
		newGutterWidth += GetNumDigits(GetLfLineCount()) * dc.GetTextExtent("8").cx;
	}

	if (newGutterWidth > 0)
		newGutterWidth += SPACE_BETWEEN_GUTTER_AND_CLIENT_AREA;

	if (m_gutterWidth != newGutterWidth) {
		m_gutterWidth = newGutterWidth;

		if (IsWindow()) {
			//I need to call SetWindowPos() with the SWP_FRAMECHANGED flag so
			//WM_NCCALCSIZE will be sent since the size of the non-client area
			//has changed. I've found a lot less flickering occurs if I also pass
			//SWP_NOREDRAW and invalidate the client and non-client area manually.
			SetWindowPos(NULL, CRect(), SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW);

			// Must do this because the client width changed. Must do this after
			// the call to SetWindowPos so the new client size will be used.
			RewrapAllLinesIfNecessary();

			if (rewrapped != NULL)
				*rewrapped = true;

			Invalidate();
			SendMessage(WM_NCPAINT, 1);//redraw non-client area
		}
	}
}

CRect EditCtrl::GetGutterRect() const {
	CRect rc;
	rc.left = GetSystemMetrics(SM_CXEDGE);
	rc.top = GetSystemMetrics(SM_CYEDGE);
	rc.right = rc.left + m_gutterWidth;
	rc.bottom = rc.top + GetClientHeight();
	if (GetVScroll())
		rc.bottom += GetSystemMetrics(SM_CYHSCROLL);
	return rc;
}

void EditCtrl::InvalidateGutter() {
	CRgn rgn;
	VERIFY(rgn.CreateRectRgnIndirect(&GetGutterRect()));
	SendMessage(WM_NCPAINT, (WPARAM)(HRGN)rgn);
}

void EditCtrl::IncreaseSel(long amount) {
	long start, end;
	GetSel(start, end);
	SetSel(start, end + amount);//change the active end
}

CString EditCtrl::GetTabSpaceString() const {
	return DupChar(' ', m_tabWidth);
}

void EditCtrl::ReplaceTabsWithSpacesIfNecessary(CString& string) const {
	if (m_tabInsertsSpaces)
		string.Replace(_T("\t"), GetTabSpaceString());
}

void EditCtrl::WrapOrUnwrapAllLines(bool wrap) {
	long selStart, selEnd;
	GetSel(selStart, selEnd);

	long oldVisibleLineOffset = LineFromChar(selEnd) - GetFirstVisibleLine();

	if (wrap)
		RewrapAllLines(selStart, selEnd);
	else
		UnwrapAllLines(selStart, selEnd);

	// Keep caret in the same vertical position by moving the first visible line
	// (FVL) by however many lines the offset from the FVL has grown by.
	// For example, if the caret is now two lines lower from the FVL, the FVL
	// must be moved two lines lower to compensate.
	// Note that the vertical scroll position can't be preseved exactly when close
	// to the buttom and unwrapping because the old offset may be after the last line
	// after unwrapping.
	long newVisibleLineOffset = LineFromChar(selEnd) - GetFirstVisibleLine();
	SetFirstVisibleLine(GetFirstVisibleLine() + newVisibleLineOffset - oldVisibleLineOffset);

	SetSel(selStart, selEnd, skSelection);
}

//Profiling shows calling GetTextMetrics() on every keystroke takes about 10% of the
//time, so text metric values are cached.
void EditCtrl::CalcCachedTextMetricValues() const {
	TEXTMETRIC tm;
	GetTextMetrics(tm);
	m_avgCharWidth = tm.tmAveCharWidth;
	m_lineHeight = tm.tmHeight;
	m_maxCharWidth = tm.tmMaxCharWidth;
}

void EditCtrl::InvalidateCachedTextMetricValues() const {
	m_avgCharWidth = -1;
	m_lineHeight = -1;
	m_maxCharWidth = -1;
}

void EditCtrl::OnPreChange(long pos, long count, const CString& newText) {
	m_preChangeLineCount = GetLineCount();

	m_visibleLines.clear();
	long first = GetFirstVisibleLine(), last = GetLastVisibleLine(false);
	for (long i = first; i <= last; ++i)
		m_visibleLines.push_back(GetCodeLine(i));

	//put selection at start of modification so that during modification
	//the text that the selection refers to doesn't change
	SetSelCaret(pos, false);
}

//pos is char index where text modification started. first and last are the first and last
//lines that were modified, respectively.
void EditCtrl::OnPostChange(long selStart, long selEnd, long first, long last, long charsAdded, long charsDeleted, long lfLinesAdded, long sizeChange, ScrollKinds scrollKind/* = skCaret*/) {
	long numLinesChange = GetLineCount() - m_preChangeLineCount;

	const long lastLine = GetLineCount() - 1;

	//update the charIndex's and the lfLineIndex's of the lines after last for changes
	if (sizeChange != 0 || lfLinesAdded != 0)
		for (long i = last + 1; i <= lastLine; ++i) {
			long& charIndex = GetLineData(i)->m_charIndexOfLine;
			if (charIndex != -1)
				charIndex += sizeChange;

			long& lfLineIndex = GetLineData(i)->m_lfLineIndex;
			if (lfLineIndex != -1)
				lfLineIndex += lfLinesAdded;
		}

	//if modification caused inconsistency between the ending and starting contexts of
	//two consecutive lines, then lines should be invalidated until there are no more
	//inconsistencies
	long i = last;
	while (i < lastLine) {
		const Context& endingContext = GetCodeLine(i)->GetEndingContext();
		const Context& startingContext = GetCodeLine(i+1)->GetStartingContext();
		if (endingContext == startingContext)
			break;

		++i;
		m_lines[i]->InvalidateCodeLine();
	}

	//*** Validate m_widestLineIndex ***

	if (m_widestLineIndex < first)
		ValidateWidestLine(first, last);
	else {
		long oldLastLine = last - numLinesChange;
		//if widest line itself may have been modified, any line could
		//be the new longest line
		ATLASSERT(first <= m_widestLineIndex);
		if (m_widestLineIndex <= oldLastLine) {
			m_widestLineIndex = 0;
			ValidateWidestLine(1, GetLineCount() - 1);
		} else {
			//widest line is after the last modified line,
			//so adjust index to compensate for any added or deleted lines
			m_widestLineIndex += numLinesChange;
			ValidateWidestLine(first, last);
		}
	}

	//make sure the validation of m_widestLineIndex is correct
	ATLASSERT(GetWidestLineWidth() == GetWidestLineWidthInefficiently());

	//Special case to remove bookmark from last line if all text is deleted.
	//The bookmark is still removed if text was added after all the text was deleted.
	if (GetTextLength() == charsAdded && charsDeleted > 0) {
		bool& lastLineBookmark = GetLineData(GetLineCount() - 1)->m_hasBookmark;
		if (lastLineBookmark) {
			lastLineBookmark = false;
			InvalidateGutter();
		}
	}

	//figure out what needs to be invalidated due to change
	long firstVisibleLine = GetFirstVisibleLine();
	long lastVisibleLine = Max(GetLastVisibleLine(false), firstVisibleLine + (long)m_visibleLines.size() - 1L);
	const long lineCount = GetLineCount();
	CRect rc(0, 0, GetClientWidth(), GetLineHeight());

	for (i = firstVisibleLine; i <= lastVisibleLine; ++i) {
		//if indexing GetCodeLine() or m_visibleLines would not be valid,
		//invalidate the entire line
		if (i >= lineCount || i - firstVisibleLine >= m_visibleLines.size())
			InvalidateRect(rc);
		else {
			const CodeLine& oldLine = *m_visibleLines[i - firstVisibleLine];
			const CodeLine& newLine = *GetCodeLine(i);
			long diff = CodeLine::FindFirstDiff(oldLine, newLine);
			if (diff != -1) {
				CurDC dc(*this);
				rc.left = dc.GetLineWidth(i, diff) - GetFirstVisiblePixel();
				InvalidateRect(rc);
				rc.left = 0;
			}
		}

		rc.OffsetRect(0, GetLineHeight());
	}

	//validate first visible line
	SetFirstVisibleLine(GetFirstVisibleLine());

	SetSel(selStart, selEnd, scrollKind);

	//if the caret hasn't moved but chars were deleted in overwrite mode, then
	//the caret is on a different character
	if (charsAdded == 0 && charsDeleted > 0 && !GetInsert())
		CreateCaret();

	//If a line is added
	//or removed, a bookmarked may have been moved or delete, so the gutter may
	//need to be redrawn.
	//Also, if line numbers are being displayed, adding or deleting a line should
	//invalidate the gutter.
	if (numLinesChange != 0) {
		// unimplemented optimization:
		// When showing line numbers, word wrap is often done twice when pasting in
		// large amount of text: once in ModifyText and once in CalcGutterWidth.
		// It would be nice to only do this once.
		CalcGutterWidth();
		InvalidateGutter();
		UpdateVScroll();
	}

	UpdateHScroll();

	NotifyParent(ECN_CHANGE);
}

EditCtrl::EditCtrl() {
	m_maxLength = 0;
	m_undoOn = true;
	m_undoLimit = 5000;
	m_modified = false;
	m_unmodifyUndoSize = 0;
	m_wordWrap = false;
	m_lines.push_back(NewLineData());
	m_font.Attach((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	MemClear(m_sel);
	m_firstVisibleLine = 0;
	m_firstVisiblePixel = 0;
	InvalidateCachedTextMetricValues();
	m_widestLineIndex = 0;
	m_preChangeLineCount = 0;
	m_insert = true;
	m_readOnly = false;
	m_autoIndent = true;
	m_greyedWhenReadOnly = false;
	m_useSystemSelectionColors = true;
	m_showBookmarks = true;
	m_showLineNumbers = false;
	CalcGutterWidth();
	SetTabWidth(4);
	m_curOffset = -1;
	m_mouseDownFromSelectWord = false;
	m_mouseDownInControl = false;
	m_tabInsertsSpaces = false;
	m_showWhitespace = false;
}

bool EditCtrl::Create(HWND parent, RECT& rc, DWORD style, DWORD exStyle, UINT id/* = 0*/, LanguagePtr lang/* = NULL*/, TokenColorsPtr tokenColors/* = NULL*/) {
	SetLanguage(lang);
	SetTokenColors(tokenColors);
	m_hScroll = (style & WS_HSCROLL) != 0;
	m_vScroll = (style & WS_VSCROLL) != 0;
	bool result = WndBase::Create(parent, rc, NULL, style, exStyle, id) != NULL;
	SetHScroll(m_hScroll);
	SetVScroll(m_vScroll);
	return result;
}

void EditCtrl::SetText(const CString& newText, bool modified/* = true*/) {
	ModifyText(0, GetTextLength(), newText, 0, 0, skCaret, modified);
}

CString EditCtrl::GetTextEx(NewLineType type) const {
	CString text = GetText();

	//num LFs = num lines - num CRs
	long numLFs = GetLineCount();

	if (GetWordWrap())
		numLFs -= RemoveCR(text);

	if (type == nlCR)
		ConvertLFtoCR(text);
	else if (type == nlCRLF)
		text = ConvertLFtoCRLF(text, numLFs);

	return text;
}

//It would be slightly more efficient for this method to expect newSel
//to never contain CRs like ModifyText (due to cases where callers already
//know newSel doesn't have CRs), but probably not enough faster to be worth
//fixing up the callers.
void EditCtrl::ReplaceSel(const CString& newSel, bool selectReplacement/* = false*/) {
	long start, end;
	GetSelRange(start, end);

	CString realNewSel = newSel;

	if (GetWordWrap())
		RemoveCR(realNewSel);

	ModifyText(start, end - start, realNewSel,
		selectReplacement ? start : -1,
		selectReplacement ? start + realNewSel.GetLength() : -1,
		selectReplacement ? skSelection : skCaret);
}

//the returned line never has a trailing CR or LF
const CString& EditCtrl::GetLine(long i) const {
	LineDataPtr lineData = GetLineData(i);

	if (!lineData->m_lineValid) {
		long start, end;
		GetLineRange(i, start, end);
		lineData->m_line = GetTextRange(start, end);
		lineData->m_lineValid = true;
	}

	return lineData->m_line;
}

// Gets line without breaking on wrapped lines.
CString EditCtrl::GetHardLine(long i) const {
	long start, end;
	GetHardLineRange(i, start, end);
	return GetTextRange(start, end);
}

long EditCtrl::GetLineLength(long line) const {
	LineDataPtr lineData = GetLineData(line);

	//Use the length cached in lineData->m_line if it's valid, but otherwise
	//calculate it. Since this function is called by functions that calculate
	//lineData->m_line, it must work when lineData->m_line isn't valid yet.
	if (lineData->m_lineValid)
		return lineData->m_line.GetLength();
	else {
		LPCTSTR start = (LPCTSTR)GetText() + CharIndexOfLine(line), end = start;
		while (*end != NULL && !IsNewLine(*end))
			++end;

		return end - start;
	}
}

long EditCtrl::CharIndexOfLine(long line) const {
	LineDataPtr lineData = GetLineData(line);
	long& charIndex = lineData->m_charIndexOfLine;

	if (charIndex < 0)
		ValidateCharIndexOfLine(line);

	return charIndex;

	/*
	Old code that calculates CharIndexOfLine without using line data.

	ATLASSERT(0 <= line && line < GetLineCount());

	//this code is hard to read due to lexical tricks, but it NEEDS to be efficient
	LPCTSTR start = GetText(), p = start;
	while (line-- > 0)
		while (!IsNewLine(*p++));

	return p - start;
	*/
}

void EditCtrl::LineFromChar(long& charIndex, long& lineNum, long pos) const {
	ATLASSERT(0 <= pos && pos <= GetTextLength());

	LPCTSTR start = GetText(), p = start + pos;

	//calculate the line index
	while (p > start && !IsNewLine(*(p - 1)))
		--p;
	charIndex = p - start;

	//use the line index to calculate lineNum
	lineNum = FindCharIndexOfLine(charIndex);
}

long EditCtrl::LineFromChar(long pos) const {
	long charIndex, lineNum;
	LineFromChar(charIndex, lineNum, pos);
	return lineNum;
}

long EditCtrl::GetLfLineIndex(long line) const {
	if (GetWordWrap()) { // optimization for non-word wrap mode
		LineDataPtr lineData = GetLineData(line);
		long& lfLineIndex = lineData->m_lfLineIndex;

		if (lfLineIndex < 0)
			ValidateLfLineIndex(line);

		return lfLineIndex;
	} else
		return line;
}

long EditCtrl::GetLineIndexFromLfLineIndex(long lfLine) const {
	if (GetWordWrap()) { // optimization for non-word wrap mode
		long line = FindLfLineIndex(lfLine);

		// All wrapped lines have the same lfLineIndex. We need to
		// find the first such line.
		while (IsWrappedLine(line))
			--line;

		return line;
	} else
		return lfLine;
}

bool EditCtrl::DoesLineWrap(long line) const {
	long pos = CharIndexOfLine(line) + GetLineLength(line);
	return pos < GetTextLength() && GetText().GetAt(pos) == '\r';
}

void EditCtrl::SetFirstVisibleLine(long fvl) {
	fvl = Max(0L, fvl);
	long maxVisible = Min(GetMaxVisibleLineCount(true), GetLineCount());
	fvl = Min(GetLineCount() - maxVisible, fvl);

	if (m_firstVisibleLine != fvl) {
		int dy = (m_firstVisibleLine - fvl) * GetLineHeight();
		m_firstVisibleLine = fvl;
		VERIFY(ScrollWindow(0, dy));
		InvalidateGutter();

		//I can't update the scroll position when pressing up or down
		//because SetScrolPos() would slow scrolling down noticably.
		//The scroll position will get updated in OnKeyUp().
		if (!IsKeyDown(VK_UP) && !IsKeyDown(VK_DOWN))
			SetScrollPos(SB_VERT, fvl);
	}
}

long EditCtrl::GetLastVisibleLine(bool completelyVisible) const {
	return
		Min(GetFirstVisibleLine() + GetMaxVisibleLineCount(completelyVisible),
			GetLineCount()) - 1L;
}

//If completelyVisible is true, the number of lines that are entirely visible
//are returned.
//If completelyVisible is true, the number of lines that any part of are visible
//are returned.
long EditCtrl::GetMaxVisibleLineCount(bool completelyVisible) const {
	long lineHeight = GetLineHeight();
	return Div(GetClientHeight(), lineHeight, completelyVisible ? rkDown : rkUp);
}

void EditCtrl::SetFirstVisiblePixel(long fvp) {
	// + 1 because caret can go past last char and we want to always be able to
	// press End and see the caret
	fvp = Min(fvp, GetWidestLineWidth() - GetClientWidth() + 1L);
	fvp = Max(0L, fvp);

	if (m_firstVisiblePixel != fvp) {
		int dx = m_firstVisiblePixel - fvp;
		m_firstVisiblePixel = fvp;
		VERIFY(ScrollWindow(dx, 0));

		//I can't update the scroll position when pressing up or down
		//because SetScrolPos() would slow scrolling down noticably.
		//The scroll position will get updated in OnKeyUp().
		if (!IsKeyDown(VK_LEFT) && !IsKeyDown(VK_RIGHT))
			SetScrollPos(SB_HORZ, fvp);
	}
}

// returns 1 based line and column numbers
// real column position is used, i.e. as if there are no wrapping CRs
void EditCtrl::GetLineAndColNum(long& lineNum, long& colNum) const {
	long end = GetSelCaret();
	long lineIndex = 0;

	// use real line and column if wrapping
	if (GetWordWrap()) {
		long line = LineFromChar(end);

		while (IsWrappedLine(line))
			--line;

		lineIndex = CharIndexOfLine(line);

		// so column number won't count CRs
		ConvertCRLFIndicesToLFIndices(GetText(), &end, 1, lineIndex);

		lineNum = GetLfLineIndex(line);
	} else
		LineFromChar(lineIndex, lineNum, end);

	colNum = end - lineIndex;

	// make one based
	++lineNum;
	++colNum;
}

void EditCtrl::SetSel(long anchor, long end, ScrollKinds sk/* = skCaret*/) {
	ValidatePos(anchor);
	ValidatePos(end);

	bool selChanged = m_sel.cpMin != anchor || m_sel.cpMax != end;
	if (selChanged) {
		long oldStart, oldEnd;
		GetSelRange(oldStart, oldEnd);
		long newStart = Min(anchor, end);
		long newEnd = Max(anchor, end);

		m_sel.cpMin = anchor;
		m_sel.cpMax = end;

		//must call ScrollCaret() before PosFromChar() or InvalidateTextRange() because
		//ScrollCaret() modifies the first visible pixel and line which
		//PosFromChar() and InvalidateTextRange() use
		ScrollCaret(sk);

		SetCaretPos(PosFromChar(end));

		//recreate the caret when in overwrite mode
		if (end != oldEnd && !m_insert)
			CreateCaret();

		//I want to invalidate only the stuff that needs it, i.e. text that is selected that
		//was not before and text that is not selected that was before. I put the sections
		//of text over which the selection is changing into vec. Then I iterate through vec,
		//and any section in vec that is only inside the new selection or only inside the old
		//selection gets invalidated.
		vector<long> vec;
		vec.push_back(newStart);
		vec.push_back(newEnd);
		vec.push_back(oldStart);
		vec.push_back(oldEnd);
		sort(vec.begin(), vec.end());

		for (size_t i = 0; i < vec.size() - 1; ++i) {
			long start = vec[i], end = vec[i + 1];
			bool insideNew = start >= newStart && end <= newEnd;
			bool insideOld = start >= oldStart && end <= oldEnd;
			if (insideNew != insideOld)
				InvalidateTextRange(start, end);
		}

		NotifyParent(ECN_SELCHANGE);
	} else
		ScrollCaret(sk);
}

void EditCtrl::GetSelNoCRs(long& anchor, long& end) const {
	GetSel(anchor, end);

	if (GetWordWrap()) // optimization for non-word wrap case
		ConvertCRLFIndicesToLFIndices(m_text, SelArrayConv(anchor, end), 2);
}

void EditCtrl::SetSelNoCRs(long anchor, long end, ScrollKinds sk/* = skCaret*/) {
	if (GetWordWrap()) // optimization for non-word wrap case
		ConvertLFIndicesToCRLFIndices(m_text, SelArrayConv(anchor, end), 2);

	SetSel(anchor, end, sk);
}

void EditCtrl::GetSelRange(long& start, long& end) const {
	start = Min(m_sel.cpMin, m_sel.cpMax);
	end = Max(m_sel.cpMin, m_sel.cpMax);
}

CString EditCtrl::GetSelText() const {
	long start, end;
	GetSelRange(start, end);
	return GetTextRange(start, end);
}

long EditCtrl::GetCaretLine() const {
	return LineFromChar(GetSelCaret());
}

//goes to a specified 0 based line number
void EditCtrl::GotoLine(long line) {
	//validate the line number
	line = Min(line, GetLineCount() - 1L);
	line = Max(line, 0L);

	//scroll vertically so that the line is in the middle of the control
	SetFirstVisibleLine(line - GetMaxVisibleLineCount(false) / 2);

	//go to the line
	long index = CharIndexOfLine(line);
	SetSelCaret(index);
}

void EditCtrl::GotoLfLine(long line) {
	if (line >= 0)
		if (line < GetLfLineCount())
			line = GetLineIndexFromLfLineIndex(line);
		else
			line = GetLineCount() - 1L;

	GotoLine(line);
}

// goes to 1 based line and column number
// real column position is used, i.e. as if there are no wrapping CRs
// symmetrycal with GetLineAndColNum
void EditCtrl::GotoLineAndColNum(long realLine, long realCol/* = 0*/) {
	if (realLine > 0) {
		GotoLfLine(realLine - 1);//- 1 to make zero-based

		if (realCol > 0) {
			long caret = GetSelCaret();
			long charIndex, line;
			LineFromChar(charIndex, line, caret);

			caret = charIndex + realCol - 1;//- 1 to make zero-based

			if (GetWordWrap())
				caret = ConvertLFPosToCRLFPos(GetText(), caret, charIndex, true);

			while (DoesLineWrap(line))
				++line;

			long lineEndIndex = CharIndexOfLine(line) + GetLineLength(line);
			if (caret > lineEndIndex)
				caret = lineEndIndex;

			SetSelCaret(caret);
		}
	}
}

void EditCtrl::RecolorAll() {
	for (long i = 0; i < GetLineCount(); ++i)
		GetLineData(i)->InvalidateCodeLine();

	if (IsWindow()) {
		Invalidate();
		InvalidateGutter();//some of the background color is drawn in the gutter
	}
}

void EditCtrl::GetWordAt(long pos, long& start, long& end) {
	end = ::GetNextWordPos(GetText(), pos, false);
	start = ::GetPrevWordPos(GetText(), end, false);
}

CPoint EditCtrl::PosFromChar(long pos) const {
	long offset, num;
	LineFromChar(offset, num, pos);

	CurDC dc(*this);
	int cx = dc.GetLineWidth(num, pos - offset);
	return CPoint(cx - GetFirstVisiblePixel(), (num - GetFirstVisibleLine()) * GetLineHeight());
}

long EditCtrl::CharFromPos(const CPoint& pt) const {
	long line = pt.y / GetLineHeight() + GetFirstVisibleLine();
	if (line < 0)
		return 0;
	if (line >= GetLineCount())
		return GetTextLength();

	const CString& text = GetLine(line);
	int tabLength = GetTabWidthInPixels();

	HDC thisDc = ::GetDC(m_hWnd);
	HDC compatDc = CreateCompatibleDC(thisDc);
	ATLASSERT(compatDc != NULL);
	VERIFY(::ReleaseDC(m_hWnd, thisDc));
	CurDC dc(*this, compatDc);
	int minusFvp = -GetFirstVisiblePixel();
	int x = minusFvp;
	//draw the line one character at a time (on a memory DC) and return when
	//the width of the drawn characters exceeds pt.x
	for (long i = 0; i < text.GetLength(); ++i) {
		x += dc.TabbedTextOut(x, 0, (LPCTSTR)text + i, 1, 1, &tabLength, minusFvp).cx;
		if (pt.x < x)
			return i + CharIndexOfLine(line);
	}

	VERIFY(DeleteDC(compatDc));

	return CharIndexOfLine(line) + GetLineLength(line);
}

long EditCtrl::GetClientWidth() const {
	CRect rc;
	GetClientRect(rc);
	return rc.Width();
}

long EditCtrl::GetClientHeight() const {
	CRect rc;
	GetClientRect(rc);
	return rc.Height();
}

long EditCtrl::GetLineHeight() const {
	if (m_lineHeight == -1)
		CalcCachedTextMetricValues();

	return m_lineHeight;
}

void EditCtrl::SetFont(const CFont& font) {
	if (!FontsEqual(m_font, font)) {
		if ((HFONT)m_font != NULL)
			VERIFY(m_font.DeleteObject());
		m_font.Attach(CloneFont(font));

		InvalidateCachedTextMetricValues();

		InvalidateLineWidths();

		bool rewrapped = false;
		CalcGutterWidth(&rewrapped);//line numbers are drawn with the current font

		if (!rewrapped) // optimization, wrapping can be slow
			RewrapAllLinesIfNecessary();

		CreateCaret();
		SetFirstVisibleLine(GetFirstVisibleLine());//validate first visible line
		SetFirstVisiblePixel(GetFirstVisiblePixel());//validate first visible pixel
		UpdateHScroll();
		UpdateVScroll();
		Invalidate();
	}
}

void EditCtrl::SetHScroll(bool val) {
	m_hScroll = val;
	ShowScrollBar(SB_HORZ, val);
	UpdateHScroll();
}

void EditCtrl::SetVScroll(bool val) {
	m_vScroll = val;
	ShowScrollBar(SB_VERT, val);
	UpdateVScroll();
}

bool EditCtrl::CanCut() const {
	return IsWriteable() && CanCopy();
}

bool EditCtrl::CanCopy() const {
	return GetTextLength() > 0;
}

bool EditCtrl::CanPaste() const {
	if (!IsWriteable())
		return false;
	else
		return 
			IsClipboardFormatAvailable(CF_TEXT) ||
#ifdef _UNICODE
			IsClipboardFormatAvailable(CF_UNICODETEXT) ||
#endif
			IsClipboardFormatAvailable(CF_HDROP);
}

void EditCtrl::Cut() {
	if (CanCut()) {
		Copy();

		if (HasSel())
			DeleteSel();
		else {
			//delete the current line

			long caretLine = GetCaretLine();
			long start, end;
			long endLine;
			GetHardLineRange(caretLine, start, end, &endLine);

			//we need to delete a new line character also if there is more than one
			//line
			if (GetLineCount() > 1)
				//delete previous new line character if we're on the last line and
				//there is no trailing new line character
				if (endLine == GetLineCount() - 1)
					--start;
				else
					++end;

			ModifyText(start, end - start, EmptyStr);
		}
	}
}

void EditCtrl::Copy() const {
	if (CanCopy()) {
		//Copy the selection if there is one. Otherwise, copy the current line.
		CString textToCopy = HasSel() ? GetSelText() : GetHardLine(GetCaretLine()) + TEXT("\n");

		if (GetWordWrap())
			RemoveCR(textToCopy);

		VERIFY(SetClipboardString(m_hWnd, textToCopy, true));
	}
}

void EditCtrl::Paste() {
	if (CanPaste()) {
		CString clipboard;
		if (GetClipboardString(m_hWnd, clipboard, true) ||
			GetClipboardFilesString(m_hWnd, clipboard, true))
			ReplaceSel(clipboard);
	}
}

void EditCtrl::ToggleBookmark() {
	LineDataPtr data = GetLineData(GetCaretLine());
	data->m_hasBookmark = !data->m_hasBookmark;
	InvalidateGutter();
}

void EditCtrl::NextBookmark(bool next) {
	long line = GetCaretLine();
	long i;

	if (next) {
		for (i = line + 1; i < GetLineCount(); ++i)
			if (GetLineData(i)->m_hasBookmark) {
				GotoLine(i);
				return;
			}

		for (i = 0; i <= line; ++i)
			if (GetLineData(i)->m_hasBookmark) {
				GotoLine(i);
				return;
			}
	} else {
		for (i = line - 1; i >= 0; --i)
			if (GetLineData(i)->m_hasBookmark) {
				GotoLine(i);
				return;
			}

		for (i = GetLineCount() - 1; i >= line; --i)
			if (GetLineData(i)->m_hasBookmark) {
				GotoLine(i);
				return;
			}
	}
}

void EditCtrl::ClearAllBookmarks() {
	for (long i = 0; i < GetLineCount(); ++i)
		GetLineData(i)->m_hasBookmark = false;

	InvalidateGutter();
}

void EditCtrl::AddBookmarks(const vector<long>& bookmarks) {
	for (size_t i = 0; i < bookmarks.size(); ++i)
		GetLineData(bookmarks[i])->m_hasBookmark = true;

	if (!bookmarks.empty())
		InvalidateGutter();
}

void EditCtrl::SetInsert(bool b) {
	if (m_insert != b) {
		m_insert = b;
		CreateCaret();
	}
}

void EditCtrl::SetReadOnly(bool b) {
	if (b != m_readOnly) {
		m_readOnly = b;
		Invalidate();
	}
}

COLORREF EditCtrl::GetSelForeColor() const {
	if (HasFocus()) {
		if (GetUseSystemSelectionColors())
			return GetSysColor(COLOR_HIGHLIGHTTEXT);
		else
			return GetColor(tkBackground);
	} else
		return GetSysColor(COLOR_WINDOW);
}

COLORREF EditCtrl::GetSelBackColor() const {
	if (HasFocus()) {
		if (GetUseSystemSelectionColors())
			return GetSysColor(COLOR_HIGHLIGHT);
		else
			return GetColor(tkPlainText);
	} else
		return GetSysColor(COLOR_BTNSHADOW);
}

void EditCtrl::SetTabWidth(long newTabWidth) {
	if (m_tabWidth != newTabWidth) {
		m_tabWidth = newTabWidth;

		if (IsWindow()) {
			InvalidateLineWidths();
			SetFirstVisiblePixel(GetFirstVisiblePixel());//validate first visible pixel
			ScrollCaret(skCaret);
			UpdateHScroll();
			Invalidate();
		}
	}
}

void EditCtrl::SetShowBookmarks(bool val) {
	m_showBookmarks = val;
	CalcGutterWidth();
}

void EditCtrl::SetShowLineNumbers(bool val) {
	m_showLineNumbers = val;
	CalcGutterWidth();
}

void EditCtrl::SetShowGreyedWhenReadOnly(bool val) {
	if (val != m_greyedWhenReadOnly) {
		m_greyedWhenReadOnly = val;
		Invalidate();
	}
}

void EditCtrl::SetUseSystemSelectionColors(bool val) {
	if (val != m_useSystemSelectionColors) {
		m_useSystemSelectionColors = val;
		Invalidate();
	}
}

// can't use the default value for the fileDir parameter (3rd parameter) because
// this is static and may be initialized before PathMgr
static LanguagePtr EmptyLanguage = new Language(EmptyStr, false, EmptyStr);

void EditCtrl::SetLanguage(LanguagePtr lang) {
	if (lang == NULL)
		lang = EmptyLanguage;

	if (m_lang != lang) {
		m_lang = lang;
		RecolorAll();
	}
}

static TokenColorsPtr DefaultColors = new TokenColors();

void EditCtrl::SetTokenColors(TokenColorsPtr clrs) {
	if (clrs == NULL)
		clrs = DefaultColors;

	if (m_tokenColors != clrs) {
		m_tokenColors = clrs;
		RecolorAll();
	}
}

void EditCtrl::SetContextMenu(HMENU menu) {
	if (m_contextMenu != NULL)
		VERIFY(m_contextMenu.DestroyMenu());

	VERIFY(m_contextMenu.Attach(menu));
}

void EditCtrl::SetWordWrap(bool val) {
	if (val != m_wordWrap) {
		m_wordWrap = val;
		WrapOrUnwrapAllLines(val);
	}
}

LRESULT EditCtrl::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	BYTE key = wParam;
	KeyModifiers mods = GetKeyModifiers();

	if (key == VK_UP || key == VK_DOWN) {
		long offset, line, pos = GetSelCaret();
		LineFromChar(offset, line, pos);

		bool willMove = (mods & kmShift) != 0 ?
			key == VK_UP || key == VK_DOWN :
			(key == VK_UP && line > 0) || (key == VK_DOWN && line < GetLineCount() - 1);

		if (willMove) {
			long fvp = GetFirstVisiblePixel();
			if (m_curOffset < 0)
				m_curOffset = PosFromChar(pos).x + fvp;

			if (key == VK_UP)
				--line;
			else
				++line;

			long curLineY = GetLineHeight() * (line - GetFirstVisibleLine());
			long newPos = CharFromPos(CPoint(m_curOffset - fvp, curLineY));

			if ((mods & kmShift) != 0)
				IncreaseSel(newPos - pos);
			else
				SetSelCaret(newPos);

			//Ctrl + Up/Down scrolls
			if ((mods & kmCtrl) != 0) {
				long fvl = GetFirstVisibleLine();
				if (key == VK_UP)
					--fvl;
				else
					++fvl;

				SetFirstVisibleLine(fvl);
			}
		} else if (mods == kmNone && HasSel()) {
			RemoveSel();
			m_curOffset = -1;
		}
	} else if (key != VK_ESCAPE)
		m_curOffset = -1;

	switch (key) {
	case VK_LEFT :
		if (mods == kmNone)
			SetSelCaret(GetSelCaret() - 1);
		else if (mods == kmShift)
			IncreaseSel(-1);
		else if (mods == kmCtrl)
			SetSelCaret(GetPrevWordPos());
		else if (mods == (kmCtrl|kmShift))
			IncreaseSel(GetPrevWordPos() - GetSelCaret());
		break;
	case VK_RIGHT :
		if (mods == kmNone)
			SetSelCaret(GetSelCaret() + 1);
		else if (mods == kmShift)
			IncreaseSel(1);
		else if (mods == kmCtrl)
			SetSelCaret(GetNextWordPos());
		else if (mods == (kmCtrl|kmShift))
			IncreaseSel(GetNextWordPos() - GetSelCaret());
		break;
	case VK_HOME : {
		if (mods == kmNone || mods == kmShift) {
			long lineOffset, lineNum;
			LineFromChar(lineOffset, lineNum, GetSelCaret());

			if (mods == kmNone)
				SetSelCaret(lineOffset);
			else
				IncreaseSel(lineOffset - GetSelCaret());
		} else if (mods == kmCtrl)
			SetSelCaret(0);
		else if (mods == (kmShift|kmCtrl))
			IncreaseSel(-GetSelCaret());
		break;
	}
	case VK_END : {
		if (mods == kmNone || mods == kmShift) {
			long lineOffset, lineNum;
			LineFromChar(lineOffset, lineNum, GetSelCaret());
			long len = GetLineLength(lineNum);

			if (mods == kmNone)
				SetSelCaret(lineOffset + len);
			else
				IncreaseSel(len - (GetSelCaret() - lineOffset));
		} else if (mods == kmCtrl)
			SetSelCaret(GetTextLength());
		else if (mods == (kmShift|kmCtrl))
			IncreaseSel(GetTextLength() - GetSelCaret());
		break;
	}
	case VK_PRIOR :
		if ((mods == kmNone || mods == kmShift) && GetCaretLine() > 0) {
			CPoint pt = PosFromChar(GetSelCaret());
			pt.y -= GetClientHeight();
			pt.x += GetAvgCharWidth() / 2;
			long pos = CharFromPos(pt);
			SetFirstVisibleLine(GetFirstVisibleLine() - GetVisibleLineCount(true));
			if (mods == kmNone)
				SetSelCaret(pos);
			else
				IncreaseSel(pos - GetSelCaret());
		}
		break;
	case VK_NEXT :
		if ((mods == kmNone || mods == kmShift) && GetCaretLine() < GetLineCount() - 1) {
			CPoint pt = PosFromChar(GetSelCaret());
			//long lastLinePixel = GetClientHeight() - GetClientHeight();
			pt.y += GetClientHeight();
			pt.x += (GetAvgCharWidth() + 1) / 2;
			long pos = CharFromPos(pt);
			SetFirstVisibleLine(GetFirstVisibleLine() + GetVisibleLineCount(true));
			if (mods == kmNone)
				SetSelCaret(pos);
			else
				IncreaseSel(pos - GetSelCaret());
		}
		break;
		case 'A' :
			if (mods == kmCtrl)
				SelectAll();
			break;
		case VK_INSERT :
			if (mods == kmNone)
				SetInsert(!GetInsert());
			break;
		case VK_ESCAPE :
			if (mods == kmNone)
				SetSelCaret(GetSelCaret());//deselect text
			break;
	}

	if ((key == 'C' && mods == kmCtrl) || (key == VK_INSERT && mods == kmCtrl))
		Copy();

	if (GetReadOnly())//the rest of this method handles keys that cause modifications
		return 0;

	if ((key == 'V' && mods == kmCtrl) || (key == VK_INSERT && mods == kmShift))
		Paste();
	else if ((key == 'X' && mods == kmCtrl) || (key == VK_DELETE && mods == kmShift))
		Cut();

	if (key == 'Z' && mods == kmCtrl)
		Undo();
	else if (key == 'Y' && mods == kmCtrl)
		Redo();

	switch (key) {
	case VK_DELETE :
		if (mods == kmNone) {
			//pos is first char to delete
			long pos = GetSelStart();

			//if trying to delete a (possibly selected) soft line break
			if ((GetSelCount() == 0 || GetSelCount() == 1) &&
				GetChar(GetSelStart()) == '\r') {

				//delete the '\r' and the char after it
				ModifyText(pos, 2, EmptyStr);
			} else {
				if (HasSel())
					DeleteSel();
				else if (pos < GetTextLength())
					DeleteChar(pos);
			}
		}
		break;
	}

	bHandled = FALSE;
	return 0;
}

LRESULT EditCtrl::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	VirtualKey key = (VirtualKey)wParam;

	//I have to update the scroll position here because updating the scroll
	//position is too slow to do on every key down.

	if (key == VK_UP || key == VK_DOWN)
		if (GetScrollPos(SB_VERT) != GetFirstVisibleLine())
			SetScrollPos(SB_VERT, GetFirstVisibleLine());

	if (key == VK_LEFT || key == VK_RIGHT)
		if (GetScrollPos(SB_HORZ) != GetFirstVisiblePixel())
			SetScrollPos(SB_HORZ, GetFirstVisiblePixel());

	bHandled = FALSE;
	return 0;
}

LRESULT EditCtrl::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (GetReadOnly())
		return 0;

	TCHAR key = wParam;
	if (key == '\r')
		key = '\n';

	//Don't allow control characters other than a new line, back space, and tab.
	//I don't check for IsKeyDown(VK_CONTROL) because on an internation keyboard,
	//true is returned for that when the right hand alt key is down which prevents
	//typing some symbols on an internation keyboard.
	//I check for both key's value and which key is down so that things like
	//Ctrl+M won't insert a new line since Ctrl+M fires an OnChar with key == '\n'.
	//Note that I use the iscntrl() function that takes a locale instead of _istcntrl()
	//because _istcntrl() returns false for the ç (c-cedil)
	//(a brazilian portugese character) in the release build (but not debug).
	//Perhaps _istcntrl() is unreliable in other cases as well.
	if (!iscntrl(key, locale::classic()) ||
		(key == '\n' && IsKeyDown(VK_RETURN)) ||
		(key == VK_BACK && IsKeyDown(VK_BACK)) ||
		(key == '\t' && IsKeyDown(VK_TAB)))
		switch (key) {
		case '\b' : {
			//pos is first char to delete
			long pos = HasSel() ? GetSelStart() : GetSelCaret() - 1;

			//if trying to backspace a (possibly selected) soft line break
			if ((GetSelCount() == 0 || GetSelCount() == 1) &&
				pos >= 0 && GetChar(pos) == '\r') {
				//pos is at the '\r'
				ATLASSERT(pos > 0);//'\r' can never be first character

				//delete the '\r' and the char before it
				if (pos > 0)
					ModifyText(pos - 1, 2, EmptyStr);
			} else {
				if (HasSel())
					DeleteSel();
				else {
					if (pos >= 0)
						DeleteChar(pos);
				}
			}
			break;
		}

		case VK_ESCAPE :
			break;
		default : {
			if (HasSel() && (key == '\t' || key == ' '))
				if (IndentBlock(key, (GetKeyModifiers() & kmShift) == 0))
					break;

			if (HasSel())
				DeleteSel();

			if (m_insert || key == '\n' || GetChar(GetSelCaret()) == '\n' || GetSelCaret() == GetTextLength()) {
				//if inserting (as opposed to overwriting)
				if (key == '\n' && GetAutoIndent()) {
					long offset, line;
					LineFromChar(offset, line, GetSelCaret());
					long caretOffset = GetSelCaret() - offset;//number of chars from beginning of line to caret

					CString space = '\n';
					if (GetSelCaret() > offset)
						space += GetPreSpace(line, caretOffset);

					// we delete trailing space from (what will be) previous line
					long spacesToDelete = GetNumSpacesBeforePos(line, caretOffset);

					ModifyText(GetSelCaret() - spacesToDelete, spacesToDelete, space);
				} else if (key == '\t' && m_tabInsertsSpaces)
					ModifyText(GetSelCaret(), 0, GetTabSpaceString());
				else
					InsertChar(GetSelCaret(), key);
			} else {
				//if overwriting
				CString keyStr = key;
				ReplaceTabsWithSpacesIfNecessary(keyStr);
				ModifyText(GetSelCaret(), 1, keyStr);
			}
		}
		}

	bHandled = FALSE;
	return 0;
}

LRESULT EditCtrl::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HideCaret();

	PAINTSTRUCT ps;
	CurDC dc(*this, BeginPaint(&ps));
	const CRect rcPaint = ps.rcPaint;

	CGdiObject* oldPen = dc.GetCurrentPen();
	CPen backPen(PS_SOLID, 1, GetBackColor());

	map<TokenKinds, auto_ptr<CPen> > forePenMap;
	for (long i = tkStart; i < tkEnd; ++i) {
		TokenKinds kind = (TokenKinds)i;
		forePenMap[kind] = auto_ptr<CPen>(new CPen(PS_SOLID, 1, GetColor(kind)));
	}

	bool hasFocus = HasFocus();

	long first = GetFirstVisibleLine() + rcPaint.top / GetLineHeight();
	first = Min(first, GetLineCount() - 1L);
	long last = GetFirstVisibleLine() + (rcPaint.bottom - 1) / GetLineHeight();
	last = Min(last, GetLineCount() - 1L);

	int y = (first - GetFirstVisibleLine()) * GetLineHeight();
	dc.SetWindowOrg(GetFirstVisiblePixel(), 0);

	for (i = first; i <= last; ++i) {
		long selStart = 0, selEnd = 0;
		GetLineSel(i, selStart, selEnd);

		if (selEnd - selStart < GetLineLength(i)) {//if at least part of the line isn't selected
			const CodeLine& codeLine = *GetCodeLine(i);
			int x = 0;
			for (long i = 0; i < codeLine.size(); ++i) {
				const Token& token = codeLine[i];

				dc.SetTextColor(GetColor(token.GetKind()));
				dc.SetBkColor(GetBackColor());

				//set pen color for showing whitespace
				dc.SelectObject(forePenMap[token.GetKind()].get());

				x += dc.DrawText(x, y, token.GetText());
			}
		}

		//draw selected text
		if (selEnd != selStart) {
			dc.SelectObject(&backPen);//set color for showing whitespace

			CString sel = GetLine(i).Mid(selStart, selEnd - selStart);
			int left = dc.GetLineWidth(i, selStart);
			int right = dc.GetLineWidth(i, selEnd);

			const COLORREF selColor = GetSelForeColor();
			const COLORREF selBackColor = GetSelBackColor();
			dc.SetTextColor(selColor);
			dc.SetBkColor(selBackColor);
			CRect rcSel(left, y, right, y + GetLineHeight());
			dc.FillSolidRect(rcSel, selBackColor);
			dc.DrawText(left, y, sel);
		}

		y += GetLineHeight();
	}

	dc.SelectObject(oldPen);

	EndPaint(&ps);
	ShowCaret();
	return 0;
}

LRESULT EditCtrl::OnGetText(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	long copyCount = Max((long)wParam - 1L, 0L);
	StrCpy((LPTSTR)lParam, GetText().Left(copyCount));
	return copyCount;
}

LRESULT EditCtrl::OnSetText(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	SetText((LPCTSTR)lParam);
	return 0;
}

LRESULT EditCtrl::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	CreateCaret();
	InvalidateSel();

	bHandled = FALSE;
	return 0;
}

LRESULT EditCtrl::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	DestroyCaret();
	InvalidateSel();

	bHandled = FALSE;
	return 0;
}

LRESULT EditCtrl::OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (IsKeyDown(VK_ESCAPE) && !HasSel())//get escape iff there is a selection
		return 0;//return 0 to NOT eat key
	if (GetReadOnly())
		return DLGC_WANTARROWS|DLGC_WANTCHARS;
	return DLGC_WANTALLKEYS|DLGC_WANTCHARS;
}

LRESULT EditCtrl::OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	long pos = CharFromPos(pt);
	SelectWord(pos);
	m_mouseDownFromSelectWord = true;
	return 0;
}

LRESULT EditCtrl::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	SetFocus();
	CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	long pos = CharFromPos(pt);

	if (wParam == (MK_CONTROL|MK_LBUTTON)) {
		SelectWord(pos);
		m_mouseDownFromSelectWord = true;
	} else if (wParam == (MK_SHIFT|MK_LBUTTON))
		IncreaseSel(pos - GetSelCaret());
	else if (wParam == MK_LBUTTON) {
		SetSelCaret(pos);
		m_mouseDownInControl = true;
	}

	VERIFY(SetTimer(ID_SCROLLSELECT, 100) == ID_SCROLLSELECT);
	return 0;
}

LRESULT EditCtrl::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	m_mouseDownFromSelectWord = false;
	m_mouseDownInControl = false;
	return 0;
}

LRESULT EditCtrl::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (IsKeyDown(VK_LBUTTON)) {
		// Checking m_mouseDownInControl prevents double clicking in the title bar or
		// open file dialog from causing this code to select text.
		// Checking m_mouseDownFromSelectWord prevents the selection code here from
		// interfering with double clicking.
		if (m_mouseDownInControl && !m_mouseDownFromSelectWord) {
			CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			IncreaseSel(CharFromPos(pt) - GetSelCaret());
		}
	} else
		m_mouseDownInControl = false;

	return 0;
}

LRESULT EditCtrl::OnCut(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	Cut();
	return 0;
}

LRESULT EditCtrl::OnCopy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	Copy();
	return 0;
}

LRESULT EditCtrl::OnPaste(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	Paste();
	return 0;
}

LRESULT EditCtrl::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	WORD pos = HIWORD(wParam);
	WORD code = LOWORD(wParam);
	if (code == SB_THUMBPOSITION || code == SB_THUMBTRACK)
		SetFirstVisibleLine(pos);
	else if (code == SB_LINEUP)
		SetFirstVisibleLine(GetFirstVisibleLine() - 1);
	else if (code == SB_LINEDOWN)
		SetFirstVisibleLine(GetFirstVisibleLine() + 1);
	else if (code == SB_PAGEUP)
		SetFirstVisibleLine(GetFirstVisibleLine() - GetVisibleLineCount(true));
	else if (code == SB_PAGEDOWN)
		SetFirstVisibleLine(GetFirstVisibleLine() + GetVisibleLineCount(true));


	return 0;
}

LRESULT EditCtrl::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	WORD pos = HIWORD(wParam);
	WORD code = LOWORD(wParam);
	if (code == SB_THUMBPOSITION || code == SB_THUMBTRACK)
		SetFirstVisiblePixel(pos);
	else if (code == SB_LINEUP)
		SetFirstVisiblePixel(GetFirstVisiblePixel() - GetHScrollAmount());
	else if (code == SB_LINEDOWN)
		SetFirstVisiblePixel(GetFirstVisiblePixel() + GetHScrollAmount());
	else if (code == SB_PAGEUP)
		SetFirstVisiblePixel(GetFirstVisiblePixel() - GetClientWidth());
	else if (code == SB_PAGEDOWN)
		SetFirstVisiblePixel(GetFirstVisiblePixel() + GetClientWidth());

	return 0;
}

LRESULT EditCtrl::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	//initialize numLines to a reasonable default value because SPI_GETWHEELSCROLLLINES
	//isn't supported on windows 95
	UINT numLines = 3;
	SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &numLines, 0);

	//-1 means the scroll one page at a time system setting is enabled
	if (numLines == -1)
		numLines = GetVisibleLineCount(true);

	short delta = HIWORD(wParam);
	long deltaLines = Div(numLines * delta, WHEEL_DELTA, rkOff);
	SetFirstVisibleLine(GetFirstVisibleLine() - deltaLines);
	return 0;
}

LRESULT EditCtrl::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	WORD code = LOWORD(lParam);

	if (code == HTCLIENT) {
		SetCursor(LoadCursor(NULL, IDC_IBEAM));
		return TRUE;
	}
	
	bHandled = FALSE;
	return 0;
}

LRESULT EditCtrl::OnEraseBkGnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	RECT rc;
	GetClientRect(&rc);
	HDC dc = (HDC)wParam;
	CBrush brush;
	brush.CreateSolidBrush(GetBackColor());
	FillRect(dc, &rc, brush);
	return 1;
}

LRESULT EditCtrl::OnEnable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	Invalidate();
	bHandled = FALSE;
	return 0;
}

//overridden to make bookmark gutter part of the non-client area
LRESULT EditCtrl::OnNcCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	LRESULT result = DefWindowProc();

	RECT& rc = wParam ? ((NCCALCSIZE_PARAMS*)lParam)->rgrc[0] : *((RECT*)lParam);
	rc.left = Min(rc.left + m_gutterWidth, rc.right);

	return result;
}

//overridden to draw bookmarks
LRESULT EditCtrl::OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	LRESULT result = DefWindowProc();

	if (m_gutterWidth == 0)
		return result;

    CDC dc;
	dc.Attach(GetWindowDC());

	CFont* oldFont = dc.SelectObject(CFont::FromHandle(GetFont()));

	CPen pen;
	pen.CreatePen(PS_SOLID, 1, RGB(0x0, 0x0, 0x0));
	CPen* oldPen = dc.SelectObject(&pen);
	CBrush brush;
	brush.CreateSolidBrush(COLOR_BOOKMARK);
	CBrush* oldBrush = dc.SelectObject(&brush);

	dc.SetTextColor(COLOR_LINENUMBER);
	dc.SetBkMode(TRANSPARENT);

	CRect rc = GetGutterRect();
	CRect space = rc;
	space.left = space.right - SPACE_BETWEEN_GUTTER_AND_CLIENT_AREA;
	rc.right -= SPACE_BETWEEN_GUTTER_AND_CLIENT_AREA;
	dc.FillSolidRect(&space, GetBackColor());
	dc.FillSolidRect(&rc, COLOR_GUTTER);
	rc.bottom = rc.top + GetLineHeight();
	rc.DeflateRect(0, 1);

	CRect rcBookmark = rc;
	rcBookmark.left = rcBookmark.right - BOOKMARK_WIDTH;
	CPoint pt(5, 5);

	long last = GetLastVisibleLine(false);
	CString lineNum;
	for (long i = GetFirstVisibleLine(); i <= last; ++i) {
		if (m_showLineNumbers && !IsWrappedLine(i)) {
			lineNum.Format(_T("%d"), GetLfLineIndex(i) + 1);
			dc.DrawText(lineNum, rc, DT_LEFT|DT_NOCLIP);
		}

		if (GetLineData(i)->m_hasBookmark)
			VERIFY(dc.RoundRect(rcBookmark, pt));

		rc.OffsetRect(0, GetLineHeight());
		rcBookmark.OffsetRect(0, GetLineHeight());
	}

	dc.SelectObject(oldPen);
	dc.SelectObject(oldBrush);
	dc.SelectObject(oldFont);

    ReleaseDC(dc.Detach());

	return result;
}

LRESULT EditCtrl::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (m_contextMenu == NULL)
		return 0;

	CPoint pt(lParam);

	if (pt.x == -1 && pt.y == -1) {//if context menu was invoked with keyboard
		pt = PosFromChar(GetSelCaret());
		ClientToScreen(&pt);
		pt.y += GetLineHeight();
	} else if (!HasSel()) {
		//move caret to where the user right-clicked if there is no selection
		//to make cut/copy of the currnet line behave more intuitively
		CPoint clientPt = pt;
		VERIFY(ScreenToClient(&clientPt));
		SetSelCaret(CharFromPos(clientPt));
	}

	//Make sure popup menu doesn't cover the line with the caret.
	//This would happen if you opened the menu towards the bottom of
	//the screen and it had to show above the caret.
	TPMPARAMS tmpParams;
	InitSized(tmpParams);
	//just use whole width of screen to make sure menu doesn't appear to left or right
	tmpParams.rcExclude = GetMonitorRect(m_hWnd);
	tmpParams.rcExclude.top = pt.y - GetLineHeight();
	tmpParams.rcExclude.bottom = pt.y;

	// Make sure caret is always visible when context menu is showing since some items
	// act on current line.
	ForceShowCaret();

	VERIFY(TrackPopupMenuEx(m_contextMenu, 0, pt.x, pt.y, GetParent(), &tmpParams));

	return 0;
}

LRESULT EditCtrl::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (IsWindowVisible()) {
		UpdateVScroll();
		UpdateHScroll();
	}

	return 0;
}

LRESULT EditCtrl::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	UINT id = (UINT)wParam;
	if (id == ID_SCROLLSELECT)
		if (IsAsyncKeyDown(VK_LBUTTON)) {
			CPoint pt;
			GetCursorPos(&pt);
			ScreenToClient(&pt);
			CRect rc;
			GetClientRect(&rc);

			if (!rc.PtInRect(pt))
				IncreaseSel(CharFromPos(pt) - GetSelCaret());
		} else
			VERIFY(KillTimer(ID_SCROLLSELECT));

	return 0;
}

#if 0
LRESULT EditCtrl::On(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	return 0;
}

#endif

