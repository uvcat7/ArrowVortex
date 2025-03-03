#pragma once

//#include <vld.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <algorithm>
#include <array>
#include <compare>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <format>

namespace AV {

using std::unique_ptr;
using std::shared_ptr;
using std::vector;

typedef unsigned long  ulong;
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef const std::string& stringref;

typedef int Row;

class Texture;

class Color;
class Colorf;
class Font;
class FontData;

struct TileBar;
struct TileRect;
struct TileRect2;
class Deserializer;
class Serializer;
struct XmrDoc;

class IHistory;
class History;
class NoteCollection;
class NoteSet;
class Tempo;
class Chart;
class Chart;
class Simfile;
class SimfileMetadata;
class TimingData;
struct GameMode;

} // namespace AV