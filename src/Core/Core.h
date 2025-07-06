#pragma once

#include <cstdint>
#include <chrono>

namespace Vortex {

typedef uint64_t  ulong;
typedef uint32_t   uint;
typedef uint16_t ushort;
typedef uint8_t  uchar;
typedef uint32_t   color32;

template <typename T> struct vec2t { T x, y; };
template <typename T> struct vec3t { T x, y, z; };
template <typename T> struct rectt { T x, y, w, h; };
template <typename T> struct areat { T l, t, r, b; };

typedef vec2t<int>    vec2i;
typedef vec2t<float>  vec2f;
typedef vec2t<double> vec2d;
typedef vec3t<float>  vec3f;
typedef rectt<int>    recti;
typedef rectt<float>  rectf;
typedef areat<int>    areai;
typedef areat<float>  areaf;

struct colorf { float r, g, b, a; };

class String;
struct TileBar;
struct TileRect;
struct TileRect2;
struct ReadStream;
struct WriteStream;
class GuiContext;
struct XmrNode;
struct TempoSelection;
struct Tempo;
class NoteList;
class SegmentGroup;
class SegmentList;
struct SegmentEdit;
struct Note;
struct NoteEdit;
struct Chart;
struct Style;
struct Noteskin;
struct Simfile;
struct TimingData;

typedef const String& StringRef;
typedef uint64_t TextureHandle;

extern std::chrono::duration<double> deltaTime;

extern void HudNote(const char* fmt, ...);
extern void HudInfo(const char* fmt, ...);
extern void HudWarning(const char* fmt, ...);
extern void HudError(const char* fmt, ...);

}; // namespace Vortex
