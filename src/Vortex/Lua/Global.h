#pragma once

#include <Precomp.h>

#include <lua/src/lua.hpp>

#include <Simfile/Formats.h>

namespace AV {
namespace Lua {

void initialize(const XmrDoc& settings);
void deinitialize();

// Static APIs.

int invokeAvIndex(lua_State* L);

int invokeSimfileIndex(lua_State* L);
int invokeSimfileNewIndex(lua_State* L);

int invokeChartIndex(lua_State* L);
int invokeChartNewIndex(lua_State* L);

int invokeTempoIndex(lua_State* L);
int invokeTempoNewIndex(lua_State* L);

// Loads the lua-based simfile importers and stores them in the out vector.
void loadImportersExporters(
	vector<unique_ptr<SimfileFormats::Importer>>& outImporters,
	vector<unique_ptr<SimfileFormats::Exporter>>& outExporters);

} // namespace Lua
} // namespace AV