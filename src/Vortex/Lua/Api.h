#pragma once

#include <Vortex/Lua/Types.h>

namespace AV {
namespace Lua {

// Api/Core.cpp
void initializeCoreApi(TypeApi& index);

// Api/Editor.cpp
void initializeEditorApi(TypeApi& index);

// Api/Simfile.cpp
void initializeSimfileApi(TypeApi& index, TypeApi& newIndex);

// Api/Chart.cpp
void initializeChartApi(TypeApi& index, TypeApi& newIndex);

// Api/Tempo.cpp
void initializeTempoApi(TypeApi& index, TypeApi& newIndex);

} // namespace Lua
} // namespace AV