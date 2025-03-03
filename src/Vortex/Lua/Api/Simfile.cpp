#include <Precomp.h>

#include <Core/System/Log.h>

#include <Simfile/Simfile.h>

#include <Vortex/Lua/Api.h>
#include <Vortex/Lua/Global.h>
#include <Vortex/Lua/Utils.h>

#include <Vortex/Managers/GameMan.h>

using namespace std;

namespace AV {
namespace Lua {

// Helpers for getting/setting observable members of simfile:

struct SimfileHelper
{
	SimfileHelper(Lua::TypeApi& index, Lua::TypeApi& newIndex)
		: index(index), newIndex(newIndex) {}

	template <typename T, Observable<T>(Simfile::* M)>
	static int get(lua_State* L)
	{
		auto [chart, _] = Lua::getArgs<Simfile*, nullptr_t>(L);
		Lua::push<T>(L, (chart->*M).get());
		return 1;
	}

	template <typename T, Observable<T>(Simfile::* M)>
	static int set(lua_State* L)
	{
		auto [chart, _, value] = Lua::getArgs<Simfile*, nullptr_t, T>(L);
		(chart->*M).set(value);
		return 0;
	}

	template <typename T, Observable<T>(Simfile::* M)>
	void add(const char* key)
	{
		index.addProp(key) = get<T, M>;
		newIndex.addProp(key) = set<T, M>;
	}

	Lua::TypeApi& index;
	Lua::TypeApi& newIndex;
};

struct ChartIter : Iterator
{
	int next(lua_State* L) override
	{
		if (index >= charts.size()) return 0;
		Lua::push(L, charts[index++]);
		return 1;
	}
	vector<shared_ptr<Chart>> charts;
	size_t index = 0;
};

// =====================================================================================================================
// Simfile userdata

static int CollectSimfile(lua_State* L);

static const Metatable SimfileMetatable {
	.name = "Simfile",
	.index = Lua::invokeSimfileIndex,
	.newIndex = Lua::invokeSimfileNewIndex,
	.gc = CollectSimfile,
};

static int CollectSimfile(lua_State* L)
{
	auto sim = Lua::getArgs<Simfile*>(L);
	auto es = getExtraSpace(L);
	if (!es->simfiles.remove(sim))
		Log::error("Simfile __gc invoked on invalid ref");
	return 0;
}

template <>
int Lua::push<shared_ptr<Simfile>>(lua_State* L, const shared_ptr<Simfile>& simfile)
{
	auto ptr = simfile.get();
	if (!ptr) return 0;
	getExtraSpace(L)->simfiles.add(simfile);
	return Lua::pushPtrUserdata(L, ptr, SimfileMetatable);
}

template <>
Simfile* Lua::get<Simfile*>(lua_State* L, int index)
{
	auto sim = Lua::getPtrUserdata(L, index, SimfileMetatable);
	if (!getExtraSpace(L)->simfiles.refs.contains(sim))
		luaL_error(L, "get Simfile invoked on invalid ref");
	return (Simfile*)sim;
}

// =====================================================================================================================
// Simfile API

#define FUN(name) index.addFunc(#name) = [](lua_State* L)
#define GET(name) index.addProp(#name) = [](lua_State* L)
#define SET(name) newIndex.addProp(#name) = [](lua_State* L)

static Simfile* GetSimfileArg(lua_State* L)
{
	auto [sim, _] = Lua::getArgs<Simfile*, nullptr_t>(L);
	return sim;
}

void Lua::initializeSimfileApi(TypeApi& index, TypeApi& newIndex)
{
	SimfileHelper helper(index, newIndex);

	// Simfile properties

	helper.add<string, &Simfile::title>("title");
	helper.add<string, &Simfile::titleTranslit>("titleTranslit");

	helper.add<string, &Simfile::subtitle>("subtitle");
	helper.add<string, &Simfile::subtitleTranslit>("subtitleTranslit");

	helper.add<string, &Simfile::artist>("artist");
	helper.add<string, &Simfile::artistTranslit>("artistTranslit");

	helper.add<string, &Simfile::genre>("genre");
	helper.add<string, &Simfile::credit>("credit");

	helper.add<string, &Simfile::music>("music");

	helper.add<string, &Simfile::banner>("banner");
	helper.add<string, &Simfile::background>("background");
	helper.add<string, &Simfile::cdTitle>("cdTitle");
	helper.add<string, &Simfile::lyricsPath>("lyricsPath");

	GET(directory) {
		return Lua::push(L, GetSimfileArg(L)->directory.str);
	};

	GET(filename) {
		return Lua::push(L, GetSimfileArg(L)->filename);
	};

	GET(tempo) {
		return Lua::push(L, GetSimfileArg(L)->tempo);
	};

	GET(charts) {
		auto sim = GetSimfileArg(L);
		auto& iter = Lua::Iterator::push<Lua::ChartIter>(L);
		iter.charts = sim->charts;
		return 1;
	};

	FUN(addChart) {
		auto [sim, id] = Lua::getArgs<Simfile*, string>(L);
		auto mode = GameModeMan::find(id);
		if (!mode)
			return luaL_error(L, "invalid game mode: %s.", id.data());

		auto chart = make_shared<Chart>(sim, mode);
		sim->addChart(chart);
		Lua::push(L, chart);
		return 1;
	};
}

} // namespace Lua
} // namespace AV