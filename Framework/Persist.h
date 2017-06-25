#ifndef PersistH
#define PersistH

#include "Object.h"

class StringVector;

enum PersistVers {
	PersistVers1 = 1,
	//version 1.0 release
	PersistVers2,//added Ctrl+Shift+G hotkey, char map menu item, and show line numbers
	PersistVers3,//added multiple color schemes feature
	PersistVers4,//added new line before persist line numbers option
	//version 1.1 release
	PersistVers5,//added tab inserts spaces and show whitespace options
	PersistVers6,//added word wrap option
	PersistVers7,//replaced ConfigData.mOpenFilterIndex with ConfigData.mOpenFilterName
	PersistVers8,//replaced Escaped End with Escape Char in languages
	PersistVers9,//added is filter for tools
	//version 2.0
	PersistVers10,//added Ctrl+L hotkey for last cipboard; no longer persist language name in .lang file (file name is used instead)
	//version 2.0.1 release
	PersistVers11,//added default file format
	PersistVers12,//use flags for existing and new bools in Command, and added visible extensions to command; convert old windows explorer command to new one
	PersistVers13,//added options for undo limit and 'add opened files to windows recent documents'

	// ADD NEW PERSISTENCE VERSIONS ABOVE THIS COMMENT

	NextPersistVer,// for defining CurPersistVer, not meant to be used
	CurPersistVer = NextPersistVer - 1// is always equal to current persistence version
};

//class for persisting data in a file. You can use PersistVal() for types where
//there is no ambiguity on how to persist them (e.g. bool, String, HFONT).
//You should use a method that specifies how to persist the data (e.g.
//PersistNumber(), PersistBool()) if there could be ambiguity.
class Persist {
private:
	const bool mLoading;
	const bool mReloading;
	PersistVers mVersion;
	CString mData;
	const CString mFileName;
	long mOffset;
	
	void PrePersist(const CString& name);
	void PostPersist();
	LPCTSTR GetReadPtr() const {return (LPCTSTR)mData + mOffset;}
	void WriteData(const CString& data) {mData += data;}

	//for numbers that are not int's
	template<class T>
	void PersistNumberImpl(const CString& name, T& val) {
		int intVal = (int)val;
		PersistNumber(name, intVal);
		val = (T)intVal;
	}

	//for booleans that are not bool's
	template<class T>
	void PersistBoolImpl(const CString& name, T& val, const T& TrueVal, const T& FalseVal) {
		bool boolVal = val != FalseVal;
		PersistBool(name, boolVal);
		val = boolVal ? TrueVal : FalseVal;
	}

	void SkipEmptyLinesAndComments();
	void SkipLine();

public:
	Persist(const CString& fileName, bool loading, bool reloading);
	~Persist();

	bool IsLoading() const {return mLoading;}
	bool IsReloading() const {return mReloading;}

	long GetVersion() const {return mVersion;}
	bool IsVersBefore(long vers) const {return mVersion < vers;}
	bool IsVersAtLeast(long vers) const {return mVersion >= vers;}

	CString GetFilePath() const {return mFileName;}

	void AddComment(const CString comment, bool addNewLineBefore, bool addNewLineAfter);
	void PersistNewLine();

	//Persistence for some basic types. There is a different method for
	//different basic types because some basic types could be persisted
	//as different things, e.g. an int could be a number or a boolean.
	void PersistBool(const CString& name, bool& val);
	void PersistNumber(const CString& name, int& val);
	void PersistString(const CString& name, CString& val);

	template <class Enum>
	void PersistEnum(const CString& name, Enum& val) {
		PersistNumberImpl(name, val);
	}

	void PersistNumber(const CString& name, unsigned short& val) {
		PersistNumberImpl(name, val);
	}

	void PersistNumber(const CString& name, BYTE& val) {
		PersistNumberImpl(name, val);
	}

	void PersistNumber(const CString& name, long& val) {
		PersistNumberImpl(name, val);
	}

	void PersistNumber(const CString& name, unsigned long& val) {
		PersistNumberImpl(name, val);
	}

	void PersistBool(const CString& name, BYTE& val) {
		PersistBoolImpl(name, val, (BYTE)1, (BYTE)0);
	}

	template<class T>
	void PersistStaticNumberArray(const CString& name, T* val, const int size) {
		for (int i = 0; i < size; ++i) {
			CString name2;
			name2.Format(TEXT("%sElem%d"), name, i);
			PersistNumber(name2, val[i]);
		}
	}

	//Overloaded PersistVal() methods for types where there should never
	//be ambiguity about how to persist them.

	void PersistVal(const CString& name, long& val) {
		PersistNumber(name, val);
	}

	void PersistVal(const CString& name, bool& val) {
		PersistBool(name, val);
	}

	void PersistVal(const CString& name, CString& val) {
		PersistString(name, val);
	}

	void PersistVal(const CString& name, LOGFONT& lf);
	void PersistVal(const CString& name, HFONT& hFont);
	void PersistVal(const CString& name, RECT& rc);

	//T must be compatible with PersistVal()
	template<class T>
	void PersistValVector(const CString& name, vector<T>& val) {
		long count = val.size();
		PersistNumber(name + TEXT("Count"), count);

		if (IsLoading()) {
			val.clear();

			for (long i = 0; i < count; ++i) {
				T temp;
				PersistVal(EmptyStr, temp);
				val.push_back(temp);
			}
		} else
			for (long i = 0; i < count; ++i)
				PersistVal(EmptyStr, val[i]);
	}

	//T must implement DoPersist(Persist&)
	template<class T>
	void PersistClassVector(const CString& name, vector<T>& val) {
		long count = val.size();
		PersistNumber(name + TEXT("Count"), count);

		if (IsLoading()) {
			val.clear();

			for (long i = 0; i < count; ++i) {
				T temp;
				temp.DoPersist(*this);
				val.push_back(temp);
			}
		} else
			for (long i = 0; i < count; ++i)
				val[i].DoPersist(*this);
	}

	//T must implement DoPersist(Persist&)
	template<class T>
	void PersistPtrVector(const CString& name, vector<Ptr<T> >& val) {
		long count = val.size();
		PersistNumber(name + TEXT("Count"), count);

		if (IsLoading()) {
			val.clear();

			for (long i = 0; i < count; ++i) {
				Ptr<T> temp = new T();
				temp->DoPersist(*this);
				val.push_back(temp);
			}
		} else
			for (long i = 0; i < count; ++i)
				val[i]->DoPersist(*this);
	}
};

#endif