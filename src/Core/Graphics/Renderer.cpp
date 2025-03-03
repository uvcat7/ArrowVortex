#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/System/Debug.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Shader.h>
#include <Core/Graphics/Renderer.h>
#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Canvas.h>
#include <Core/Graphics/Texture.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Renderer :: Static data.

// Batch rendering.

typedef uint32_t GLuint;
typedef int32_t GLint;
typedef int32_t GLenum;
typedef float GLfloat;

static constexpr int NumShaders = 4;
static constexpr int BatchLimit = Renderer::Constants::BatchLimit;

static constexpr int IndicesPerQuad = 6;
static constexpr int XYCoordPerQuad = 8;
static constexpr int UVCoordsPerQuad = 8;
static constexpr int ColorsPerQuad = 4;

static const size_t QuadIndexDataSize = IndicesPerQuad * sizeof(GLuint);
static const size_t QuadPositionDataSize = XYCoordPerQuad * max(sizeof(GLfloat), sizeof(GLint));
static const size_t QuadTexCoordDataSize = UVCoordsPerQuad * sizeof(GLfloat);
static const size_t QuadColorDataSize = ColorsPerQuad * sizeof(Color);

static uint QuadIndices[BatchLimit * QuadIndexDataSize];
static uchar VertexPositions[BatchLimit * QuadPositionDataSize];
static uchar VertexTexCoords[BatchLimit * QuadTexCoordDataSize];
static uchar VertexColors[BatchLimit * QuadColorDataSize];

static Renderer::FillType CurrentFillType = Renderer::FillType::PlainQuads;
static Renderer::VertexType CurrentVertexType = Renderer::VertexType::Int;

static int NumQuadsInBatch = 0;

namespace Renderer
{
	struct State
	{
		ShaderInstance shaders[NumShaders];
		ShaderInstance* currentShader = nullptr;
		vector<Rect> scissorStack;
		TileRect roundedBox;
	};
	static State* state = nullptr;
}
using Renderer::state;

// =====================================================================================================================
// Renderer :: batch rendering.

#define ASSERT_BATCH_IS_EMPTY()\
	VortexAssertf(NumQuadsInBatch == 0,\
		"The render state was changed while a batch is still waiting to be rendered.")

#define ASSERT_CORRECT_BATCH(fill, vertex)\
	VortexAssertf(CurrentFillType == fill && CurrentVertexType == vertex,\
		"Quads were pushed for a batch type that is not currently active.")

#define ASSERT_AMOUNT_IS_WITHIN_BATCH_LIMIT(x)\
	VortexAssertf(x >= 0 && x < BatchLimit,\
		"A maximum of %i quads can be pushed in a single operation.", BatchLimit)

static void InitializeQuadIndices()
{
	for (uint* p = QuadIndices, i = 0; i < BatchLimit * 4; i += 4)
	{
		*p = i + 0; ++p;
		*p = i + 3; ++p;
		*p = i + 1; ++p;
		*p = i + 0; ++p;
		*p = i + 2; ++p;
		*p = i + 3; ++p;
	}
}

// =====================================================================================================================
// Renderer :: public API.

void Renderer::initialize()
{
	state = new State();

	InitializeQuadIndices();

	Canvas roundedBox(16, 16, 1.0f);
	roundedBox.setColor(1.0f);
	roundedBox.box(0, 0, 16, 16, 4.0f);

	state->roundedBox.texture = roundedBox.createTexture("rounded box", false);
	state->roundedBox.border = 6;
}

void Renderer::deinitialize()
{
	Util::reset(state);
}

// =====================================================================================================================
// Renderer :: render state.

void Renderer::setColor(const Colorf& color)
{
	ASSERT_BATCH_IS_EMPTY();
	// glColor4f(color.red, color.green, color.blue, color.alpha);
}

void Renderer::setColor(Color color)
{
	ASSERT_BATCH_IS_EMPTY();
	// glColor4ub(color.red(), color.green(), color.blue(), color.alpha());
}

void Renderer::resetColor()
{
	ASSERT_BATCH_IS_EMPTY();
	// glColor4ub(255, 255, 255, 255);
}

void Renderer::bindTexture(const Texture& texture)
{
	bindTexture(&texture);
}

