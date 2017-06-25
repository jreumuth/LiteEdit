#include "StdAfx.h"
#include "CodeColoring.h"
#include "Persist.h"

CString TokenToString(TokenKinds kind, TokenStrOptions options/* = tsoNone*/) {
	bool separateWords = (options & tsoSeparateWords) != 0;

	switch (kind) {
	case tkPlainText :
		return separateWords ? TEXT("Plain Text") : TEXT("PlainText");
	case tkKeyword :
		return TEXT("Keyword");
	case tkSymbol :
		return TEXT("Symbol");
	case tkString :
		return TEXT("String");
	case tkComment :
		return TEXT("Comment");
	case tkBackground :
		return TEXT("Background");
	default :
		ASSERT(false);
		return EmptyStr;
	}
}

TokenColors::TokenColors() {
#if 0
	//windows looking default colors
	m_colors[tkPlainText] = RGB(0x0, 0x0, 0x0);
	m_colors[tkBackground] = RGB(0xFF, 0xFF, 0xFF);
	m_colors[tkComment] = RGB(0x0, 0x80, 0x0);
	m_colors[tkSymbol] = RGB(0x80, 0x00, 0x80);
	m_colors[tkKeyword] = RGB(0x0, 0x0, 0xFF);
	m_colors[tkString] = RGB(0x60, 0x60, 0xC0);

	//pac man colors
	m_colors[tkPlainText] = RGB(255, 255, 0);
	m_colors[tkBackground] = RGB(0, 0, 172);
	m_colors[tkComment] = RGB(0, 255, 255);
	m_colors[tkSymbol] = RGB(255, 255, 255);
	m_colors[tkKeyword] = RGB(255, 255, 255);
	m_colors[tkString] = RGB(64, 255, 64);
#endif

	//white background and black text
	MemSet(m_colors, 0);
	m_colors[tkBackground] = RGB(255, 255, 255);
}

void TokenColors::DoPersist(Persist& p) {
	for (long t = tkStart; t < tkEnd; ++t)
		p.PersistNumber(TokenToString((TokenKinds)t) + TEXT("Color"), Item((TokenKinds)t));
}

void LangElem::Save(Persist& p) {
	p.PersistEnum(TEXT("TokenKind"), m_kind);
	p.PersistNumber(TEXT("Options"), m_options);	
}

void Words::Save(Persist& p) {
	LangElem::Save(p);
	p.PersistValVector(TEXT("Words"), m_words);
}

LangElemPtr Words::Load(Persist& p) {
	TokenKinds kind = tkPlainText;
	LangElemOptions options = NoOptions;
	StringVector words;

	p.PersistEnum(TEXT("TokenKind"), kind);
	p.PersistNumber(TEXT("Options"), options);	
	p.PersistValVector(TEXT("Words"), words);

	WordsPtr result = new Words(kind, options);
	for (long i = 0; i < words.size(); ++i)
		result->AddWord(words[i]);

	return result;
}

long RestOfLine::Parse(Context& context, const CString& string, long start, bool matchCase) const {
	context.mWrappedLangElem = this;
	return string.GetLength();
}

void RestOfLine::Save(Persist& p) {
	LangElem::Save(p);
	p.PersistString(TEXT("Marker"), m_marker);
}

LangElemPtr RestOfLine::Load(Persist& p) {
	TokenKinds kind = tkPlainText;
	LangElemOptions options = NoOptions;
	CString marker;

	p.PersistEnum(TEXT("TokenKind"), kind);
	p.PersistNumber(TEXT("Options"), options);	
	p.PersistString(TEXT("Marker"), marker);
	
	return new RestOfLine(kind, options, marker);
}

