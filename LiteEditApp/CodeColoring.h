#ifndef CodeColoringH
#define CodeColoringH

#include "Utility.h"
#include "Object.h"
#include "PathMgr.h"
#include <stack>
#include <map>

enum TokenKinds {tkPlainText, tkComment, tkKeyword, tkSymbol, tkString, tkBackground};
const long tkStart = tkPlainText, tkEnd = tkBackground + 1;

typedef long TokenStrOptions;
const TokenStrOptions tsoNone = 0x0;
const TokenStrOptions tsoSeparateWords = 0x1;

CString TokenToString(TokenKinds kind, TokenStrOptions options = tsoNone);

class Persist;

//class that stores a color for every token kind
class TokenColors : public Obj {
private:
	COLORREF m_colors[tkEnd - tkStart];

public:
	TokenColors();
	COLORREF& operator[](TokenKinds kind) {return Item(kind);}
	const COLORREF& operator[](TokenKinds kind) const {return Item(kind);}

	COLORREF& Item(TokenKinds kind) {
		ATLASSERT(tkStart <= kind && kind < tkEnd);
		return m_colors[kind - tkStart];
	}

	const COLORREF& Item(TokenKinds kind) const {
		return const_cast<TokenColors*>(this)->Item(kind);
	}

	virtual void DoPersist(Persist& p);
};
PTR_DEF(TokenColors);

typedef long LangElemOptions;
const LangElemOptions NoOptions = 0x0;
const LangElemOptions WholeWord = 0x1;//only identifiers can be WholeWords
const LangElemOptions SpanLines = 0x2;//applies only to Ranges
const LangElemOptions Nested = 0x4;//applies only to Ranges

class Context;

enum LangElemKinds {leWords, leRestOfLine, leRange};

//base class for language elements, e.g. comments, keywords, strings
class LangElem : public Obj, public StringIterator {
private:
	TokenKinds m_kind;
	LangElemOptions m_options;

public:
	LangElem(TokenKinds kind, LangElemOptions options) : m_kind(kind), m_options(options) {}

	TokenKinds GetTokenKind() const {return m_kind;}
	LangElemOptions GetOptions() const {return m_options;}
	bool GetWholeWord() const {return (m_options & WholeWord) != 0;}
	//Parse gives a LangElem a chance to do some extra parsing of its own
	virtual long Parse(Context& context, const CString& string, long start, bool matchCase) const {return start;}
	virtual LangElemKinds GetLangElemKind() const = 0;

	virtual void Save(Persist& p);
};
PTR_DEF(LangElem);

//For stuff like keyword and symbols. If the WholeWord LangElemOption is specified,
//Words probably contains keywords and it probably contains symbols otherwise.
class Words : public LangElem {
private:
	StringVector m_words;

public:
	Words(TokenKinds kind, LangElemOptions options) :
	  LangElem(kind, options) {}

	void AddWord(const CString& word) {m_words.push_back(word);}
	virtual LangElemKinds GetLangElemKind() const {return leWords;}
	virtual long GetNumStrings() const {return m_words.size();}

	virtual const CString& GetNthString(long i) const {
		ATLASSERT(0 <= i && i < GetNumStrings());
		return m_words[i];
	}

	virtual void Save(Persist& p);
	static LangElemPtr Load(Persist& p);
};
PTR_DEF(Words);

//for stuff that applies to the rest of the line, e.g. single line comment
class RestOfLine : public LangElem {
private:
	CString m_marker;

public:
	RestOfLine(TokenKinds kind, LangElemOptions options, const CString& marker) :
	  LangElem(kind, options), m_marker(marker) {}

	CString GetMarker() const {return m_marker;}
	virtual LangElemKinds GetLangElemKind() const {return leRestOfLine;}
	virtual long GetNumStrings() const {return 1;}

	virtual const CString& GetNthString(long i) const {
		ATLASSERT(0 <= i && i < GetNumStrings());
		return m_marker;
	}

	virtual long Parse(Context& context, const CString& string, long start, bool matchCase) const;

	virtual void Save(Persist& p);
	static LangElemPtr Load(Persist& p);
};
PTR_DEF(RestOfLine);

template<class T>
class Finder {
public:
	virtual ~Finder() {}
	virtual void Clear() = 0;
	virtual void AddString(const CString& string, const T& data) = 0;
	virtual bool FindElement(const CString& string, long start, bool matchCase, T& data, long& elemStart, long& elemEnd) const = 0;
	virtual bool GetElement(const CString& elem, bool matchCase, T& data) const = 0;
};

