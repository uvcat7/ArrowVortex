#include <Core/TextDraw.h>

#include <Core/TextLayout.h>
#include <Core/Renderer.h>
#include <Core/FontData.h>
#include <Core/Shader.h>
#include <Core/StringUtils.h>

#include <System/File.h>
#include <System/Debug.h>

namespace Vortex {
namespace {

struct ShaderData {
    Shader* program;
    int colorLoc;
};

// ================================================================================================
// TextDraw singleton

struct TextDrawData {
    // Render settings.
    const Texture::Data* texture;
    const ShaderData* shader;

    // Shader data.
    ShaderData regularShader;
    ShaderData alphaShader;

    // Markup properties.
    uint32_t textColor;
    uint32_t shadowColor;

    // Vertex data.
    int glyphCount;
    int glyphCap;
    int* vpos;
    float* vtex;
};

static TextDrawData* RD = nullptr;

// ================================================================================================
// Shader creation/destruction.

static void CreateShaders() {
    VortexCheckGlError();

    std::string err;

    const char* textShaderVert =
        "varying vec2 uvs;"
        "void main() { uvs = gl_MultiTexCoord0.xy; gl_Position = "
        "gl_ModelViewProjectionMatrix * gl_Vertex; }";

    const char* textShaderFrag =
        "uniform sampler2D tex; uniform vec4 col; varying vec2 uvs;"
        "void main() { gl_FragColor = col * texture2D(tex, uvs); }";

    const char* textAlphaShaderFrag =
        "uniform sampler2D tex; uniform vec4 col; varying vec2 uvs;"
        "void main() { gl_FragColor = vec4(col.rgb, col.a * texture2D(tex, "
        "uvs).r); }";

    if (Shader::isSupported()) {
        RD->regularShader.program = new Shader();
        RD->regularShader.program->load(textShaderVert, textShaderFrag, nullptr,
                                        "TextDraw::regularShader");

        RD->alphaShader.program = new Shader();
        RD->alphaShader.program->load(textShaderVert, textAlphaShaderFrag,
                                      nullptr, "TextDraw::alphaShader");

        RD->regularShader.program->bind();
        RD->regularShader.colorLoc =
            RD->regularShader.program->getUniformLocation("col");
        Shader::uniform1i(RD->regularShader.program->getUniformLocation("tex"),
                          0);

        RD->alphaShader.program->bind();
        RD->alphaShader.colorLoc =
            RD->alphaShader.program->getUniformLocation("col");
        Shader::uniform1i(RD->alphaShader.program->getUniformLocation("tex"),
                          0);

        Shader::unbind();
    } else {
        RD->regularShader.program = nullptr;
        RD->alphaShader.program = nullptr;
    }

    VortexCheckGlError();
}

static void DestroyShaders() {
    delete RD->regularShader.program;
    delete RD->alphaShader.program;
}

};  // anonymous namespace

// ================================================================================================
// Creation and destruction.

void TextDraw::create() {
    RD = new TextDrawData;

    RD->shader = nullptr;
    RD->texture = nullptr;

    RD->textColor = Colors::white;
    RD->shadowColor = Colors::black;

    RD->glyphCount = 0;
    RD->glyphCap = 16;

    int vcap = RD->glyphCap * 4;
    RD->vpos = static_cast<int*>(malloc(sizeof(int) * 2 * vcap));
    RD->vtex = static_cast<float*>(malloc(sizeof(float) * 2 * vcap));

    CreateShaders();
}

void TextDraw::destroy() {
    DestroyShaders();

    free(RD->vpos);
    free(RD->vtex);

    delete RD;
    RD = nullptr;
}

// ================================================================================================
// Quad renderings.

static void AllocateMoreGlyphs() {
    RD->glyphCap *= 2;
    int vcap = RD->glyphCap * 4;
    auto tmp = realloc(RD->vpos, sizeof(int) * 2 * vcap);
    if (tmp) {
        RD->vpos = static_cast<int*>(tmp);
    }
    tmp = realloc(RD->vtex, sizeof(float) * 2 * vcap);
    if (tmp) {
        RD->vtex = static_cast<float*>(tmp);
    }
}

static void DrawCurrentGlyphs() {
    if (RD->glyphCount > 0) {
        if (Shader::isSupported()) {
            if (RD->shadowColor & kColor32_AlphaMask) {
                Shader::uniform4f(RD->shader->colorLoc,
                                  ToColorf(RD->shadowColor));
                for (int i = 0; i < RD->glyphCount * 8; ++i) --RD->vpos[i];
                Renderer::drawQuads(RD->glyphCount, RD->vpos, RD->vtex);
                for (int i = 0; i < RD->glyphCount * 8; ++i) ++RD->vpos[i];
            }
            Shader::uniform4f(RD->shader->colorLoc, ToColorf(RD->textColor));
        } else {
            Renderer::setColor(RD->textColor);
        }
        Renderer::drawQuads(RD->glyphCount, RD->vpos, RD->vtex);
        RD->glyphCount = 0;
    }
}

static void SetCurrentTexture(const Glyph& g) {
    DrawCurrentGlyphs();

    Renderer::bindTexture(g.tex->handle);
    RD->texture = g.tex;

    ShaderData* shader = g.hasAlphaTex ? &RD->alphaShader : &RD->regularShader;
    if (Shader::isSupported()) {
        if (RD->shader != shader) {
            RD->shader = shader;
            shader->program->bind();
        }
    }
}

static void PushGlyph(int x, int y, const Glyph& g) {
    if (g.tex != RD->texture) {
        SetCurrentTexture(g);
    }

    if (RD->glyphCount == RD->glyphCap) {
        AllocateMoreGlyphs();
    }

    int curVertex = RD->glyphCount * 4;
    ++RD->glyphCount;

    // Vertex positions.
    int* vp = RD->vpos + curVertex * 2;
    vp[0] = vp[4] = x + g.ofs.l;
    vp[1] = vp[3] = y + g.ofs.t;
    vp[2] = vp[6] = x + g.ofs.r;
    vp[5] = vp[7] = y + g.ofs.b;

    // Vertexs UVs.
    float* vt = RD->vtex + curVertex * 2;
    vt[0] = vt[4] = g.uvs.l;
    vt[1] = vt[3] = g.uvs.t;
    vt[2] = vt[6] = g.uvs.r;
    vt[5] = vt[7] = g.uvs.b;
}

// Applies the effect of a markup tag during glyph rendering.
static void ApplyMarkup(const LMarkup& markup) {
    DrawCurrentGlyphs();
    switch (markup.type) {
        case LMarkup::SET_TEXT_COLOR:
            RD->textColor = markup.setTextColor;
            break;
        case LMarkup::SET_SHADOW_COLOR:
            RD->shadowColor = markup.setShadowColor;
            break;
    }
}

// ================================================================================================
// Main draw function.

void Text::draw(recti textBox) { draw(TextLayout::getTextPos(textBox)); }

void Text::draw(vec2i textPos) {
    const LLayout& layout = TextLayout::get();
    const TextStyle& style = layout.style;

    RD->glyphCount = 0;
    RD->texture = nullptr;
    RD->shader = nullptr;

    Renderer::resetColor();

    // Draw background quads.

    if (layout.bgQuads.size()) {
        Renderer::bindShader(Renderer::SH_COLOR);
        auto batch = Renderer::batchC();
        for (auto& quad : layout.bgQuads) {
            auto& line = layout.lines[quad.line];
            int x = textPos.x + line.x + quad.x;
            int y = textPos.y + line.y + line.top;
            Draw::fill(&batch, {x, y, quad.w, line.bottom - line.top},
                       quad.color);
        }
        batch.flush();
    }

    // Set the intial markup properties.
    RD->textColor = style.textColor;
    RD->shadowColor = style.shadowColor;

    // Render the text.
    auto* markup = layout.markup.begin();
    int markupIndex = 0, numMarkup = layout.markup.size();
    auto* glyphs = layout.glyphs.begin();
    for (auto& line : layout.lines) {
        int glyphIndex = line.beginGlyph, lineEnd = line.endGlyph;
        while (glyphIndex != lineEnd) {
            // Apply markup until the current glyph index is reached.
            while (markupIndex < numMarkup &&
                   markup[markupIndex].glyphIndex <= glyphIndex) {
                ApplyMarkup(markup[markupIndex]);
                ++markupIndex;
            }

            // Find out how many glyph we can draw, until either the next markup
            // or the line end.
            int drawEnd = lineEnd;
            if (markupIndex < numMarkup &&
                markup[markupIndex].glyphIndex < lineEnd) {
                drawEnd = markup[markupIndex].glyphIndex;
            }

            // Draw said glyphs.
            for (; glyphIndex < drawEnd; ++glyphIndex) {
                auto& item = glyphs[glyphIndex];
                if (item.glyph->hasPixels) {
                    PushGlyph(textPos.x + line.x + item.x, textPos.y + line.y,
                              *item.glyph);
                }
            }
        }
    }
    DrawCurrentGlyphs();

    // Draw foreground quads.
    if (layout.fgQuads.size()) {
        Renderer::bindShader(Renderer::SH_COLOR);
        auto batch = Renderer::batchC();
        for (auto& quad : layout.fgQuads) {
            auto& line = layout.lines[quad.line];
            int x = textPos.x + line.x + quad.x;
            int y = textPos.y + line.y + line.top;
            Draw::fill(&batch, {x, y, quad.w, line.bottom - line.top},
                       quad.color);
        }
        batch.flush();
    }
}

};  // namespace Vortex