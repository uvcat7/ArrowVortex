#pragma once

namespace vortex {

/*

// Binding that binds to a single value.
class ValueBinding
{
public:
	struct Data;

	ValueBinding();
	~ValueBinding();

	// Copies the binding of another value slot.
	void bind(ValueBinding* other);

	// Binds a read-only value.
	void bind(const int* v);
	void bind(const uint* v);
	void bind(const long* v);
	void bind(const ulong* v);
	void bind(const float* v);
	void bind(const double* v);
	void bind(const bool* v);

	// Binds a read-write value.
	void bind(int* v);
	void bind(uint* v);
	void bind(long* v);
	void bind(ulong* v);
	void bind(float* v);
	void bind(double* v);
	void bind(bool* v);

protected:
	Data* myData;
};

// Binding that binds to an integer value.
class IntBinding : public ValueBinding
{
public:
	IntBinding();
	~IntBinding();

	// Clears the current binding.
	void unbind();

	// Assigns a value to the current binding, if it is read-write.
	void set(int v);

	// Returns the value of the current binding.
	int get() const;
};

// Binding that binds to a floating point value.
class FloatBinding : public ValueBinding
{
public:
	FloatBinding();
	~FloatBinding();

	// Clears the current binding.
	void unbind();

	// Assigns a value to the current binding, if it is read-write.
	void set(double v);

	// Returns the value of the current binding.
	double get() const;
};

// Binding that binds to a boolean value.
class BoolBinding : public ValueBinding
{
public:
	BoolBinding();
	~BoolBinding();

	// Clears the current binding.
	void unbind();

	// Assigns a value to the current binding, if it is read-write.
	void set(bool v);

	// Returns the value of the current binding.
	bool get() const;
};

// Binding that binds to a string.
class StringBinding
{
public:
	struct Data;

	StringBinding();
	~StringBinding();

	// Clears the current binding.
	void unbind();

	// Copies the binding of another string slot.
	void bind(StringBinding* other);

	// Binds a read-only string.
	void bind(const char* str);
	void bind(const std::string* str);

	// Binds a read-write string.
	void bind(std::string* str);

	// Assigns a string to the current binding, if the binding is writable.
	void set(const char* str);
	void set(stringref str);

	// Returns the string contents of the current binding.
	const char* get() const;

private:
	Data* myData;
};

// Binding that binds to an action.
class ActionBinding
{
public:
	ActionBinding();
	~ActionBinding();

	// Base action.
	struct Action
	{
		virtual ~Action() {}
		virtual void exec() = 0;
	};

	// Encapsulates a static function without arguments.
	template <typename Result>
	struct Static : public Action
	{
		typedef Result(*Function)();
		Static(Function f) : f(f) {}
		void exec() override { (*f)(); }
		Function f;
	};

	// Encapsulates static function with one argument.
	template <typename Result, typename Arg>
	struct StaticWithArg : public Action
	{
		typedef Result(*Function)(Arg);
		StaticWithArg(Function f, Arg a) : f(f), a(a) {}
		void exec() override { (*f)(a); }
		Function f;	Arg a;
	};

	// Encapsulates a member function without arguments.
	template <typename Result, typename Object>
	struct Member : public Action
	{
		typedef Result(Object::*Function)();
		Member(Object* o, Function f) : o(o), f(f) {}
		void exec() override { (o->*f)(); }
		Function f;	Object* o;
	};

	// Encapsulates a member function with one argument.
	template <typename Result, typename Object, typename Arg>
	struct MemberWithArg : public Action
	{
		typedef Result(Object::*Function)(Arg);
		MemberWithArg(Object* o, Function f, Arg a) : o(o), f(f), a(a) {}
		void exec() override { (o->*f)(a); }
		Function f;	Object* o; Arg a;
	};

	// Clears the current binding, deleting the functor.
	void unbind();

	// Calls the functor of the current binding.
	void call();

	// Binds an action, and takes ownership of it.
	void bind(Action* action);

	// Binds a static function that takes no arguments.
	template <typename Result>
	void bind(Result(*function)())
	{
		bind(new Static<Result>(function));
	}

	// Binds a static function that takes one argument.
	template <typename Result, typename Arg>
	void bind(Result(*function)(Arg), Arg arg)
	{
		bind(new StaticWithArg<Result, Arg>(function, arg));
	}

	// Binds a member function that takes no arguments.
	template <typename Result, typename Object>
	void bind(Object* obj, Result(Object::*member)())
	{
		bind(new Member<Result, Object>(obj, member));
	}

	// Binds a member function that takes one argument.
	template <typename Result, typename Object, typename Arg>
	void bind(Object* obj, Result(Object::*member)(Arg), Arg arg)
	{
		bind(new MemberWithArg<Result, Object, Arg>(obj, member, arg));
	}

private:
	Action* myAction;
};

*/

} // namespace vortex