//For stuff that has a starting and ending marker, e.g. strings and multiline comments.
//If m_startMarker == m_endMarker, then range cannot be nested (it's logically impossible).
class Range : public LangElem {
private:
	const CString m_startMarker, m_endMarker, m_escapeChar;
	const bool m_nested;
	const bool m_spanLines;

	enum RangeMarkers {rmNone = 0, rmEscapedEscape, rmEscapedEnd, rmStart, rmEnd};
	auto_ptr<Finder<RangeMarkers> > m_finder;

public:
	Range(TokenKinds kind, LangElemOptions options, const CString& startMarker, const CString& endMarker, const CString& escapeChar = EmptyStr);

	const CString& GetStartMarker() const {return m_startMarker;}
	const CString& GetEndMarker() const {return m_endMarker;}
	const CString& GetEscapeChar() const {return m_escapeChar;}
	bool HasEscapeChar() const {return !m_escapeChar.IsEmpty();}
	CString GetEscapedEscape() const {return m_escapeChar + m_escapeChar;}
	CString GetEscapedEnd() const {return m_escapeChar + m_endMarker;}
	bool GetNested() const {return m_nested;}
	bool GetSpanLines() const {return m_spanLines;}
	virtual LangElemKinds GetLangElemKind() const {return leRange;}
	virtual long GetNumStrings() const {return 1;}

	virtual const CString& GetNthString(long i) const {
		ATLASSERT(0 <= i && i < GetNumStrings());
		return m_startMarker;
	}

	virtual long Parse(Context& context, const CString& string, long start, bool matchCase) const;
	virtual void Save(Persist& p);
	static LangElemPtr Load(Persist& p);
};
PTR_DEF(Range);

//Used to know if a CodeLine may parse differently because of stuff on previous lines.
//E.g. if there is a /* on a previous line, then stuff on the current line will parse as
//a comment. I need a stack because in some languages comments can be nested, e.g.
//a /*/* needs 2 */s to end the comment.
class Context {
public:
	stack<RangeConstPtr> mRangeStack;

	//stores either a Range's that doesn't span lines or a RestOfLine for
	//a wrapped line
	LangElemConstPtr mWrappedLangElem;

	bool HasContext() const {
		return !mRangeStack.empty() || mWrappedLangElem != NULL;
	}

	LangElemConstPtr GetParsingLangElem() const {
		ASSERT(HasContext());

		//shouldn't have both kinds of contexts
		ASSERT(mRangeStack.empty() || mWrappedLangElem == NULL);

		if (!mRangeStack.empty())
			return mRangeStack.top();
		else
			return mWrappedLangElem;
	}

	bool operator==(const Context& rhs) const {
		return mRangeStack == rhs.mRangeStack && mWrappedLangElem == rhs.mWrappedLangElem;
	}
};

class Token {
private:
	TokenKinds m_kind;
	CString m_text;
	long m_start;

public:
	Token(TokenKinds k = tkPlainText, long s = 0, const CString& t = CString()) :
	m_kind(k), m_start(s), m_text(t) {}

	TokenKinds GetKind() const {return m_kind;}
	void SetKind(TokenKinds kind) {m_kind = kind;}
	const CString& GetText() const {return m_text;}
	long GetStart() const {return m_start;}
};

//Class used to parse symbols (i.e. things that don't match whole words) efficiently.
//If we added the strings +=, ++, --, and - into the tree, it would look like this
//     <root>
//   +       -
//  / \     /
// =   +   -
//All to nodes except the one for + would contains a pointer to a language element. +
//would not since the string "+" wasn't added to the tree. If I have a string, I can
//tell I can get the longest symbol that matches the beginning of the string in log n time
//where n is the number of elements in the tree.
template<class T>
class ParserTree : public Finder<T> {
private:
	struct Node;

public:
	PTR_DEF(Node);

private:
	struct Node : Obj {
		TCHAR m_char;
		T m_data;
		bool m_hasData;
		typedef vector<NodePtr> Children;
		Children m_children;

		Node(TCHAR ch, const T& data) :
		m_char(ch), m_data(data), m_hasData(true) {}

		Node(TCHAR ch) :
		m_char(ch), m_data(T()), m_hasData(false) {}