Range::Range(TokenKinds kind, LangElemOptions options, const CString& startMarker, const CString& endMarker, const CString& escapeChar/* = EmptyStr*/) :
	LangElem(kind, options),
	m_startMarker(startMarker), m_endMarker(endMarker), m_escapeChar(escapeChar),
	m_nested((options & Nested) != 0), m_spanLines((options & SpanLines) != 0) {

	//I can't nest if m_starMarker == m_endMarker.
	//Technically, I should pass the matchCase option from the Language to
	//StrCmpEx(), but it's not worth getting that data for this assert.
	ATLASSERT(!GetNested() || StrCmpEx(m_startMarker, m_endMarker, false) != 0);

	if (GetWholeWord()) {
		typedef WordList<RangeMarkers> RangeParser;
		m_finder = auto_ptr<Finder<RangeMarkers> >(new RangeParser());
	} else {
		typedef ParserTree<RangeMarkers> RangeParser;
		m_finder = auto_ptr<Finder<RangeMarkers> >(new RangeParser());

		//escape chars are not supported if using words
		if (HasEscapeChar()) {
			m_finder->AddString(GetEscapedEscape(), rmEscapedEscape);
			m_finder->AddString(GetEscapedEnd(), rmEscapedEnd);
		}
	}

	m_finder->AddString(m_endMarker, rmEnd);
	if (GetNested())
		m_finder->AddString(m_startMarker, rmStart);
}

long Range::Parse(Context& context, const CString& string, long start, bool matchCase) const {
	if (context.mRangeStack.empty()) {
		//if we're not parsing from a wrapped line, then the string begins with the
		//start marker which we want to skip
		if (context.mWrappedLangElem == NULL)
			start += m_startMarker.GetLength();

		//mWrappedLangElem will get set again if I don't find the end token
		context.mWrappedLangElem = NULL;

		context.mRangeStack.push(this);
	}

	const long length = string.GetLength();
	long pos = start;

	while (pos < length) {
		long symbolStart, symbolEnd;

		RangeMarkers rm;
		if (!m_finder->FindElement(string, pos, matchCase, rm, symbolStart, symbolEnd)) {
			pos = string.GetLength();
			break;
		}

		switch (rm) {
		case rmNone :
		case rmEscapedEscape :
		case rmEscapedEnd :
			break;
		case rmStart :
			context.mRangeStack.push(this);
			break;
		case rmEnd :
			ATLASSERT(!context.mRangeStack.empty());
			context.mRangeStack.pop();
			break;
		default :
			ATLASSERT(false);
		}
		
		pos = symbolEnd;

		if (context.mRangeStack.empty())
			break;
	}

	if (!GetSpanLines() && !context.mRangeStack.empty()) {
		context.mWrappedLangElem = this;

		while (!context.mRangeStack.empty())
			context.mRangeStack.pop();
	}
		
	return pos;
}

void Range::Save(Persist& p) {
	LangElem::Save(p);
	p.PersistString(TEXT("StartMarker"), CString(m_startMarker));
	p.PersistString(TEXT("EndMarker"), CString(m_endMarker));
	p.PersistString(TEXT("EscapeChar"), CString(m_escapeChar));
}

LangElemPtr Range::Load(Persist& p) {
	TokenKinds kind = tkPlainText;
	LangElemOptions options = NoOptions;
	CString startMarker, endMarker, escapeChar;

	p.PersistEnum(TEXT("TokenKind"), kind);
	p.PersistNumber(TEXT("Options"), options);	
	p.PersistString(TEXT("StartMarker"), startMarker);
	p.PersistString(TEXT("EndMarker"), endMarker);

	if (p.IsVersAtLeast(PersistVers8))
		p.PersistString(TEXT("EscapeChar"), escapeChar);
	else {
		//If it's an old language file that uses a false end,
		//try to convert that to an escape char. A false end is
		//a superset of the end marker that isn't the end of a
		//string, e.g. \" for an end marker of ". I replaced the
		//idea of a false end with an escape char because a
		//false end could be a false, false end. For example, if
		//a string contains \\" then the " should be the end
		//marker. The escape char concept works in this case.
		CString falseEnd;
		p.PersistString(TEXT("FalseEnd"), falseEnd);

		if (!falseEnd.IsEmpty())
			if (EndsWith(falseEnd, endMarker)) {
				escapeChar = falseEnd.Left(falseEnd.GetLength() - endMarker.GetLength());

				//can't use escape char that's the same as the end marker
				if (escapeChar == endMarker) {
					ATLASSERT(escapeChar == startMarker);//false end wasn't needed
					escapeChar = EmptyStr;
				}
			} else
				ATLASSERT(false);//can't convert false end to escape char
	}

	return new Range(kind, options, startMarker, endMarker, escapeChar);
}

