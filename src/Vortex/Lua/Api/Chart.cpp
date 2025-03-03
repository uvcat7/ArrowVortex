#include <Precomp.h>

#include <Core/System/Log.h>

#include <Core/Utils/Enum.h>

#include <Simfile/Chart.h>

#include <Vortex/Lua/Api.h>
#include <Vortex/Lua/Global.h>
#include <Vortex/Lua/Utils.h>

using namespace std;

namespace AV {
namespace Lua {


// =====================================================================================================================
// Chart userdata

static int CollectChart(lua_State* L);

static const Metatable ChartMetatable{
	.name = "Chart",
	.index = Lua::invokeChartIndex,
	.newIndex = Lua::invokeChartNewIndex,
	.gc = CollectChart,
};

static int CollectChart(lua_State* L)
{
	auto chart = Lua::getArgs<Chart*>(L);
	if (!getExtraSpace(L)->charts.remove(chart))
		Log::error("Chart __gc invoked on invalid ref");
	return 0;
}

template <>
int Lua::push<shared_ptr<Chart>>(lua_State* L, const shared_ptr<Chart>& chart)
{
	auto ptr = chart.get();
	if (!ptr) return 0;
	getExtraSpace(L)->charts.add(chart);
	return Lua::pushPtrUserdata(L, ptr, ChartMetatable);
}

template <>
Chart* Lua::get<Chart*>(lua_State* L, int index)
{
	auto chart = Lua::getPtrUserdata(L, index, ChartMetatable);
	if (!getExtraSpace(L)->charts.refs.contains(chart))
		luaL_error(L, "get Chart invoked on invalid ref");
	return (Chart*)chart;
}

// Helpers for getting/setting observable members of chart:

struct ChartHelper
{
	ChartHelper(Lua::TypeApi& index, Lua::TypeApi& newIndex)
		: index(index), newIndex(newIndex) {}

	template <typename T, Observable<T>(Chart::* M)>
	static int get(lua_State* L)
	{
		auto [chart, _] = Lua::getArgs<Chart*, nullptr_t>(L);
		Lua::push<T>(L, (chart->*M).get());
		return 1;
	}

	template <typename T, Observable<T>(Chart::* M)>
	static int set(lua_State* L)
	{
		auto [chart, _, value] = Lua::getArgs<Chart*, nullptr_t, T>(L);
		(chart->*M).set(value);
		return 0;
	}

	template <typename T, Observable<T>(Chart::* M)>
	void add(const char* key)
	{
		index.addProp(key) = get<T, M>;
		newIndex.addProp(key) = set<T, M>;
	}

	Lua::TypeApi& index;
	Lua::TypeApi& newIndex;
};

// Helpers for adding notes:

static int AddNote(lua_State* L, NoteType type)
{
	bool hasEnd = type == NoteType::Hold || type == NoteType::Roll;
	Lua::checkArgCount(L, hasEnd ? 4 : 3);

	auto chart = Lua::get<Chart*>(L, 1);
	auto col = Lua::get<int>(L, 2);

	if (col < 1 || col > chart->notes.numColumns())
		return luaL_error(L, "invalid column: %d", col);

	auto row = Lua::get<int>(L, 3);
	if (row < 0)
		return luaL_error(L, "invalid row: %d", row);

	Row endRow = row;
	if (type == NoteType::Hold || type == NoteType::Roll)
	{
		endRow = Lua::get<int>(L, 4);
		if (endRow <= row)
			return luaL_error(L, "invalid end row: %d", endRow);
	}

	// TODO: implement properly
	chart->notes
		.column(col - 1) // 1-based to 0-based.
		.append(Note(row, endRow, type, 0, 0));

	return 0;
}

struct NoteRowIter : Iterator
{
	int next(lua_State* L) override
	{
		// if (index >= charts.size()) return 0;
		// Lua::push(L, charts[index++]);
		// return 1;
	}
	shared_ptr<Chart> chart;
	Row row = 0;
};

// =====================================================================================================================
// Chart API

#define FUN(name) index.addFunc(#name) = [](lua_State* L)
#define GET(name) index.addProp(#name) = [](lua_State* L)
#define SET(name) newIndex.addProp(#name) = [](lua_State* L)

static Chart* GetChartArg(lua_State* L)
{
	auto [chart, _] = Lua::getArgs<Chart*, nullptr_t>(L);
	return chart;
}

void Lua::initializeChartApi(TypeApi& index, TypeApi& newIndex)
{
	Lua::ChartHelper helper(index, newIndex);

	// Chart properties

	helper.add<string, &Chart::name>("name");
	helper.add<string, &Chart::style>("style");

	helper.add<string, &Chart::author>("author");
	helper.add<string, &Chart::description>("description");

	helper.add<Difficulty, &Chart::difficulty>("difficulty");
	helper.add<int, &Chart::meter>("meter");

	GET(tempo) {
		return Lua::push(L, GetChartArg(L)->tempo);
	};

	GET(endRow) {
		int endRow = 0;
		for (auto& col : GetChartArg(L)->notes)
			endRow = max(endRow, col.endRow());
		return Lua::push<int>(L, endRow);
	};

	GET(noteRows) {
		auto chart = GetChartArg(L);
		return 1;
	};

	// Note functions

	FUN(addStep) {
		AddNote(L, NoteType::Step);
		return 0;
	};

	FUN(addHold) {
		AddNote(L, NoteType::Hold);
		return 0;
	};
}

} // namespace Lua
} // namespace AV