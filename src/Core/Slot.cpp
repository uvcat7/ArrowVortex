#include <Core/Slot.h>

#include <math.h>

namespace Vortex {

// ================================================================================================
// Value helper classes.

namespace {

struct BaseValue
{
	BaseValue() : refs(1) {}
	virtual ~BaseValue() {}

	virtual void set(int v) {}
	virtual void set(double v) {}
	virtual void set(bool v) {}

	virtual int geti() const = 0;
	virtual double getf() const = 0;
	virtual bool getb() const = 0;

	int refs;
};

// Self-contained values.

struct IntValue : public BaseValue
{
	IntValue() : val(0) {}

	void set(int v) { val = v; }
	void set(double v) { val = (int)lround(v); }
	void set(bool v) { val = v; }

	int geti() const { return val; }
	double getf() const { return val; }
	bool getb() const { return val != 0; }

	int val;
};

struct DoubleValue : public BaseValue
{
	DoubleValue() : val(0) {}

	void set(int v) { val = v; }
	void set(double v) { val = v; }
	void set(bool v) { val = v; }

	int geti() const { return (int)val; }
	double getf() const { return val; }
	bool getb() const { return val != 0; }

	double val;
};

struct BoolValue : public BaseValue
{
	BoolValue() : val(false) {}

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

// Values by reference.

template <typename T>
struct IntPtr : public BaseValue
{
	IntPtr(T* v) : val(v) {}

	void set(int v) { *val = (T)v; }
	void set(double v) { *val = (T)lround(v); }
	void set(bool v) { *val = v; }

	int geti() const { return (int)*val; }
	double getf() const { return (double)*val; }
	bool getb() const { return *val != 0; }

	T* val;
};

template <typename T>
struct ConstIntPtr : public BaseValue
{
	ConstIntPtr(const T* v) : val(v) {}

	int geti() const { return (int)*val; }
	double getf() const { return (double)*val; }
	bool getb() const { return *val != 0; }

	const T* val;
};

template <typename T>
struct FloatPtr : public BaseValue
{
	FloatPtr(T* v) : val(v) {}

	void set(int v) { *val = (T)v; }
	void set(double v) { *val = (T)v; }
	void set(bool v) { *val = v; }

	int geti() const { return (int)lround(*val); }
	double getf() const { return (double)*val; }
	bool getb() const { return *val != 0; }

	T* val;
};

template <typename T>
struct ConstFloatPtr : public BaseValue
{
	ConstFloatPtr(const T* v) : val(v) {}

	int geti() const { return (int)lround(*val); }
	double getf() const { return (double)*val; }
	bool getb() const { return *val != 0; }

	const T* val;
};

struct BoolPtr : public BaseValue
{
	BoolPtr(bool* v) : val(v) {}

	void set(int v) { *val = v != 0; }
	void set(double v) { *val = v != 0; }
	void set(bool v) {
		*val = v;
	}

	int geti() const { return (int)*val; }
	double getf() const { return (double)*val; }
	bool getb() const { return *val; }

	bool* val;
};

struct ConstBoolPtr : public BaseValue
{
	ConstBoolPtr(const bool* v) : val(v) {}

	int geti() const { return (int)*val; }
	double getf() const { return (double)*val; }
	bool getb() const { return *val; }

