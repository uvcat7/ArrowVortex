#include <Precomp.h>

#include <list>

#include <Core/Utils/Util.h>
#include <Core/System/Log.h>
#include <Core/Common/Event.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// State

using Subscription = EventSubscriptions::Subscription;

namespace EventSystem
{
	struct State
	{
		map<EventId, list<Subscription*>> subscriptions;
	};
	static State* state = nullptr;
}

void EventSystem::initialize(const XmrDoc& settings)
{
	state = new State();
}

void EventSystem::deinitialize()
{
	Util::reset(state);
}

// =====================================================================================================================
// Subscriptions

using EventSystem::state;

EventSubscriptions::Subscription::Subscription(EventId id)
	: id(id)
{
}

EventSubscriptions::Subscription::~Subscription()
{
	if (state)
	{
		auto it = state->subscriptions.find(id);
		if (it != state->subscriptions.end())
		{
			if (it->second.remove(this) != 1)
				Log::error("Could not remove event subscription");

			if (it->second.empty())
				state->subscriptions.erase(it);
		}
		else
		{
			Log::error("Could not remove event subscription");
		}
	}
	else
	{
		Log::error("Dangling subscription after event system was deinitialized.");
	}
}

void EventSubscriptions::add(EventId id, void (*handler)())
{
	struct Sub : EventSubscriptions::Subscription
	{
		typedef void(*Func)();
		Sub(EventId id, Func f) : f(f), Subscription(id) {}
		void publish(void*) override { (*f)(); }
		Func f;
	};
	subscribe(std::make_unique<Sub>(id, handler));
}

void EventSubscriptions::clear()
{
	mySubscriptions.clear();
}

void EventSubscriptions::subscribe(unique_ptr<Subscription> sub)
{
	EventSystem::state->subscriptions[sub->id].emplace_back(sub.get());
	mySubscriptions.emplace_back().swap(sub);
}

void EventSystem::publish(EventId id)
{
	EventSystem::Internal::publish(id, nullptr);
}

void EventSystem::Internal::publish(EventId id, void* arg)
{
	auto it = state->subscriptions.find(id);
	if (it != state->subscriptions.end())
	{
		for (auto sub : it->second)
			sub->publish(arg);
	}
}

} // namespace AV