//Returns position into string e1 and e2 were parsed from where the first difference occurs.
//A difference is defined is a token being a different kind, having a different starting
//position, or being composed of different text.
long CodeLine::FindFirstDiff(const CodeLine& e1, const CodeLine& e2) {
	long size = Min(e1.size(), e2.size());
	for (long i = 0; i < size; ++i) {
		const Token& t1 = e1[i];
		const Token& t2 = e2[i];
		if (t1.GetStart() != t2.GetStart())
			return Min(t1.GetStart(), t2.GetStart());

		if (t1.GetKind() != t2.GetKind())
			return t1.GetStart();

		long diff = ::FindFirstDiff(t1.GetText(), t2.GetText());
		if (diff != -1)//if the the strings are different
			return t1.GetStart() + diff;
	}

	if (e1.size() > size)
		return e1[size].GetStart();
	else if (e2.size() > size)
		return e2[size].GetStart();
	return -1;
}

void Language::Clear() {
	m_symbolTree.Clear();
	m_identList.Clear();
	m_langElems.clear();
}

void Language::AddLangElem(const LangElemPtr& langElem) {
	if (langElem->GetWholeWord())
		for (long i = 0; i < langElem->GetNumStrings(); ++i)
			m_identList.AddString(langElem->GetNthString(i), langElem);
	else
		for (long i = 0; i < langElem->GetNumStrings(); ++i)
			m_symbolTree.AddString(langElem->GetNthString(i), langElem);

	m_langElems.push_back(langElem);
}

CodeLinePtr Language::NewCodeLine(const CString& string, const Context& startingContext, bool doesLineWrap) const {
	//possible optimization: passing this to CodeLine made paste a little faster
	//in some cases, but not sure it doesn't make other cases slower
	//long maxPossibleNumTokens = GetNumTokenCount(string);

	CodeLinePtr codeLine = new CodeLine(startingContext);
	long pos = 0;
	const long length = string.GetLength();
	CString token;

	while (pos < length) {
		if (codeLine->m_endingContext.HasContext()) {
			LangElemConstPtr langElem = codeLine->m_endingContext.GetParsingLangElem();
			long end = langElem->Parse(codeLine->m_endingContext, string, pos, GetMatchCase());
			codeLine->push_back(Token(langElem->GetTokenKind(), pos, string.Mid(pos, end - pos)));
			pos = end;
		} else {
			StringToken st = GetNextToken(string, pos, token);

			switch (st) {
			case stWhiteSpace :
				codeLine->push_back(Token(tkPlainText, pos, token));
				pos += token.GetLength();
				break;

			case stIdentifier : {
				LangElemPtr langElem;
				if (m_identList.GetElement(token, GetMatchCase(), langElem)) {
					ATLASSERT(langElem != NULL);

					long end = langElem->Parse(codeLine->m_endingContext, string, pos, GetMatchCase());
					if (end > pos) {
						codeLine->push_back(Token(langElem->GetTokenKind(), pos, string.Mid(pos, end - pos)));
						pos = end;
						break;
					}
				}

				TokenKinds kind = langElem == NULL ? tkPlainText : langElem->GetTokenKind();
				codeLine->push_back(Token(kind, pos, token));
				pos += token.GetLength();
				break;
			}

			case stSymbol : {
				long symbolStart = 0, symbolEnd = 0;
				LangElemPtr langElem;

				if (m_symbolTree.FindElement(token, 0, GetMatchCase(), langElem, symbolStart, symbolEnd)) {
					ATLASSERT(langElem != NULL);

					//if there is some text before the symbol
					if (symbolStart > 0)
						codeLine->push_back(Token(tkPlainText, pos, token.Left(symbolStart)));

					//put the symbol in codeLine
					pos += symbolStart;
					long end = langElem->Parse(codeLine->m_endingContext, string, pos, GetMatchCase());
					if (end > pos) {
						codeLine->push_back(Token(langElem->GetTokenKind(), pos, string.Mid(pos, end - pos)));
						pos = end;
					} else {
						codeLine->push_back(Token(langElem->GetTokenKind(), pos, token.Mid(symbolStart, symbolEnd - symbolStart)));
						pos += symbolEnd - symbolStart;
					}
				} else {//can't find a symbol inside token
					codeLine->push_back(Token(tkPlainText, pos, token));
					pos += token.GetLength();
				}

				break;
			}

			default :
				ATLASSERT(false);
			}
		}
	}

	//clear the mWrappedLangElem if the line doesn't wrapped
	if (!doesLineWrap)
		codeLine->m_endingContext.mWrappedLangElem = NULL;

	return codeLine;
}