		void AddChild(const NodePtr& node) {
			Children::iterator it = m_children.begin();

			while (it != m_children.end()) {
				ATLASSERT(node->m_char != (*it)->m_char);

				if (node->m_char < (*it)->m_char)
					break;

				++it;
			}

			m_children.insert(it, node);
		}

		NodePtr FindChild(TCHAR ch) const {return FindChildImpl(ch, 0, m_children.size() - 1);}

		NodePtr FindChildImpl(TCHAR ch, long first, long last) const {
			if (first > last)
				return NULL;

			// ??? make iterative to avoid possible stack overflow
			long mid = (first + last) / 2;
			TCHAR ch2 = m_children[mid]->m_char;
			if (ch == ch2)
				return m_children[mid];
			else if (ch2 < ch)
				first = mid + 1;
			else
				last = mid - 1;

			return FindChildImpl(ch, first, last);
		}

		void Dump() const {
			if (m_hasData)
				ATLTRACE("%c\n", m_char);

			for (long i = 0; i < m_children.size(); ++i) {
				ATLTRACE("%c", m_char);
				m_children[i]->Dump();
			}
		}
	};

	NodePtr m_root;

	struct FindResult {
		NodePtr node;
		long pos;

		FindResult(const NodePtr& n, long p) : node(n), pos(p) {}
	};

	FindResult Find(const CString& string, bool mustHaveData) const;

public:
	ParserTree() : m_root(new Node(NULL, T())) {}

	void Dump() const {m_root->Dump();}//will print all string in ParserTree
	virtual void Clear() {m_root = new Node(NULL, T());}
	virtual void AddString(const CString& string, const T& data);
	virtual bool FindElement(const CString& string, long start, bool matchCase, T& data, long& elemStart, long& elemEnd) const;
	virtual bool GetElement(const CString& elem, bool matchCase, T& data) const;
};

template<class T>
ParserTree<T>::FindResult ParserTree<T>::Find(const CString& string, bool mustHaveData) const {
	ParserTree<T>::FindResult lastItemWithData(m_root, 0);
	ParserTree<T>::FindResult curItem(m_root, 0);
	const long length = string.GetLength();

	while (curItem.pos < length) {
		ParserTree<T>::NodePtr next = curItem.node->FindChild(string.GetAt(curItem.pos));
		if (next == NULL)
			break;

		curItem.node = next;
		++curItem.pos;

		if (curItem.node->m_hasData)
			lastItemWithData = curItem;
	}

	return mustHaveData ? lastItemWithData : curItem;
}

template<class T>
void ParserTree<T>::AddString(const CString& string, const T& data) {
	FindResult item = Find(string, false);
	const long length = string.GetLength();

	if (item.pos == length) {//string is a substring of a string already in the tree
		ATLASSERT(!item.node->m_hasData);//string already in tree?
		item.node->m_data = data;
		item.node->m_hasData = true;
	} else
		while (item.pos < length) {
			NodePtr next = item.pos == length - 1 ? new Node(string.GetAt(item.pos), data) : new Node(string.GetAt(item.pos));
			item.node->AddChild(next);
			item.node = next;
			++item.pos;
		}
}

template<class T>
bool ParserTree<T>::FindElement(const CString& string, long start, bool /*matchCase*/, T& data, long& elemStart, long& elemEnd) const {
	long pos = start;
	const long length = string.GetLength();

	ParserTree<T>::FindResult item(NULL, 0);
	while (pos < length) {
		item = Find((LPCTSTR)string + pos, true);

		if (item.pos > 0) {//if found a elem starting at pos
			ATLASSERT(item.node->m_hasData);
			elemStart = pos;
			elemEnd = pos + item.pos;
			data = item.node->m_data;
			return true;
		}
		
		++pos;
	}

	return false;
}

template<class T>
bool ParserTree<T>::GetElement(const CString& elem, bool /*matchCase*/, T& data) const {
	ParserTree<T>::FindResult item = Find(elem, true);
	if (item.pos > 0) {
		data = item.node->m_data;
		return true;
	} else
		return false;
}

template<class T>
class WordList : public Finder<T>, public StringIterator {
public:
	struct Elem {
		CString m_word;
		T m_data;

		Elem(const CString& s, const T& l) : m_word(s), m_data(l) {}
	};

private:
	typedef vector<Elem> Elems;
	Elems m_elems;

public:
	virtual long GetNumStrings() const {return m_elems.size();}

