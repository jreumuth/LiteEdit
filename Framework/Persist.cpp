#include "StdAfx.h"
#include "Persist.h"
#include "Utility.h"

const CString CommentStart = _T(";;");

void Persist::PrePersist(const CString& name) {
	if (IsLoading()) {
		SkipEmptyLinesAndComments();

		//make sure the correct field exists
		if (FindSubstring(mData, name + '=', ffMatchCase, mOffset) != mOffset) {
			CString msg;
			msg.Format(TEXT("Could not find field '%s' when loading file '%s'. The file may be corrupted. You should either replace the file with a valid copy or delete the file from disk."), name, mFileName);
			throw Exception(ErrPersistedDataNotFound, msg);
		}

		mOffset += name.GetLength() + 1;//+1 for =
	} else {
		mData += name;
		mData += '=';
	}
}

void Persist::PostPersist() {
	if (IsLoading())
		SkipLine(); // move past data we just read
	else
		PersistNewLine();
}


void Persist::SkipEmptyLinesAndComments() {
	ASSERT(IsLoading());

	while (mOffset < mData.GetLength()) {
		TCHAR ch = mData.GetAt(mOffset);

		if (ch == '\n')
			++mOffset;
		else if (ch == CommentStart[0] && // optimization
			     mData.Mid(mOffset, CommentStart.GetLength()) == CommentStart)
			SkipLine();
		else
			break;
	}
}

void Persist::SkipLine() {
	ASSERT(IsLoading());

	long end = mData.Find('\n', mOffset);
	if (end < 0)
		mOffset = mData.GetLength();
	else
		mOffset = end + 1;
}

Persist::Persist(const CString& fileName, bool loading, bool reloading) : mFileName(fileName), mLoading(loading), mReloading(reloading), mOffset(0), mVersion(CurPersistVer) {
	// if reloading is true, then loading must be true also
	ASSERT(!reloading || loading);

	//can't load if the file doesn't exist
	ASSERT(!IsLoading() || FileExists(fileName));

	if (IsLoading())
		mData = FileToString(mFileName);

	PersistEnum(TEXT("Version"), mVersion);

	if (loading && mVersion > CurPersistVer)
		throw Exception(ErrNewerFileVersion);

	PersistNewLine();
}

Persist::~Persist() {
	if (!IsLoading()) {
		//convert to CRLF so it can be read by normal windows text editors
		CString fileData = ConvertLFtoCRLF(mData);
		StringToFile(mFileName, fileData);
	}
}

void Persist::AddComment(const CString comment, bool addNewLineBefore, bool addNewLineAfter) {
	if (!IsLoading()) {
		if (addNewLineBefore)
			mData += '\n';

		StringVector wrappedComments;
		WrapLines(comment, wrappedComments, 70);

		const CString myCommentStart = CommentStart + ' ';

		CString commentString =
			myCommentStart +
			wrappedComments.Concat('\n' + myCommentStart, true) +
			'\n';

		mData += commentString;

		if (addNewLineAfter)
			mData += '\n';
	}
}

//puts a new line in persistence file to make it more readable
void Persist::PersistNewLine() {
	// when loading, SkipEmptyLinesAndComments() skips past the new lines
	if (!IsLoading())
		mData += '\n';
}

void Persist::PersistBool(const CString& name, bool& val) {
	CString stringVal = val ? TEXT("True") : TEXT("False");
	PersistString(name, stringVal);
	val = StrCmpI(stringVal, TEXT("False")) != 0;
}

void Persist::PersistNumber(const CString& name, int& val) {
	PrePersist(name);

	if (IsLoading()) {
		_stscanf(GetReadPtr(), TEXT("%d"), &val);
	} else {
		CString temp;
		temp.Format(TEXT("%d"), val);
		WriteData(temp);
	}

	PostPersist();
}

void Persist::PersistString(const CString& name, CString& val) {
	PrePersist(name);

	if (IsLoading()) {
		val = EmptyStr;
		long offset = 0;
		while (true) {
			char ch = NULL;
			_stscanf(GetReadPtr() + offset, _T("%c"), &ch);
			if (ch == NULL || ch == '\n')
				break;

			val += ch;
			++offset;
		}
	} else {
		//Can't persist a string with new lines in it. That will
		//need to be persisted a several strings.
		ASSERT(val.Find('\n') < 0);
		WriteData(val);
	}

	PostPersist();
}

void Persist::PersistVal(const CString& name, LOGFONT& lf) {
	PersistNumber(name + TEXT("Height"), lf.lfHeight);
	PersistNumber(name + TEXT("Width"), lf.lfWidth);
	PersistNumber(name + TEXT("Escapement"), lf.lfEscapement);
	PersistNumber(name + TEXT("Orientation"), lf.lfOrientation);
	PersistNumber(name + TEXT("Weight"), lf.lfWeight);
	PersistBool(name + TEXT("Italic"), lf.lfItalic);
	PersistBool(name + TEXT("Underline"), lf.lfUnderline);
	PersistBool(name + TEXT("StrikeOut"), lf.lfStrikeOut);
	PersistNumber(name + TEXT("CharSet"), lf.lfCharSet);
	PersistNumber(name + TEXT("OutPrecision"), lf.lfOutPrecision);
	PersistNumber(name + TEXT("ClipPrecision"), lf.lfClipPrecision);
	PersistNumber(name + TEXT("Quality"), lf.lfQuality);
	PersistNumber(name + TEXT("PitchAndFamily"), lf.lfPitchAndFamily);
	CString faceName = lf.lfFaceName;
	PersistString(name + TEXT("FaceName"), faceName);
	StrCpy(lf.lfFaceName, faceName);
}

void Persist::PersistVal(const CString& name, HFONT& hFont) {
	LOGFONT lf;
	MemClear(lf);

	if (IsLoading()) {
		PersistVal(name, lf);
		hFont = CreateFontIndirect(&lf);
	} else {
		//convert hFont to LOGFONT
		VERIFY(::GetObject(hFont, sizeof(lf), &lf) == sizeof(lf));
		PersistVal(name, lf);
	}
}

void Persist::PersistVal(const CString& name, RECT& rc) {
	PersistNumber(name + TEXT("Top"), rc.top);
	PersistNumber(name + TEXT("Left"), rc.left);
	PersistNumber(name + TEXT("Bottom"), rc.bottom);
	PersistNumber(name + TEXT("Right"), rc.right);
}