void Language::DoPersist(Persist& p) {
	long count = m_langElems.size();

	if (p.IsVersBefore(PersistVers10))
		p.PersistString(TEXT("LanguageName"), CString());

	p.PersistValVector(TEXT("Extensions"), m_extensions);
	p.PersistBool(TEXT("MatchCase"), m_matchCase);
	p.PersistNewLine();
	p.PersistNumber(TEXT("LangElemCount"), count);
	p.PersistNewLine();

	if (p.IsLoading()) {
		Clear();

		m_fileDir = GetDir(p.GetFilePath());
		m_name = GetFileNameNoExt(p.GetFilePath());

		for (long i = 0; i < count; ++i) {
			LangElemKinds kind = leWords;
			p.PersistEnum(TEXT("LangElemKind"), kind);
			LangElemPtr langElem;
			switch (kind) {
			case leWords :
				langElem = Words::Load(p);
				break;
			case leRestOfLine :
				langElem = RestOfLine::Load(p);
				break;
			case leRange :
				langElem = Range::Load(p);
				break;
			default :
				ASSERT(false);
			}

			AddLangElem(langElem);
			p.PersistNewLine();
		}
	} else {
		for (long i = 0; i < count; ++i) {
			LangElemKinds kind = m_langElems[i]->GetLangElemKind();
			p.PersistEnum(TEXT("LangElemKind"), kind);
			m_langElems[i]->Save(p);
			p.PersistNewLine();
		}
	}
}

