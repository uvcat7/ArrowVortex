#include <Precomp.h>

#include <Core/System/Log.h>

#include <Core/Utils/Enum.h>

#include <Vortex/Lua/Api.h>
#include <Vortex/Lua/Global.h>
#include <Vortex/Lua/Utils.h>

#include <Simfile/Tempo.h>
#include <Simfile/Tempo/SegmentUtils.h>

using namespace std;

namespace AV {
namespace Lua {

// =====================================================================================================================
// Tempo userdata

static int CollectTempo(lua_State* L);

static const Metatable TempoMetatable {
	.name = "Tempo",
	.index = Lua::invokeTempoIndex,
	.newIndex = Lua::invokeTempoNewIndex,
	.gc = CollectTempo,
};

static int CollectTempo(lua_State* L)
{
	auto tempo = Lua::getArgs<Tempo*>(L);
	if (!getExtraSpace(L)->tempos.remove(tempo))
		Log::error("Tempo __gc invoked on invalid ref");
	return 0;
}

template <>
int Lua::push<shared_ptr<Tempo>>(lua_State* L, const shared_ptr<Tempo>& tempo)
{
	auto ptr = tempo.get();
	if (!ptr) return 0;
	getExtraSpace(L)->tempos.add(tempo);
	return Lua::pushPtrUserdata(L, ptr, TempoMetatable);
}

template <>
Tempo* Lua::get<Tempo*>(lua_State* L, int index)
{
	auto tempo = Lua::getPtrUserdata(L, index, TempoMetatable);
	if (!getExtraSpace(L)->tempos.refs.contains(tempo))
		luaL_error(L, "get Tempo invoked on invalid ref");
	return (Tempo*)tempo;
}

// =====================================================================================================================
// Tempo API

#define FN(name) index.addFunc(#name) = [](lua_State* L)
#define GET(name) index.addProp(#name) = [](lua_State* L)
#define SET(name) newIndex.addProp(#name) = [](lua_State* L)

void Lua::initializeTempoApi(TypeApi& index, TypeApi& newIndex)
{
	FN(setBpm) {
		auto [tempo, row, bpm] = Lua::getArgs<Tempo*, int, double>(L);
		SetSegment(tempo->bpmChanges, row, { .bpm = bpm });
		return 0;
	};

	FN(getBpm) {
		auto [tempo, row] = Lua::getArgs<Tempo*, int>(L);
		lua_pushnumber(L, tempo->bpmChanges.get(row).bpm);
		return 1;
	};

	FN(setStop) {
		auto [tempo, row, len] = Lua::getArgs<Tempo*, int, double>(L);
		SetSegment(tempo->stops, row, { .seconds = len });
		return 0;
	};

	GET(offset) {
		auto [tempo, _] = Lua::getArgs<Tempo*, nullptr_t>(L);
		lua_pushnumber(L, tempo->offset.get());
		return 1;
	};

	SET(offset) {
		auto [tempo, _, value] = Lua::getArgs<Tempo*, nullptr_t, double>(L);
		tempo->offset.set(value);
		return 0;
	};

	FN(rowToTime) {
		auto [tempo, row] = Lua::getArgs<Tempo*, double>(L);
		lua_pushnumber(L, tempo->timing.rowToTime(row));
		return 1;
	};

	FN(timeToRow) {
		auto [tempo, row] = Lua::getArgs<Tempo*, double>(L);
		lua_pushnumber(L, tempo->timing.timeToRow(row));
		return 1;
	};

	FN(setDisplayBpm) {
		auto tempo = Lua::get<Tempo*>(L, 1);
		auto type = Lua::get<DisplayBpmType>(L, 2);
		switch (type)
		{
		case DisplayBpmType::Actual:
			Lua::checkArgCount(L, 2);
			tempo->displayBpm.set(DisplayBpm::actual);
			break;
		case DisplayBpmType::Custom: {
			Lua::checkArgCount(L, 4);
			auto low = Lua::get<double>(L, 3);
			auto high = Lua::get<double>(L, 4);
			tempo->displayBpm.set(DisplayBpm(low, high, DisplayBpmType::Custom));
			break; }
		case DisplayBpmType::Random:
			Lua::checkArgCount(L, 2);
			tempo->displayBpm.set(DisplayBpm::random);
			break;
		}
		return 0;
	};
}

} // namespace Lua
} // namespace AV