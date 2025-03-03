#pragma once

#include <Core/Common/NonCopyable.h>

namespace AV {

typedef void (*EventId)();

struct Event {};

// Manages event subscriptions.
class EventSubscriptions : NonCopyable
{
public:
	struct Subscription;

	// Subscribes a static function without argument.
	template <typename EventType>
	void add(void (*handler)());

	// Subscribes a static function with argument.
	template <typename EventType>
	void add(void (*handler)(const EventType&));

	// Subscribes a member function handler without argument.
	template <typename EventType, typename Target>
	void add(Target* target, void (Target::*handler)());

	// Subscribes a member function with argument.
	template <typename EventType, typename Target>
	void add(Target* target, void (Target::*handler)(const EventType&));

	// Subscribes a static function without argument.
	void add(EventId id, void (*handler)());

	// Subscribes a member function without argument.
	template <typename Target>
	void add(EventId id, Target* target, void (Target::* handler)());

	// Clears all subscriptions.
	void clear();

private:
	void subscribe(unique_ptr<Subscription> sub);
	vector<unique_ptr<Subscription>> mySubscriptions;
};

// Wraps a value and publishes an event whenever that value changes.
template <typename T>
class Observable : NonCopyable
{
public:
	Observable(EventId id);
	Observable(EventId id, T value);
	void set(const T& v);
	const T& get() const;
private:
	EventId myEvent;
	T myValue;
};

namespace EventSystem {

void initialize(const XmrDoc& settings);
void deinitialize();

// Gets a unique identifier for the given event type.
template <typename T>
EventId getId();

// Publishes the given event.
template <typename EventType>
void publish(const EventType& evt);

// Publishes the given event.
template <typename EventType>
void publish();

// Publishes the given event.
void publish(EventId id);

} // namespace EventSystem

// =====================================================================================================================
// Implementation below here

struct EventSubscriptions::Subscription
{
	Subscription(EventId id);
	virtual ~Subscription();
	virtual void publish(void* arg) = 0;
	const EventId id;
};

namespace EventSystem {
namespace Internal {

void publish(EventId id, void* arg);

template <typename T>
struct Id { static void id() {} };

} // namespace Internal

template <typename T>
EventId getId()
{
	static_assert(std::is_base_of<Event, T>::value);
	return &EventSystem::Internal::Id<T>::id;
}

template <typename ET>
void publish(const ET& arg)
{
	Internal::publish(getId<ET>(), (void*)&arg);
}

template <typename ET>
void publish()
{
	Internal::publish(getId<ET>(), nullptr);
}

} // namespace EventSystem

template <typename ET>
void EventSubscriptions::add(void (*handler)())
{
	add(EventSystem::getId<ET>(), handler);
}

template <typename ET>
void EventSubscriptions::add(void (*handler)(const ET&))
{
	struct Sub : Subscription
	{
		typedef void (*Func)(const ET&);
		Sub(EventId id, Func f) : f(f), Subscription(id) {}
		void publish(void* e) override { (*f)(*(ET*)e); }
		const Func f;
	};
	subscribe(std::make_unique<Sub>(EventSystem::getId<ET>(), handler));
}

template <typename ET, typename Target>
void EventSubscriptions::add(Target* target, void (Target::*handler)())
{
	add(EventSystem::getId<ET>(), target, handler);
}

template <typename ET, typename Target>
void EventSubscriptions::add(Target* target, void (Target::*handler)(const ET&))
{
	struct Sub : Subscription
	{
		typedef void (Target::*Func)();
		Sub(EventId id, Target* t, Func f) : t(t), f(f), Subscription(id) {}
		void publish(void* e) override { (t->*f)(*(ET*)e); }
		const Func f;
		Target* t;
	};
	subscribe(std::make_unique<Sub>(EventSystem::getId<ET>(), target, handler));
}

template <typename Target>
void EventSubscriptions::add(EventId id, Target* target, void (Target::* handler)())
{
	struct Sub : Subscription
	{
		typedef void (Target::* Func)();
		Sub(EventId id, Target* t, Func f) : t(t), f(f), Subscription(id) {}
		void publish(void*) override { (t->*f)(); }
		const Func f;
		Target* t;
	};
	subscribe(std::make_unique<Sub>(id, target, handler));
}

template <typename T>
Observable<T>::Observable(EventId id)
	: myEvent(id), myValue(T())
{
}

template <typename T>
Observable<T>::Observable(EventId id, T value)
	: myEvent(id), myValue(value)
{
}

template <typename T>
void Observable<T>::set(const T& v)
{
	if (myValue != v)
	{
		myValue = v;
		EventSystem::publish(myEvent);
	}
}

template <typename T>
const T& Observable<T>::get() const
{
	return myValue;
}

} // namespace AV