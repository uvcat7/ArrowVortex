#include <Precomp.h>

#include <Core/Interface/Binding.h>

namespace vortex {

using namespace std;

/*

// Value binding base.

struct ValueBinding::Data
{
	virtual ~Data() {}

	virtual void set(int v) {}
	virtual void set(double v) {}
	virtual void set(bool v) {}

	virtual int geti() const = 0;
	virtual double getf() const = 0;
	virtual bool getb() const = 0;

	int count = 1;
};

// string binding base.

struct StringBinding::Data
{
	virtual ~Data() {}

	virtual void set(const char* s) {}
	virtual const char* get() const = 0;

	int count = 1;
};

typedef ValueBinding::Data BValue;
typedef StringBinding::Data BString;

// Self-contained value bindings.

struct BInt: public BValue
{
	void set(int v) { val = v; }
	void set(double v) { val = (int)lround(v); }
	void set(bool v) { val = v; }

	int geti() const { return val; }
	double getf() const { return val; }
	bool getb() const { return val != 0; }

	int val = 0;
};

struct BDouble: public BValue
{
	void set(int v) { val = v; }
	void set(double v) { val = v; }
	void set(bool v) { val = v; }

	int geti() const { return (int)val; }
	double getf() const { return val; }
	bool getb() const { return val != 0; }

	double val = 0.0;
};

struct BBool: public BValue
{
	BBool() : val(false) {}

	void set(int v) { val = (v != 0); }
	void set(double v) { val = (v != 0); }
	void set(bool v) {
		val = v;
	}

	int geti() const { return val; }
	double getf() const { return val; }
	bool getb() const { return val; }

	bool val;
};

// Value bindings by reference.

template <typename T>
struct BIntRef: public BValue
{
	BIntRef(T* v) : val(v) {}

	void set(int v) { *val = (T)v; }
	void set(double v) { *val = (T)lround(v); }
	void set(bool v) { *val = v; }

	int geti() const { return (int)*val; }
	double getf() const { return (double)*val; }
	bool getb() const { return *val != 0; }

	T* val;
};

template <typename T>
struct BConstIntRef: public BValue
{
	BConstIntRef(const T* v) : val(v) {}

	int geti() const { return (int)*val; }
	double getf() const { return (double)*val; }
	bool getb() const { return *val != 0; }

	const T* val;
};

template <typename T>
struct BFloatRef: public BValue
{
	BFloatRef(T* v) : val(v) {}

	void set(int v) { *val = (T)v; }
	void set(double v) { *val = (T)v; }
	void set(bool v) { *val = v; }

	int geti() const { return (int)lround(*val); }
	double getf() const { return (double)*val; }
	bool getb() const { return *val != 0; }

	T* val;
};

template <typename T>
struct BConstFloatRef: public BValue
{
	BConstFloatRef(const T* v) : val(v) {}

	int geti() const { return (int)lround(*val); }
	double getf() const { return (double)*val; }
	bool getb() const { return *val != 0; }

	const T* val;
};

struct BBoolRef: public BValue
{
	BBoolRef(bool* v) : val(v) {}

	void set(int v) { *val = v != 0; }
	void set(double v) { *val = v != 0; }
	void set(bool v) { *val = v; }

	int geti() const { return (int)*val; }
	double getf() const { return (double)*val; }
	bool getb() const { return *val; }

	bool* val;
};

struct BConstBoolRef: public BValue
{
	BConstBoolRef(const bool* v) : val(v) {}

	int geti() const { return (int)*val; }
	double getf() const { return (double)*val; }
	bool getb() const { return *val; }

	const bool* val;
};

// Self contained string binding.

struct BStringValue : public BString
{
	void set(const char* s) { str = s; }
	const char* get() const { return str.data(); }
	string str;
};

// string bindings by reference.

struct BStringLiteral : public BString
{
	BStringLiteral(const char* s) : str(s) {}
	const char* get() const { return str; }
	const char* str;
};

struct BStringRef : public BString
{
	BStringRef(string* s) : str(s) {}
	void set(const char* s) { *str = s; }
	const char* get() const { return str->data(); }
	string* str;
};

struct BConstStringRef : public BString
{
	BConstStringRef(const string* s) : str(s) {}
	const char* get() const { return str->data(); }
	const string* str;
};

// Binding management.

static void Increment(BValue* data)
{
	++data->count;
}

static void Release(BValue* data)
{
	if (--data->count == 0) delete data;
}

static void Increment(BString* data)
{
	++data->count;
}

static void Release(BString* data)
{
	if (--data->count == 0) delete data;
}

// ====================================================================================================================
// ValueBinding :: implementation.

ValueBinding::ValueBinding()
	: myData(nullptr)
{
}

ValueBinding::~ValueBinding()
{
	Release(myData);
}

void ValueBinding::bind(ValueBinding* other)
{
	Increment(other->myData);
	Release(myData);
	myData = other->myData;
}

void ValueBinding::bind(const int* v)
{
	Release(myData);
	myData = new BConstIntRef<int>(v);
}

void ValueBinding::bind(const uint* v)
{
	Release(myData);
	myData = new BConstIntRef<uint>(v);
}

void ValueBinding::bind(const long* v)
{
	Release(myData);
	myData = new BConstIntRef<long>(v);
}

void ValueBinding::bind(const ulong* v)
{
	Release(myData);
	myData = new BConstIntRef<ulong>(v);
}

void ValueBinding::bind(const float* v)
{
	Release(myData);
	myData = new BConstFloatRef<float>(v);
}

void ValueBinding::bind(const double* v)
{
	Release(myData);
	myData = new BConstFloatRef<double>(v);
}

void ValueBinding::bind(const bool* v)
{
	Release(myData);
	myData = new BConstBoolRef(v);
}

void ValueBinding::bind(int* v)
{
	Release(myData);
	myData = new BIntRef<int>(v);
}

void ValueBinding::bind(uint* v)
{
	Release(myData);
	myData = new BIntRef<uint>(v);
}

void ValueBinding::bind(long* v)
{
	Release(myData);
	myData = new BIntRef<long>(v);
}

void ValueBinding::bind(ulong* v)
{
	Release(myData);
	myData = new BIntRef<ulong>(v);
}

void ValueBinding::bind(float* v)
{
	Release(myData);
	myData = new BFloatRef<float>(v);
}

void ValueBinding::bind(double* v)
{
	Release(myData);
	myData = new BFloatRef<double>(v);
}

void ValueBinding::bind(bool* v)
{
	Release(myData);
	myData = new BBoolRef(v);
}

// ====================================================================================================================
// IntBinding :: implementation.

IntBinding::IntBinding()
{
	myData = new BInt;
}

IntBinding::~IntBinding()
{
}

void IntBinding::unbind()
{
	Release(myData);
	myData = new BInt;
}

void IntBinding::set(int v)
{
	myData->set(v);
}

int IntBinding::get() const
{
	return myData->geti();
}

// ====================================================================================================================
// FloatBinding :: implementation.

FloatBinding::FloatBinding()
{
	myData = new BDouble;
}

FloatBinding::~FloatBinding()
{
}

void FloatBinding::unbind()
{
	Release(myData);
	myData = new BDouble;
}

void FloatBinding::set(double v)
{
	myData->set(v);
}

double FloatBinding::get() const
{
	return myData->getf();
}

// ====================================================================================================================
// BoolBinding :: implementation.

BoolBinding::BoolBinding()
{
	myData = new BBool;
}

BoolBinding::~BoolBinding()
{
}

void BoolBinding::unbind()
{
	Release(myData);
	myData = new BBool;
}

void BoolBinding::set(bool v)
{
	myData->set(v);
}

bool BoolBinding::get() const
{
	return myData->getb();
}

// ====================================================================================================================
// StringBinding :: implementation.

StringBinding::StringBinding()
{
	myData = new BStringValue;
}

StringBinding::~StringBinding()
{
	Release(myData);
}

void StringBinding::unbind()
{
	Release(myData);
	myData = new BStringValue;
}

void StringBinding::bind(StringBinding* other)
{
	Increment(other->myData);
	Release(myData);
	myData = other->myData;
}

void StringBinding::bind(const char* str)
{
	Release(myData);
	myData = new BStringLiteral(str);
}

void StringBinding::bind(const string* str)
{
	Release(myData);
	myData = new BConstStringRef(str);
}

void StringBinding::bind(string* str)
{
	Release(myData);
	myData = new BStringRef(str);
}

void StringBinding::set(const char* str)
{
	myData->set(str);
}

void StringBinding::set(stringref str)
{
	myData->set(str.data());
}

const char* StringBinding::get() const
{
	return myData->get();
}

// ====================================================================================================================
// ActionBinding :: implementation.

ActionBinding::ActionBinding()
: myAction(nullptr)
{
}

ActionBinding::~ActionBinding()
{
	delete myAction;
}

void ActionBinding::unbind()
{
	Util::reset(myAction);
}

void ActionBinding::bind(Action* action)
{
	delete myAction;
	myAction = action;
}

void ActionBinding::call()
{
	if (myAction) myAction->exec();
}

*/

} // namespace vortex