#include <Precomp.h>

#include <Core/System/Log.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/Xmr.h>
#include <Core/Utils/String.h>

#include <Simfile/Simfile.h>

#include <Vortex/Lua/Global.h>
#include <Vortex/Lua/Utils.h>
#include <Vortex/Lua/Api.h>

using namespace std;

namespace AV {
namespace Lua {

// =====================================================================================================================
// Static data

struct State
{
	Lua::TypeApi avIndex;

	Lua::TypeApi simfileIndex;
	Lua::TypeApi simfileNewIndex;

	Lua::TypeApi chartIndex;
	Lua::TypeApi chartNewIndex;

	Lua::TypeApi tempoIndex;
	Lua::TypeApi tempoNewIndex;
};

static State* state = nullptr;

// =====================================================================================================================
// Initialization

void Lua::initialize(const XmrDoc& settings)
{
	state = new State();

	Lua::initializeCoreApi(state->avIndex);
	Lua::initializeEditorApi(state->avIndex);
	Lua::initializeSimfileApi(state->simfileIndex, state->simfileNewIndex);
	Lua::initializeChartApi(state->chartIndex, state->chartNewIndex);
	Lua::initializeTempoApi(state->tempoIndex, state->tempoNewIndex);
}

void Lua::deinitialize()
{
	Util::reset(state);
}

// =====================================================================================================================
// API invocation

int invokeAvIndex(lua_State* L)
{
	return state->avIndex.invoke(L);
}

int invokeSimfileIndex(lua_State* L)
{
	return state->simfileIndex.invoke(L);
}

int invokeSimfileNewIndex(lua_State* L)
{
	return state->simfileNewIndex.invoke(L);
}

int invokeChartIndex(lua_State* L)
{
	return state->chartIndex.invoke(L);
}

int invokeChartNewIndex(lua_State* L)
{
	return state->chartNewIndex.invoke(L);
}

int invokeTempoIndex(lua_State* L)
{
	return state->tempoIndex.invoke(L);
}

int invokeTempoNewIndex(lua_State* L)
{
	return state->tempoNewIndex.invoke(L);
}

// =====================================================================================================================
// Importers and exporters

struct SimfileImporter : public SimfileFormats::Importer
{
	FilePath path;
	string func;
	SimfileFormats::LoadResult load(const FilePath& path) const override;
};

struct SimfileExporter : public SimfileFormats::Exporter
{
	FilePath path;
	string func;
	SimfileFormats::SaveResult save(const Simfile& sim, bool backup) const override;
};

static bool PushMainFunc(lua_State* L, const FilePath& path, stringref func, string& err)
{
	if (!Lua::runFile(L, path.str))
	{
		err = format("error while running '{}'", path.str);
		return false;
	}

	if (lua_getglobal(L, func.data()) == LUA_TNIL)
	{
		err = format("function '{}' in '{}' does not exist", func, path.str);
		return false;
	}

	return true;
}

SimfileFormats::LoadResult SimfileImporter::load(const FilePath& simfilePath) const
{
	auto state = Lua::createState();
	auto L = state.get();

	string err;
	if (!PushMainFunc(L, path, func, err))
		return err;

	auto sim = make_shared<Simfile>();

	lua_createtable(L, 0, 2);
	Lua::push(L, simfilePath.str);
	lua_setfield(L, -2, "path");
	Lua::push(L, sim);
	lua_setfield(L, -2, "simfile");

	if (!Lua::safeCall(L, 1, 1))
		return "Error while executing importer script";

	return sim;
}

SimfileFormats::SaveResult SimfileExporter::save(const Simfile& sim, bool backup) const
{
	auto state = Lua::createState();
	auto L = state.get();

	string err;
	if (!PushMainFunc(L, path, func, err))
		return err;

	// TODO: this is pretty sketch, refactor at some point.
	struct DummyDeleter { void operator()(Simfile*) {} };
	shared_ptr<Simfile> simPtr((Simfile*)&sim, DummyDeleter());

	lua_createtable(L, 0, 1);
	Lua::push(L, simPtr);
	lua_setfield(L, -2, "simfile");

	auto success = Lua::safeCall(L, 1, 1);
	state.reset();

	VortexAssert(simPtr.use_count() == 1);

	if (!success)
		return "Error while executing importer script";

	return {};
}

static void LoadImportersExporters(
	vector<unique_ptr<SimfileFormats::Importer>>& importers,
	vector<unique_ptr<SimfileFormats::Exporter>>& exporters,
	DirectoryPath& dir,
	FilePath& metadata)
{
	XmrDoc doc;
	if (doc.loadFile(metadata) != XmrResult::Success)
	{
		Log::error(doc.lastError);
		return;
	}

	for (auto it : doc.children("importer"))
	{
		auto fmt = it->get("format");
		auto ext = String::split(it->get("extensions", ""), ",");
		auto file = it->get("file");
		auto func = it->get("function");
		if (fmt && file && func && !ext.empty())
		{
			auto script = make_unique<SimfileImporter>();
			script->fmt = fmt;
			script->extensions = ext;
			script->path = FilePath(dir, file);
			script->func = func;
			importers.push_back(move(script));
		}
		else
		{
			Log::error(format("Invalid importer definition in: {}", metadata.str));
		}
	}

	for (auto it : doc.children("exporter"))
	{
		auto fmt = it->get("format");
		auto ext = String::split(it->get("extensions", ""), ",");
		auto file = it->get("file");
		auto func = it->get("function");
		if (fmt && file && func && !ext.empty())
		{
			auto script = make_unique<SimfileExporter>();
			script->fmt = fmt;
			script->extensions = ext;
			script->path = FilePath(dir, file);
			script->func = func;
			exporters.push_back(move(script));
		}
		else
		{
			Log::error(format("Invalid exporter definition in: {}", metadata.str));
		}
	}
}

void Lua::loadImportersExporters(
	vector<unique_ptr<SimfileFormats::Importer>>& outImporters,
	vector<unique_ptr<SimfileFormats::Exporter>>& outExporters)
{
	auto root = DirectoryPath("lua/formats");
	for (auto& dir : FileSystem::listDirectories(root))
	{
		auto fullDir = DirectoryPath(root, dir);
		auto metadata = FilePath(fullDir, "metadata.txt");
		LoadImportersExporters(outImporters, outExporters, fullDir, metadata);
	}
}

} // namespace Lua
} // namespace AV