CLanguage::CLanguage() : Language(TEXT("C++"), true) {
	AddExtension(TEXT("h"));
	AddExtension(TEXT("c"));
	AddExtension(TEXT("cpp"));

	WordsPtr keywords = new Words(tkKeyword, WholeWord);
	keywords->AddWord(TEXT("break"));
	keywords->AddWord(TEXT("case"));
	keywords->AddWord(TEXT("char"));
	keywords->AddWord(TEXT("const"));
	keywords->AddWord(TEXT("continue"));
	keywords->AddWord(TEXT("default"));
	keywords->AddWord(TEXT("do"));
	keywords->AddWord(TEXT("double"));
	keywords->AddWord(TEXT("else"));
	keywords->AddWord(TEXT("enum"));
	keywords->AddWord(TEXT("extern"));
	keywords->AddWord(TEXT("float"));
	keywords->AddWord(TEXT("for"));
	keywords->AddWord(TEXT("goto"));
	keywords->AddWord(TEXT("if"));
	keywords->AddWord(TEXT("int"));
	keywords->AddWord(TEXT("long"));
	keywords->AddWord(TEXT("return"));
	keywords->AddWord(TEXT("short"));
	keywords->AddWord(TEXT("signed"));
	keywords->AddWord(TEXT("sizeof"));
	keywords->AddWord(TEXT("static"));
	keywords->AddWord(TEXT("struct"));
	keywords->AddWord(TEXT("switch"));
	keywords->AddWord(TEXT("typedef"));
	keywords->AddWord(TEXT("union"));
	keywords->AddWord(TEXT("unsigned"));
	keywords->AddWord(TEXT("void"));
	keywords->AddWord(TEXT("volatile"));
	keywords->AddWord(TEXT("while"));
	keywords->AddWord(TEXT("class"));
	keywords->AddWord(TEXT("public"));
	keywords->AddWord(TEXT("private"));
	keywords->AddWord(TEXT("protected"));
	keywords->AddWord(TEXT("virtual"));
	keywords->AddWord(TEXT("new"));
	keywords->AddWord(TEXT("delete"));
	keywords->AddWord(TEXT("bool"));
	keywords->AddWord(TEXT("true"));
	keywords->AddWord(TEXT("false"));
	AddLangElem(LangElemPtr(keywords));
	
	WordsPtr symbols = new Words(tkSymbol, NoOptions);
	symbols->AddWord(TEXT("("));
	symbols->AddWord(TEXT(")"));
	symbols->AddWord(TEXT("~"));
	symbols->AddWord(TEXT("!"));
	symbols->AddWord(TEXT("%"));
	symbols->AddWord(TEXT("^"));
	symbols->AddWord(TEXT("&"));
	symbols->AddWord(TEXT("|"));
	symbols->AddWord(TEXT("*"));
	symbols->AddWord(TEXT("-"));
	symbols->AddWord(TEXT("="));
	symbols->AddWord(TEXT("+"));
	symbols->AddWord(TEXT("/"));
	symbols->AddWord(TEXT("?"));
	symbols->AddWord(TEXT(":"));
	symbols->AddWord(TEXT("=="));
	symbols->AddWord(TEXT("+="));
	symbols->AddWord(TEXT("-+"));
	symbols->AddWord(TEXT("*="));
	symbols->AddWord(TEXT("/="));
	symbols->AddWord(TEXT("&="));
	symbols->AddWord(TEXT("|="));
	symbols->AddWord(TEXT("!="));
	symbols->AddWord(TEXT("||"));
	symbols->AddWord(TEXT("&&"));
	symbols->AddWord(TEXT("++"));
	symbols->AddWord(TEXT("--"));
	symbols->AddWord(TEXT("::"));
	symbols->AddWord(TEXT("."));
	symbols->AddWord(TEXT(","));
	symbols->AddWord(TEXT(";"));
	symbols->AddWord(TEXT("{"));
	symbols->AddWord(TEXT("}"));
	symbols->AddWord(TEXT("["));
	symbols->AddWord(TEXT("]"));
	symbols->AddWord(TEXT("<"));
	symbols->AddWord(TEXT(">"));
	symbols->AddWord(TEXT("<="));
	symbols->AddWord(TEXT(">="));
	symbols->AddWord(TEXT("<<"));
	symbols->AddWord(TEXT(">>"));
	AddLangElem(LangElemPtr(symbols));
	
	RestOfLinePtr singlelineComment = new RestOfLine(tkComment, NoOptions, TEXT("//"));
	AddLangElem(LangElemPtr(singlelineComment));
	
	RestOfLinePtr preProcessor = new RestOfLine(tkKeyword, NoOptions, TEXT("#"));
	AddLangElem(LangElemPtr(preProcessor));
	
	RangePtr string = new Range(tkString, NoOptions, TEXT("\""), TEXT("\""), TEXT("\\\""));
	AddLangElem(LangElemPtr(string));
	
	RangePtr ch = new Range(tkString, NoOptions, TEXT("'"), TEXT("'"), TEXT("\\\'"));
	AddLangElem(LangElemPtr(ch));
	
	RangePtr multilineComment = new Range(tkComment, SpanLines, TEXT("/*"), TEXT("*/"));
	AddLangElem(LangElemPtr(multilineComment));
}

HtmlLanguage::HtmlLanguage() : Language(TEXT("Html"), false) {
	AddExtension(TEXT("htm"));
	AddExtension(TEXT("html"));

	RangePtr comment = new Range(tkComment, SpanLines, TEXT("<!--"), TEXT("-->"));
	AddLangElem(LangElemPtr(comment));

	WordsPtr symbols = new Words(tkSymbol, NoOptions);
	symbols->AddWord(TEXT("<"));
	symbols->AddWord(TEXT(">"));
	symbols->AddWord(TEXT("</"));
	AddLangElem(LangElemPtr(symbols));
}