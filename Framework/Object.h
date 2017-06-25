#ifndef ObjectH
#define ObjectH

//Obj derived classes must be created with new
class Obj {
private:
	mutable long mRefs;

public:
	Obj() : mRefs(0) {}

	// don't copy mRefs
	Obj(const Obj& rhs) : mRefs(0) {}
	operator=(const Obj& rhs) {}

	virtual ~Obj() {
		ASSERT(mRefs == 0);//deleted too early or statically allocated?
	}

	void AddRef() const {
		ASSERT(mRefs >= 0);
		++mRefs;
	}

	void Release() const {
		ASSERT(mRefs >= 1);
		if (--mRefs == 0)
			delete this;
	}
};

//T must be an Obj derived object
template<class T>
class Ptr {
private:
	T* mObj;

	void AddRef() {
		if (mObj != NULL)
			mObj->AddRef();
	}

	void Release() {
		if (mObj != NULL)
			mObj->Release();
	}

public:
	Ptr(T* obj = NULL) : mObj(obj) {AddRef();}
	Ptr(const Ptr& ptr) : mObj(NULL) {*this = ptr;}
	~Ptr() {Release();}

	Ptr& operator=(const Ptr& rhs) {return *this = rhs.mObj;}

	Ptr& operator=(T* obj) {
		if (mObj != obj) {
			Release();
			mObj = obj;
			AddRef();
		}

		return *this;
	}

	bool operator==(const Ptr& rhs) const {return mObj == rhs.mObj;}
	bool operator==(const Obj* obj) const {return mObj == obj;}
	bool operator!=(const Ptr& rhs) const {return !(*this == rhs);}
	bool operator!=(const Obj* obj) const {return !(*this == obj);}	

	T* operator->() {ASSERT(mObj != NULL); return mObj;}
	const T* operator->() const {ASSERT(mObj != NULL); return mObj;}
	T& operator*() {ASSERT(mObj != NULL); return *mObj;}
	const T& operator*() const {ASSERT(mObj != NULL); return *mObj;}
	operator T*() {return mObj;}
	operator const T*() const {return mObj;}
	T* GetPtr() {return mObj;}
	const T* GetPtr() const {return mObj;}
};

//macros for defining smart pointers
#define PTR_DEF(ClassName) \
	typedef Ptr<ClassName> ClassName##Ptr; \
	typedef Ptr<const ClassName> ClassName##ConstPtr;

#define PTR_DEF_T(ClassName) \
	template<class T> typedef Ptr<ClassName<T> > ClassName##Ptr; \
	template<class T> typedef Ptr<const ClassName<T> > ClassName##ConstPtr;

#endif