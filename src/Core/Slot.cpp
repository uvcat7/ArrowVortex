#include <Core/Slot.h>

#include <math.h>

namespace Vortex {

// ================================================================================================
// Value helper classes.

namespace {

struct BaseValue {
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

struct IntValue : public BaseValue {
    IntValue() : val(0) {}

    void set(int v) { val = v; }
    void set(double v) { val = (int)lround(v); }
    void set(bool v) { val = v; }

    int geti() const { return val; }
    double getf() const { return val; }
    bool getb() const { return val != 0; }

    int val;
};

struct DoubleValue : public BaseValue {
    DoubleValue() : val(0) {}

    void set(int v) { val = v; }
    void set(double v) { val = v; }
    void set(bool v) { val = v; }

    int geti() const { return (int)val; }
    double getf() const { return val; }
    bool getb() const { return val != 0; }

    double val;
};

struct BoolValue : public BaseValue {
    BoolValue() : val(false) {}

    void set(int v) { val = (v != 0); }
    void set(double v) { val = (v != 0); }
    void set(bool v) { val = v; }

    int geti() const { return val; }
    double getf() const { return val; }
    bool getb() const { return val; }

    bool val;
};

// Values by reference.

template <typename T>
struct IntPtr : public BaseValue {
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
struct ConstIntPtr : public BaseValue {
    ConstIntPtr(const T* v) : val(v) {}

    int geti() const { return (int)*val; }
    double getf() const { return (double)*val; }
    bool getb() const { return *val != 0; }

    const T* val;
};

template <typename T>
struct FloatPtr : public BaseValue {
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
struct ConstFloatPtr : public BaseValue {
    ConstFloatPtr(const T* v) : val(v) {}

    int geti() const { return (int)lround(*val); }
    double getf() const { return (double)*val; }
    bool getb() const { return *val != 0; }

    const T* val;
};

struct BoolPtr : public BaseValue {
    BoolPtr(bool* v) : val(v) {}

    void set(int v) { *val = v != 0; }
    void set(double v) { *val = v != 0; }
    void set(bool v) { *val = v; }

    int geti() const { return (int)*val; }
    double getf() const { return (double)*val; }
    bool getb() const { return *val; }

    bool* val;
};

struct ConstBoolPtr : public BaseValue {
    ConstBoolPtr(const bool* v) : val(v) {}

    int geti() const { return (int)*val; }
    double getf() const { return (double)*val; }
    bool getb() const { return *val; }

    const bool* val;
};

static void ReferenceVal(void* data) {
    BaseValue* v = (BaseValue*)data;
    ++v->refs;
}

static void ReleaseVal(void* data) {
    BaseValue* v = (BaseValue*)data;
    if (!--v->refs) delete v;
}

};  // anonymous namespace.

// ================================================================================================
// ValueSlot :: implementation.

ValueSlot::ValueSlot() : data_(nullptr) {}

ValueSlot::~ValueSlot() { ReleaseVal(data_); }

void ValueSlot::bind(ValueSlot* other) {
    ReferenceVal(other->data_);
    ReleaseVal(data_);
    data_ = other->data_;
}

void ValueSlot::bind(const int* v) {
    ReleaseVal(data_);
    data_ = new ConstIntPtr<int>(v);
}

void ValueSlot::bind(const uint32_t* v) {
    ReleaseVal(data_);
    data_ = new ConstIntPtr<uint32_t>(v);
}

void ValueSlot::bind(const long* v) {
    ReleaseVal(data_);
    data_ = new ConstIntPtr<long>(v);
}

void ValueSlot::bind(const uint64_t* v) {
    ReleaseVal(data_);
    data_ = new ConstIntPtr<uint64_t>(v);
}

void ValueSlot::bind(const float* v) {
    ReleaseVal(data_);
    data_ = new ConstFloatPtr<float>(v);
}

void ValueSlot::bind(const double* v) {
    ReleaseVal(data_);
    data_ = new ConstFloatPtr<double>(v);
}

void ValueSlot::bind(const bool* v) {
    ReleaseVal(data_);
    data_ = new ConstBoolPtr(v);
}

void ValueSlot::bind(int* v) {
    ReleaseVal(data_);
    data_ = new IntPtr<int>(v);
}

void ValueSlot::bind(uint32_t* v) {
    ReleaseVal(data_);
    data_ = new IntPtr<uint32_t>(v);
}

void ValueSlot::bind(long* v) {
    ReleaseVal(data_);
    data_ = new IntPtr<long>(v);
}

void ValueSlot::bind(uint64_t* v) {
    ReleaseVal(data_);
    data_ = new IntPtr<uint64_t>(v);
}

void ValueSlot::bind(float* v) {
    ReleaseVal(data_);
    data_ = new FloatPtr<float>(v);
}

void ValueSlot::bind(double* v) {
    ReleaseVal(data_);
    data_ = new FloatPtr<double>(v);
}

void ValueSlot::bind(bool* v) {
    ReleaseVal(data_);
    data_ = new BoolPtr(v);
}

// ================================================================================================
// IntSlot :: implementation.

IntSlot::IntSlot() { data_ = new IntValue; }

IntSlot::~IntSlot() {}

void IntSlot::unbind() {
    ReleaseVal(data_);
    data_ = new IntValue;
}

void IntSlot::set(int v) { ((BaseValue*)data_)->set(v); }

int IntSlot::get() const { return ((BaseValue*)data_)->geti(); }

// ================================================================================================
// FloatSlot :: implementation.

FloatSlot::FloatSlot() { data_ = new DoubleValue; }

FloatSlot::~FloatSlot() {}

void FloatSlot::unbind() {
    ReleaseVal(data_);
    data_ = new DoubleValue;
}

void FloatSlot::set(double v) { ((BaseValue*)data_)->set(v); }

double FloatSlot::get() const { return ((BaseValue*)data_)->getf(); }

// ================================================================================================
// BoolSlot :: implementation.

BoolSlot::BoolSlot() { data_ = new BoolValue; }

BoolSlot::~BoolSlot() {}

void BoolSlot::unbind() {
    ReleaseVal(data_);
    data_ = new BoolValue;
}

void BoolSlot::set(bool v) { ((BaseValue*)data_)->set(v); }

bool BoolSlot::get() const { return ((BaseValue*)data_)->getb(); }

// ================================================================================================
// TextSlot :: helper classes.

namespace {

struct BaseStr {
    BaseStr() : refs(1) {}
    virtual ~BaseStr() {}

