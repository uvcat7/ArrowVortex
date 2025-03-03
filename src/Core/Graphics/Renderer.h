#pragma once

#include <Core/Types/Rect.h>

#include <Core/Graphics/Color.h>

namespace AV {

namespace Renderer
{
	// The maximum number of quads or triangles a render batch is allowed to push in a single operation.
	struct Constants
	{
		static constexpr int BatchLimit = 256;
	};
	
	// Quad batch with integer vertex positions.
	struct PlainQuadData { int* pos; };
	
	// Render batch with integer vertex positions, and texture coordinates.
	struct TexturedQuadData { int* pos; float* uvs; };
	
	// Quad batch with integer vertex positions, and colors.
	struct ColoredQuadData { int* pos; Color* col; };
	
	// Quad batch with integer vertex positions, colors, and texture coordinates.
	struct ColoredTexturedQuadData { int* pos; float* uvs; Color* col; };
	
	// List of default shaders.
	enum class Shader
	{
		Color,        // No texture.
		ColorHSV,     // No texture, HSV color values.
		Texture,      // Non alpha-only texture.
		TextureAlpha, // Alpha-only texture.
	};
	
	// List of blend modes that can be used for rendering.
	enum class BlendMode
	{
		Alpha, // Alpha blending.
		Add,   // Additive blending.
	};
	
	// List of fill types that can be used for batch rendering.
	enum class FillType
	{
		PlainQuads,
		ColoredQuads,
		TexturedQuads,
		ColoredTexturedQuads,
	};
	
	// List of types that can be used for vertex positions in batch rendering.
	enum class VertexType
	{
		Int,
		Float,
	};
	
	void initialize();
	void deinitialize();
	
// Render settings:
	
	// Binds one of the default shaders for drawing.
	void bindShader(Shader shader);
	
	// Binds the given texture and one of the texture shaders. The shader selection is based on whether the given texture is an alpha-only texture.
	void bindTextureAndShader(const Texture& texture);
	
	// Binds the given texture and one of the texture shaders. The shader selection is based on whether the given texture is an alpha-only texture.
	void bindTextureAndShader(const Texture* texture);
	
	// Binds a texture for drawing.
	void bindTexture(const Texture& texture);
	
	// Binds a texture for drawing.
	void bindTexture(const Texture* texture);
	
	// Binds the null texture.
	void unbindTexture();
	
	// Sets the uniform color for the current shader.
	void setColor(const Colorf& color);
	
	// Sets the uniform color for the current shader.
	void setColor(Color color);
	
	// Resets the uniform color for the current shader to white.
	void resetColor();
	
	// Sets the active blend mode.
	void setBlendMode(BlendMode mode);
	
	// Resets the active blend mode to alpha blending.
	void resetBlendMode();
	
	// Pushes a new scissor rectangle onto the stack. Anything outside of the scissor area is masked.
	void pushScissorRect(int lx, int ty, int rx, int by);
	
	// Pushes a new scissor rectangle onto the stack. Anything outside of the scissor area is masked.
	void pushScissorRect(const Rect& r);
	
	// Pops the topmost scissor rectangle from the stack.
	void popScissorRect();
	
	// TODO: move this!
	const TileRect& getRoundedBox();
	
// Direct rendering:
	
	// Draws quads, given a list of vertex positions <pos>, texture coordinates <uvs> and/or colors <col>.
	// The size of supplied vertex data must be as follows:
	// - vertex positions    : 8 per quad (2 per vertex).
	// - texture coordinates : 8 per quad (2 per vertex).
	// - vertex colors       : 4 per quad (1 per vertex).
	
	void drawQuads(int numQuads, const int* pos);
	void drawQuads(int numQuads, const float* pos);
	void drawQuads(int numQuads, const int* pos, const float* uvs);
	void drawQuads(int numQuads, const int* pos, const Color* col);
	void drawQuads(int numQuads, const int* pos, const float* uvs, const Color* col);
	void drawQuads(int numQuads, const float* pos, const float* uvs, const Color* col);
	
	// Draws triangles, given a list of vertex positions <pos> and/or texture coordinates <uvs>.
	// The size of supplied vertex data must be as follows:
	// - indices             : 3 per triangle.
	// - vertex positions    : 2 per index.
	// - texture coordinates : 2 per index.
	void drawTris(int numTris, const uint* indices, const int* pos);
	void drawTris(int numTris, const uint* indices, const int* pos, const float* uvs);
	
// Batch rendering:
	
	// Sets the active batch mode.
	void startBatch(FillType fill, VertexType vertexType = VertexType::Int);
	
	// Pushes a plain quad with the given positions (left, top, right, bottom).
	void pushQuad(int lx, int ty, int rx, int by);
	
	// Pushes a colored quad with the given positions (left, top, right, bottom).
	void pushQuad(float lx, float ty, float rx, float by);
	
	// Pushes a colored quad with the given positions (left, top, right, bottom) and color.
	void pushQuad(int lx, int ty, int rx, int by, Color color);
	
	// Pushes a colored quad with the given positions (left, top, right, bottom) and color.
	void pushQuad(float lx, float ty, float rx, float by, Color color);
	
	// Pushes a textured quad with the given positions (left, top, right, bottom) and texture coordindates.
	void pushQuad(int lx, int ty, int rx, int by, const float uvs[8]);
	
	// Pushes a textured quad with the given positions (left, top, right, bottom) and texture coordindates.
	void pushQuad(float lx, float ty, float rx, float by, const float uvs[8]);
	
	// Pushes a colored quad with the given positions (left, top, right, bottom), color and texture coordinates. 
	void pushQuad(int lx, int ty, int rx, int by, Color color, const float uvs[8]);
	
	// Pushes a colored quad with the given positions (left, top, right, bottom), color and texture coordinates. 
	void pushQuad(float lx, float ty, float rx, float by, Color color, const float uvs[8]);
	
	// Pushes one or more colored textured quads.
	ColoredTexturedQuadData pushColoredTexturedQuads(int amount);
	
	// Renders all geometry that is currently pending in a batch.
	void flushBatch();
};

} // namespace AV