	virtual const CString& GetNthString(long i) const {
		ATLASSERT(0 <= i && i < GetNumStrings());
		return m_elems[i].m_word;
	}
	
	virtual const Elem& GetNthElem(long i) const {
		ATLASSERT(0 <= i && i < (long)m_elems.size());
		return m_elems[i];;
	}

	virtual void Clear() {m_elems.clear();}
	virtual void AddString(const CString& string, const T& data);
	virtual bool FindElement(const CString& string, long start, bool matchCase, T& data, long& elemStart, long& elemEnd) const;
	virtual bool GetElement(const CString& elem, bool matchCase, T& data) const;
};

template<class T>
void WordList<T>::AddString(const CString& string, const T& data) {
	Elems::iterator it = m_elems.begin();

	while (it != m_elems.end()) {
		int comp = StrCmp(string, it->m_word);
		ATLASSERT(comp != 0);
		if (comp < 0)
			break;

		++it;
	}

	m_elems.insert(it, Elem(string, data));
}

template<class T>
bool WordList<T>::FindElement(const CString& string, long start, bool matchCase, T& data, long& elemStart, long& elemEnd) const {
	const length = string.GetLength();
	CString token;

	while (start < length) {
		StringToken st = GetNextToken(string, start, token);
		if (st == stIdentifier) {
			if (GetElement(token, matchCase, data)) {
				elemStart = start;
				elemEnd = start + token.GetLength();
				return true;
			}
		}
		
		start += token.GetLength();
	}

	return false;
}

template<class T>
bool WordList<T>::GetElement(const CString& elem, bool matchCase, T& data) const {
	long index = FindMatch(*this, elem, matchCase);
	if (index == -1)
		return false;

	data = m_elems[index].m_data;
	return true;
}

class Language;

class CodeLine : public Obj, public vector<Token> {
friend Language;
private:
	Context m_startingContext, m_endingContext;

	//private b/c only Language can instantiate it
	CodeLine(const Context& context, size_t reserveSize = 0) :
		m_startingContext(context), m_endingContext(context)
	{
		if (reserveSize > 0)
			reserve(reserveSize);
	}

public:
	Token& Item(long i) {return (*this)[i];}
	const Context& GetStartingContext() const {return m_startingContext;}
	const Context& GetEndingContext() const {return m_endingContext;}

	static long FindFirstDiff(const CodeLine& e1, const CodeLine& e2);
};
PTR_DEF(CodeLine);

typedef ParserTree<LangElemPtr> SymbolsTree;
typedef WordList<LangElemPtr> IdentList;

class Language : public Obj {
private:
	SymbolsTree m_symbolTree;//build from all elements of this that DON'T match whole words
	IdentList m_identList;
	bool m_matchCase;
	Vector<LangElemPtr> m_langElems;//used for persistence
	CString m_name;
	CString m_fileDir;

	void Clear();

public:
	StringVector m_extensions;

	Language(const CString& name = EmptyStr, bool matchCase = true, const CString& fileDir = PathMgr().GetNewLanguageFileDir()) :
		m_name(name),
		m_matchCase(matchCase),
		m_fileDir(fileDir)
		{}

	bool GetMatchCase() const {return m_matchCase;}
	void SetMatchCase(bool val) {m_matchCase = val;}

	const CString& GetName() const {return m_name;}
	void SetName(const CString& name) {m_name = name;}

	bool HasExtension(const CString& ext) const {return m_extensions.HasString(ext, false);}
	void AddExtension(const CString& ext) {m_extensions.push_back(ext);}

	void AddLangElem(const LangElemPtr& langElem);

	//this is the only way to create a code line
	CodeLinePtr NewCodeLine(const CString& string, const Context& startingContext, bool doesLineWrap) const;

	void DoPersist(Persist& p);

	const CString& GetFileDir() const {return m_fileDir;}
	void SetFileDir(const CString& val) {m_fileDir = val;}

	CString GetFilePath() const {return MakePath(m_fileDir, m_name + LangExt);}

	const Vector<LangElemPtr>& GetLangElems() const {return m_langElems;}
};
PTR_DEF(Language);

class CLanguage : public Language {
public:
	CLanguage();
};

class HtmlLanguage : public Language {
public:
	HtmlLanguage();
};

#endif