    virtual void set(const char* s) {}
    virtual const char* get() const = 0;

    int refs;
};

struct StringVal : public BaseStr {
    void set(const char* s) { str = s; }
    const char* get() const { return str.c_str(); }
    std::string str;
};

struct ConstCharPtr : public BaseStr {
    ConstCharPtr(const char* s) : str(s) {}
    const char* get() const { return str; }
    const char* str;
};

struct StringPtr : public BaseStr {
    StringPtr(std::string* s) : str(s) {}
    void set(const char* s) { *str = s; }
    const char* get() const { return str->c_str(); }
    std::string* str;
};

struct ConstStringPtr : public BaseStr {
    ConstStringPtr(const std::string* s) : str(s) {}
    const char* get() const { return str->c_str(); }
    const std::string* str;
};

static void ReferenceStr(void* data) {
    BaseStr* str = (BaseStr*)data;
    ++str->refs;
}

static void ReleaseStr(void* data) {
    BaseStr* str = (BaseStr*)data;
    if (--str->refs == 0) delete str;
}

};  // anonymous namespace.

// ================================================================================================
// TextSlot :: implementation.

TextSlot::TextSlot() { data_ = new StringVal; }

TextSlot::~TextSlot() { ReleaseStr(data_); }

void TextSlot::unbind() {
    ReleaseStr(data_);
    data_ = new StringVal;
}

void TextSlot::bind(TextSlot* other) {
    ReferenceStr(other->data_);
    ReleaseStr(data_);
    data_ = other->data_;
}

void TextSlot::bind(const char* str) {
    ReleaseStr(data_);
    data_ = new ConstCharPtr(str);
}

void TextSlot::bind(const std::string* str) {
    ReleaseStr(data_);
    data_ = new ConstStringPtr(str);
}

void TextSlot::bind(std::string* str) {
    ReleaseStr(data_);
    data_ = new StringPtr(str);
}

void TextSlot::set(const char* str) { ((BaseStr*)data_)->set(str); }

void TextSlot::set(const std::string& str) {
    ((BaseStr*)data_)->set(str.c_str());
}

const char* TextSlot::get() const { return ((BaseStr*)data_)->get(); }

// ================================================================================================
// CallSlot :: implementation.

CallSlot::CallSlot() : data_(nullptr) {}

CallSlot::~CallSlot() { delete (Functor::Generic*)data_; }

void CallSlot::unbind() {
    delete (Functor::Generic*)data_;
    data_ = nullptr;
}

void CallSlot::bind(Functor::Generic* fun) {
    delete (Functor::Generic*)data_;
    data_ = fun;
}

void CallSlot::call() {
    if (data_) ((Functor::Generic*)data_)->exec();
}

};  // namespace Vortex