	const bool* val;
};

static void ReferenceVal(void* data)
{
	BaseValue* v = (BaseValue*)data;
	++v->refs;
}

static void ReleaseVal(void* data)
{
	BaseValue* v = (BaseValue*)data;
	if(!--v->refs) delete v;
}

}; // anonymous namespace.

// ================================================================================================
// ValueSlot :: implementation.

ValueSlot::ValueSlot()
	: myData(nullptr)
{
}

ValueSlot::~ValueSlot()
{
	ReleaseVal(myData);
}

void ValueSlot::bind(ValueSlot* other)
{
	ReferenceVal(other->myData);
	ReleaseVal(myData);
	myData = other->myData;
}

void ValueSlot::bind(const int* v)
{
	ReleaseVal(myData);
	myData = new ConstIntPtr<int>(v);
}

void ValueSlot::bind(const uint* v)
{
	ReleaseVal(myData);
	myData = new ConstIntPtr<uint>(v);
}

void ValueSlot::bind(const long* v)
{
	ReleaseVal(myData);
	myData = new ConstIntPtr<long>(v);
}

void ValueSlot::bind(const ulong* v)
{
	ReleaseVal(myData);
	myData = new ConstIntPtr<ulong>(v);
}

void ValueSlot::bind(const float* v)
{
	ReleaseVal(myData);
	myData = new ConstFloatPtr<float>(v);
}

void ValueSlot::bind(const double* v)
{
	ReleaseVal(myData);
	myData = new ConstFloatPtr<double>(v);
}

void ValueSlot::bind(const bool* v)
{
	ReleaseVal(myData);
	myData = new ConstBoolPtr(v);
}

void ValueSlot::bind(int* v)
{
	ReleaseVal(myData);
	myData = new IntPtr<int>(v);
}

void ValueSlot::bind(uint* v)
{
	ReleaseVal(myData);
	myData = new IntPtr<uint>(v);
}

void ValueSlot::bind(long* v)
{
	ReleaseVal(myData);
	myData = new IntPtr<long>(v);
}

void ValueSlot::bind(ulong* v)
{
	ReleaseVal(myData);
	myData = new IntPtr<ulong>(v);
}

void ValueSlot::bind(float* v)
{
	ReleaseVal(myData);
	myData = new FloatPtr<float>(v);
}

void ValueSlot::bind(double* v)
{
	ReleaseVal(myData);
	myData = new FloatPtr<double>(v);
}

void ValueSlot::bind(bool* v)
{
	ReleaseVal(myData);
	myData = new BoolPtr(v);
}

// ================================================================================================
// IntSlot :: implementation.

IntSlot::IntSlot()
{
	myData = new IntValue;
}

IntSlot::~IntSlot()
{
}

void IntSlot::unbind()
{
	ReleaseVal(myData);
	myData = new IntValue;
}

void IntSlot::set(int v)
{
	((BaseValue*)myData)->set(v);
}

int IntSlot::get() const
{
	return ((BaseValue*)myData)->geti();
}

// ================================================================================================
// FloatSlot :: implementation.

FloatSlot::FloatSlot()
{
	myData = new DoubleValue;
}

FloatSlot::~FloatSlot()
{
}

void FloatSlot::unbind()
{
	ReleaseVal(myData);
	myData = new DoubleValue;
}

void FloatSlot::set(double v)
{
	((BaseValue*)myData)->set(v);
}

double FloatSlot::get() const
{
	return ((BaseValue*)myData)->getf();
}

// ================================================================================================
// BoolSlot :: implementation.

BoolSlot::BoolSlot()
{
	myData = new BoolValue;
}

BoolSlot::~BoolSlot()
{
}

void BoolSlot::unbind()
{
	ReleaseVal(myData);
	myData = new BoolValue;
}

void BoolSlot::set(bool v)
{
	((BaseValue*)myData)->set(v);
}

bool BoolSlot::get() const
{
	return ((BaseValue*)myData)->getb();
}

// ================================================================================================
// TextSlot :: helper classes.

namespace {

struct BaseStr
{
	BaseStr() : refs(1) {}
	virtual ~BaseStr() {}

	virtual void set(const char* s) {}
	virtual const char* get() const = 0;

	int refs;
};

struct StringVal : public BaseStr
{
	void set(const char* s) { str = s; }
	const char* get() const { return str.str(); }
	String str;
};

struct ConstCharPtr : public BaseStr
{
	ConstCharPtr(const char* s) : str(s) {}
	const char* get() const { return str; }
	const char* str;
};

struct StringPtr : public BaseStr
{
	StringPtr(String* s) : str(s) {}
	void set(const char* s) { *str = s; }
	const char* get() const { return str->str(); }
	String* str;
};

struct ConstStringPtr : public BaseStr
{
	ConstStringPtr(const String* s) : str(s) {}
	const char* get() const { return str->str(); }
	const String* str;
};

static void ReferenceStr(void* data)
{
	BaseStr* str = (BaseStr*)data;
	++str->refs;
}

static void ReleaseStr(void* data)
{
	BaseStr* str = (BaseStr*)data;
	if(--str->refs == 0) delete str;
}

}; // anonymous namespace.

// ================================================================================================
// TextSlot :: implementation.

TextSlot::TextSlot()
{
	myData = new StringVal;
}

TextSlot::~TextSlot()
{
	ReleaseStr(myData);
}

void TextSlot::unbind()
{
	ReleaseStr(myData);
	myData = new StringVal;
}

void TextSlot::bind(TextSlot* other)
{
	ReferenceStr(other->myData);
	ReleaseStr(myData);
	myData = other->myData;
}

void TextSlot::bind(const char* str)
{
	ReleaseStr(myData);
	myData = new ConstCharPtr(str);
}

void TextSlot::bind(const String* str)
{
	ReleaseStr(myData);
	myData = new ConstStringPtr(str);
}

void TextSlot::bind(String* str)
{
	ReleaseStr(myData);
	myData = new StringPtr(str);
}

void TextSlot::set(const char* str)
{
	((BaseStr*)myData)->set(str);
}

void TextSlot::set(StringRef str)
{
	((BaseStr*)myData)->set(str.str());
}

const char* TextSlot::get() const
{
	return ((BaseStr*)myData)->get();
}

// ================================================================================================
// CallSlot :: implementation.

CallSlot::CallSlot() : myData(nullptr)
{
}

CallSlot::~CallSlot()
{
	delete (Functor::Generic*)myData;
}

void CallSlot::unbind()
{
	delete (Functor::Generic*)myData;
	myData = nullptr;
}

void CallSlot::bind(Functor::Generic* fun)
{
	delete (Functor::Generic*)myData;
	myData = fun;
}

void CallSlot::call()
{
	if(myData) ((Functor::Generic*)myData)->exec();
}

}; // namespace Vortex
