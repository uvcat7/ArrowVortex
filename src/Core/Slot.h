#pragma once

#include <Core/Core.h>

namespace Vortex {

using namespace Vortex;

/// Collection of functors that can be bound to call slots.
struct Functor {
    /// Generic functor that can be called trough exec.
    struct Generic {
        virtual ~Generic() {}
        virtual void exec() = 0;
    };

    /// Functor for a static function without arguments.
    template <typename Result>
    struct Static : public Generic {
        typedef Result (*Function)();
        Static(Function f) : f(f) {}
        void exec() { (*f)(); }
        Function f;
    };

    /// Functor for a static function with one argument.
    template <typename Result, typename Arg>
    struct StaticWithArg : public Generic {
        typedef Result (*Function)(Arg);
        StaticWithArg(Function f, Arg a) : f(f), a(a) {}
        void exec() { (*f)(a); }
        Function f;
        Arg a;
    };

    /// Functor for a member function without arguments.
    template <typename Result, typename Object>
    struct Member : public Generic {
        typedef Result (Object::*Function)();
        Member(Object* o, Function f) : o(o), f(f) {}
        void exec() { (o->*f)(); }
        Object* o;
        Function f;
    };

    /// Functor for a member function with one argument.
    template <typename Result, typename Object, typename Arg>
    struct MemberWithArg : public Generic {
        typedef Result (Object::*Function)(Arg);
        MemberWithArg(Object* o, Function f, Arg a) : o(o), f(f), a(a) {}
        void exec() { (o->*f)(a); }
        Object* o;
        Function f;
        Arg a;
    };
};

/// Slot that binds to a single value.
class ValueSlot {
   public:
    ValueSlot();
    ~ValueSlot();

    /// Copies the binding of another value slot.
    void bind(ValueSlot* other);

    /// Binds a read-only value.
    void bind(const int* v);
    void bind(const uint32_t* v);
    void bind(const long* v);
    void bind(const uint64_t* v);
    void bind(const float* v);
    void bind(const double* v);
    void bind(const bool* v);

    /// Binds a read-write value.
    void bind(int* v);
    void bind(uint32_t* v);
    void bind(long* v);
    void bind(uint64_t* v);
    void bind(float* v);
    void bind(double* v);
    void bind(bool* v);

   protected:
    void* data_;  // TODO: replace with a more descriptive variable name.
};

/// Slot that binds to an integer value.
class IntSlot : public ValueSlot {
   public:
    IntSlot();
    ~IntSlot();

    /// Clears the current binding.
    void unbind();

    /// Assigns a value to the current binding, if it is read-write.
    void set(int v);

    /// Returns the value of the current binding.
    int get() const;
};

/// Slot that binds to a floating point value.
class FloatSlot : public ValueSlot {
   public:
    FloatSlot();
    ~FloatSlot();

    /// Clears the current binding.
    void unbind();

    /// Assigns a value to the current binding, if it is read-write.
    void set(double v);

    /// Returns the value of the current binding.
    double get() const;
};

/// Slot that binds to a boolean value.
class BoolSlot : public ValueSlot {
   public:
    BoolSlot();
    ~BoolSlot();

    /// Clears the current binding.
    void unbind();

    /// Assigns a value to the current binding, if it is read-write.
    void set(bool v);

    /// Returns the value of the current binding.
    bool get() const;
};

/// Slot that binds to a string.
class TextSlot {
   public:
    TextSlot();
    ~TextSlot();

    /// Clears the current binding.
    void unbind();

    /// Copies the binding of another String slot.
    void bind(TextSlot* other);

    /// Binds a read-only String.
    void bind(const char* str);
    void bind(const std::string* str);

    /// Binds a read-write String.
    void bind(std::string* str);

    /// Assigns a string to the current binding, if the binding is writable.
    void set(const char* str);
    void set(const std::string& str);

    /// Returns the string contents of the current binding.
    const char* get() const;

   private:
    void* data_;  // TODO: replace with a more descriptive variable name.
};

/// Slot that binds to a functor.
class CallSlot {
   public:
    CallSlot();
    ~CallSlot();

    /// Clears the current binding, deleting the functor.
    void unbind();

    /// Calls the functor of the current binding.
    void call();

    /// Binds a functor, and takes ownership of it.
    void bind(Functor::Generic* f);

    /// Binds a static function without arguments.
    template <typename Result>
    void bind(Result (*function)()) {
        bind(new Functor::Static<Result>(function));
    }

    /// Binds a static function with one argument.
    template <typename Result, typename Arg>
    void bind(Result (*function)(Arg), Arg arg) {
        bind(new Functor::StaticWithArg<Result, Arg>(function, arg));
    }

    /// Binds an object with a member function without arguments.
    template <typename Result, typename Object>
    void bind(Object* obj, Result (Object::*member)()) {
        bind(new Functor::Member<Result, Object>(obj, member));
    }

    /// Binds an object with a member function with one argument.
    template <typename Result, typename Object, typename Arg>
    void bind(Object* obj, Result (Object::*member)(Arg), Arg arg) {
        bind(new Functor::MemberWithArg<Result, Object, Arg>(obj, member, arg));
    }

   private:
    void* data_;  // TODO: replace with a more descriptive variable name.
};

};  // namespace Vortex