void Renderer::bindTexture(const Texture* texture)
{
	ASSERT_BATCH_IS_EMPTY();

	// if (texture)
	// 	glBindTexture(GL_TEXTURE_2D, texture->handle());
	// else
	// 	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::unbindTexture()
{
	ASSERT_BATCH_IS_EMPTY();

	// glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::bindShader(Shader shader)
{
	ASSERT_BATCH_IS_EMPTY();

	// if (Glx::supportsShaders)
	// {
	// 	state->currentShader = state->shaders + (int)shader;
	// 	Glx::glUseProgram(state->currentShader->getProgram());
	// }
}


void Renderer::bindTextureAndShader(const Texture& texture)
{
	bindTextureAndShader(&texture);
}

void Renderer::bindTextureAndShader(const Texture* texture)
{
	ASSERT_BATCH_IS_EMPTY();

	if (texture != nullptr)
	{
		bindShader(texture->format() == TextureFormat::Alpha
			? Shader::TextureAlpha
			: Shader::Texture);

		// glBindTexture(GL_TEXTURE_2D, texture->handle());
	}
	else
	{
		bindShader(Shader::Color);
		// glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void Renderer::setBlendMode(BlendMode mode)
{
	ASSERT_BATCH_IS_EMPTY();

	switch (mode)
	{
	case BlendMode::Alpha:
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;

	case BlendMode::Add:
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	}
}

void Renderer::resetBlendMode()
{
	ASSERT_BATCH_IS_EMPTY();

	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::pushScissorRect(const Rect& r)
{
	ASSERT_BATCH_IS_EMPTY();

	pushScissorRect(r.l, r.t, r.r, r.b);
}

void Renderer::pushScissorRect(int lx, int ty, int rx, int by)
{
	ASSERT_BATCH_IS_EMPTY();

	auto& stack = state->scissorStack;
	rx = max(rx, lx);
	by = max(by, ty);
	if (stack.empty())
	{
		// The new scissor region is the first scissor region, use it as-is.
		// glEnable(GL_SCISSOR_TEST);
	}
	else if (stack.size() < 256)
	{
		// Calculate the intersection of the current and new scissor region.
		Rect last = stack.back();
		lx = max(lx, last.l);
		ty = max(ty, last.t);
		rx = min(rx, last.r);
		by = min(by, last.b);
	}

	// Apply the new scissor region.
	stack.push_back(Rect(lx, ty, rx, by));
	Size2 view = Window::getSize();
	// glScissor(lx, view.h - by, rx - lx, by - ty);
}

void Renderer::popScissorRect()
{
	ASSERT_BATCH_IS_EMPTY();

	auto& stack = state->scissorStack;
	if (stack.size() == 1)
	{
		stack.pop_back();
		// glDisable(GL_SCISSOR_TEST);
	}
	else if (stack.size() > 0)
	{
		stack.pop_back();
		Rect r = stack.back();
		Size2 view = Window::getSize();
		// glScissor(r.l, view.h - r.b, r.w(), r.h());
	}
}

// =====================================================================================================================
// Renderer :: draw functions.

static constexpr int QuadIndicesPerBatch = BatchLimit * IndicesPerQuad;
static constexpr int QuadXYCoordsPerBatch = BatchLimit * XYCoordPerQuad;
static constexpr int QuadUVCoordsPerBatch = BatchLimit * UVCoordsPerQuad;
static constexpr int QuadColorsPerBatch = BatchLimit * ColorsPerQuad;

template <typename T>
static void DrawQuads(int numQuads, const T* pos, GLenum glVertexType)
{
	ASSERT_BATCH_IS_EMPTY();

	// glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	// glDisableClientState(GL_COLOR_ARRAY);
	// glVertexPointer(2, glVertexType, 0, pos);
	// for (; numQuads > BatchLimit; numQuads -= BatchLimit)
	// {
	// 	glDrawElements(GL_TRIANGLES, BatchLimit * IndicesPerQuad, GL_UNSIGNED_INT, QuadIndices);
	// 	glVertexPointer(2, glVertexType, 0, pos += QuadXYCoordsPerBatch);
	// }
	// glDrawElements(GL_TRIANGLES, numQuads * IndicesPerQuad, GL_UNSIGNED_INT, QuadIndices);
}

void Renderer::drawQuads(int numQuads, const int* pos)
{
	// DrawQuads(numQuads, pos, GL_INT);
}

void Renderer::drawQuads(int numQuads, const float* pos)
{
	// DrawQuads(numQuads, pos, GL_FLOAT);
}

void Renderer::drawQuads(int numQuads, const int* pos, const Color* col)
{
	ASSERT_BATCH_IS_EMPTY();

	// glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	// glEnableClientState(GL_COLOR_ARRAY);

	// glVertexPointer(2, GL_INT, 0, pos);
	// glColorPointer(4, GL_UNSIGNED_BYTE, 0, col);
	// for (; numQuads > BatchLimit; numQuads -= BatchLimit)
	// {
	// 	glDrawElements(GL_TRIANGLES, QuadIndicesPerBatch, GL_UNSIGNED_INT, QuadIndices);
	// 	glVertexPointer(2, GL_INT, 0, pos += QuadXYCoordsPerBatch);
	// 	glColorPointer(4, GL_UNSIGNED_BYTE, 0, col += QuadColorsPerBatch);
	// }
	// glDrawElements(GL_TRIANGLES, numQuads * IndicesPerQuad, GL_UNSIGNED_INT, QuadIndices);
}

void Renderer::drawQuads(int numQuads, const int* pos, const float* uvs)
{
	ASSERT_BATCH_IS_EMPTY();

	// glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// glDisableClientState(GL_COLOR_ARRAY);
	// 
	// glVertexPointer(2, GL_INT, 0, pos);
	// glTexCoordPointer(2, GL_FLOAT, 0, uvs);
	// for (; numQuads > BatchLimit; numQuads -= BatchLimit)
	// {
	// 	glDrawElements(GL_TRIANGLES, QuadIndicesPerBatch, GL_UNSIGNED_INT, QuadIndices);
	// 	glVertexPointer(2, GL_INT, 0, pos += QuadXYCoordsPerBatch);
	// 	glTexCoordPointer(2, GL_FLOAT, 0, uvs += QuadUVCoordsPerBatch);
	// }
	// glDrawElements(GL_TRIANGLES, numQuads * IndicesPerQuad, GL_UNSIGNED_INT, QuadIndices);
}

void Renderer::drawQuads(int numQuads, const int* pos, const float* uvs, const Color* col)
{
	ASSERT_BATCH_IS_EMPTY();

	// glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// glEnableClientState(GL_COLOR_ARRAY);
	// 
	// glVertexPointer(2, GL_INT, 0, pos);
	// glTexCoordPointer(2, GL_FLOAT, 0, uvs);
	// glColorPointer(4, GL_UNSIGNED_BYTE, 0, col);
	// for (; numQuads > BatchLimit; numQuads -= BatchLimit)
	// {
	// 	glDrawElements(GL_TRIANGLES, BatchLimit, GL_UNSIGNED_INT, QuadIndices);
	// 	glVertexPointer(2, GL_INT, 0, pos += QuadXYCoordsPerBatch);
	// 	glTexCoordPointer(2, GL_FLOAT, 0, uvs += QuadUVCoordsPerBatch);
	// 	glColorPointer(4, GL_UNSIGNED_BYTE, 0, col += QuadColorsPerBatch);
	// }
	// glDrawElements(GL_TRIANGLES, numQuads * IndicesPerQuad, GL_UNSIGNED_INT, QuadIndices);
}

void Renderer::drawQuads(int numQuads, const float* pos, const float* uvs, const Color* col)
{
	ASSERT_BATCH_IS_EMPTY();

	// glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// glEnableClientState(GL_COLOR_ARRAY);
	// glVertexPointer(2, GL_FLOAT, 0, pos);
	// glTexCoordPointer(2, GL_FLOAT, 0, uvs);
	// glColorPointer(4, GL_UNSIGNED_BYTE, 0, col);
	// for (; numQuads > BatchLimit; numQuads -= BatchLimit)
	// {
	// 	glDrawElements(GL_TRIANGLES, BatchLimit, GL_UNSIGNED_INT, QuadIndices);
	// 	glVertexPointer(2, GL_FLOAT, 0, pos += QuadXYCoordsPerBatch);
	// 	glTexCoordPointer(2, GL_FLOAT, 0, uvs += QuadUVCoordsPerBatch);
	// 	glColorPointer(4, GL_UNSIGNED_BYTE, 0, col += QuadColorsPerBatch);
	// }
	// glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT, QuadIndices);
}

void Renderer::drawTris(int numTris, const uint* indices, const int* pos)
{
	ASSERT_BATCH_IS_EMPTY();

	// glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	// glDisableClientState(GL_COLOR_ARRAY);
	// glVertexPointer(2, GL_INT, 0, pos);
	// glDrawElements(GL_TRIANGLES, numTris * 3, GL_UNSIGNED_INT, indices);
}

void Renderer::drawTris(int numTris, const uint* indices, const int* pos, const float* uvs)
{
	ASSERT_BATCH_IS_EMPTY();

	// glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// glDisableClientState(GL_COLOR_ARRAY);
	// glVertexPointer(2, GL_INT, 0, pos);
	// glTexCoordPointer(2, GL_FLOAT, 0, uvs);
	// glDrawElements(GL_TRIANGLES, numTris * 3, GL_UNSIGNED_INT, indices);
}

// =====================================================================================================================
// Renderer :: batch functions.

#define GET_CURRENT_QUAD_VERTEX_POS_POINTER(type)\
	(((type*)VertexPositions) + NumQuadsInBatch * XYCoordPerQuad)

#define GET_CURRENT_QUAD_TEX_COORD_POINTER()\
	(((float*)VertexTexCoords) + NumQuadsInBatch * UVCoordsPerQuad)

#define GET_CURRENT_QUAD_COLOR_POINTER()\
	(((Color*)VertexColors) + NumQuadsInBatch * ColorsPerQuad)

#define SET_QUAD_POS_FROM_RECT(type)\
	type* vp = GET_CURRENT_QUAD_VERTEX_POS_POINTER(type);\
	vp[0] = lx;\
	vp[1] = ty;\
	vp[2] = rx;\
	vp[3] = ty;\
	vp[4] = lx;\
	vp[5] = by;\
	vp[6] = rx;\
	vp[7] = by;

#define SET_QUAD_UVS_FROM_UV_ARRAY()\
	float* vt = GET_CURRENT_QUAD_TEX_COORD_POINTER();\
	memcpy(vt, uvs, sizeof(float) * 8);

#define SET_QUAD_COLOR_FROM_COLOR32()\
	Color* col = GET_CURRENT_QUAD_COLOR_POINTER();\
	col[0] = color;\
	col[1] = color;\
	col[2] = color;\
	col[3] = color;

const TileRect& Renderer::getRoundedBox()
{
	return state->roundedBox;
}

inline void FlushWhenFull()
{
	if (NumQuadsInBatch == BatchLimit)
		Renderer::flushBatch();
}

void Renderer::startBatch(FillType fill, VertexType vertexPositionType)
{
	ASSERT_BATCH_IS_EMPTY();

	CurrentFillType = fill;
	CurrentVertexType = vertexPositionType;
}

void Renderer::pushQuad(int lx, int ty, int rx, int by)
{
	ASSERT_CORRECT_BATCH(FillType::PlainQuads, VertexType::Int);
	FlushWhenFull();
	SET_QUAD_POS_FROM_RECT(int);
	++NumQuadsInBatch;
}

void Renderer::pushQuad(float lx, float ty, float rx, float by)
{
	ASSERT_CORRECT_BATCH(FillType::PlainQuads, VertexType::Float);
	FlushWhenFull();
	SET_QUAD_POS_FROM_RECT(float);
	++NumQuadsInBatch;
}

void Renderer::pushQuad(int lx, int ty, int rx, int by, Color color)
{
	ASSERT_CORRECT_BATCH(FillType::ColoredQuads, VertexType::Int);
	FlushWhenFull();
	SET_QUAD_POS_FROM_RECT(int);
	SET_QUAD_COLOR_FROM_COLOR32();
	++NumQuadsInBatch;
}

void Renderer::pushQuad(float lx, float ty, float rx, float by, Color color)
{
	ASSERT_CORRECT_BATCH(FillType::ColoredQuads, VertexType::Float);
	FlushWhenFull();
	SET_QUAD_POS_FROM_RECT(float);
	SET_QUAD_COLOR_FROM_COLOR32();
	++NumQuadsInBatch;
}

void Renderer::pushQuad(int lx, int ty, int rx, int by, const float uvs[8])
{
	ASSERT_CORRECT_BATCH(FillType::TexturedQuads, VertexType::Int);
	FlushWhenFull();
	SET_QUAD_POS_FROM_RECT(int);
	SET_QUAD_UVS_FROM_UV_ARRAY();
	++NumQuadsInBatch;
}

void Renderer::pushQuad(float lx, float ty, float rx, float by, const float uvs[8])
{
	ASSERT_CORRECT_BATCH(FillType::TexturedQuads, VertexType::Float);
	FlushWhenFull();
	SET_QUAD_POS_FROM_RECT(float);
	SET_QUAD_UVS_FROM_UV_ARRAY();
	++NumQuadsInBatch;
}

void Renderer::pushQuad(int lx, int ty, int rx, int by, Color color, const float uvs[8])
{
	ASSERT_CORRECT_BATCH(FillType::ColoredTexturedQuads, VertexType::Float);
	FlushWhenFull();
	SET_QUAD_POS_FROM_RECT(int);
	SET_QUAD_UVS_FROM_UV_ARRAY();
	SET_QUAD_COLOR_FROM_COLOR32();
	++NumQuadsInBatch;
}

void Renderer::pushQuad(float lx, float ty, float rx, float by, Color color, const float uvs[8])
{
	ASSERT_CORRECT_BATCH(FillType::ColoredTexturedQuads, VertexType::Float);
	FlushWhenFull();
	SET_QUAD_POS_FROM_RECT(float);
	SET_QUAD_UVS_FROM_UV_ARRAY();
	SET_QUAD_COLOR_FROM_COLOR32();
	++NumQuadsInBatch;
}

Renderer::ColoredTexturedQuadData Renderer::pushColoredTexturedQuads(int amount)
{
	ASSERT_CORRECT_BATCH(FillType::ColoredTexturedQuads, VertexType::Int);
	ASSERT_AMOUNT_IS_WITHIN_BATCH_LIMIT(amount);

	if (NumQuadsInBatch + amount > BatchLimit)
		flushBatch();

	ColoredTexturedQuadData result
	{
		GET_CURRENT_QUAD_VERTEX_POS_POINTER(int),
		GET_CURRENT_QUAD_TEX_COORD_POINTER(),
		GET_CURRENT_QUAD_COLOR_POINTER(),
	};
	NumQuadsInBatch += amount;
	return result;
}

static void RenderPlainQuadBatch()
{
	// glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	// glDisableClientState(GL_COLOR_ARRAY);
	// glVertexPointer(2, GetGlVertexType(), 0, VertexPositions);
	// glDrawElements(GL_TRIANGLES, NumQuadsInBatch * IndicesPerQuad, GL_UNSIGNED_INT, QuadIndices);
}

static void RenderColoredQuadBatch()
{
	// glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	// glEnableClientState(GL_COLOR_ARRAY);
	// glVertexPointer(2, GetGlVertexType(), 0, VertexPositions);
	// glColorPointer(4, GL_UNSIGNED_BYTE, 0, VertexColors);
	// glDrawElements(GL_TRIANGLES, NumQuadsInBatch * IndicesPerQuad, GL_UNSIGNED_INT, QuadIndices);
}

static void RenderTexturedQuadBatch()
{
	// glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// glDisableClientState(GL_COLOR_ARRAY);
	// glVertexPointer(2, GetGlVertexType(), 0, VertexPositions);
	// glTexCoordPointer(2, GL_FLOAT, 0, VertexTexCoords);
	// glDrawElements(GL_TRIANGLES, NumQuadsInBatch * IndicesPerQuad, GL_UNSIGNED_INT, QuadIndices);
}

static void RenderColoredTexturedQuadBatch()
{
	// glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// glEnableClientState(GL_COLOR_ARRAY);
	// glVertexPointer(2, GetGlVertexType(), 0, VertexPositions);
	// glTexCoordPointer(2, GL_FLOAT, 0, VertexTexCoords);
	// glColorPointer(4, GL_UNSIGNED_BYTE, 0, VertexColors);
	// glDrawElements(GL_TRIANGLES, NumQuadsInBatch * IndicesPerQuad, GL_UNSIGNED_INT, QuadIndices);
}

// Renders all geometry that is currently pending in a batch.
void Renderer::flushBatch()
{
	switch (CurrentFillType)
	{
	case FillType::PlainQuads:
		RenderPlainQuadBatch();
		break;

	case FillType::ColoredQuads:
		RenderColoredQuadBatch();
		break;

	case FillType::TexturedQuads:
		RenderTexturedQuadBatch();
		break;

	case FillType::ColoredTexturedQuads:
		RenderColoredTexturedQuadBatch();
		break;
	}
	NumQuadsInBatch = 0;
}

} // namespace AV
