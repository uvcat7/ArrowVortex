#include <Core/Gui.h>

#include <Core/Vector.h>
#include <Core/Shader.h>
#include <Core/Renderer.h>
#include <Core/Draw.h>
#include <Core/Canvas.h>

#include <System/Debug.h>
#include <System/OpenGL.h>

namespace Vortex {

static const int BATCH_QUAD_LIMIT = 256;

static const int VB_POS_STRIDE = sizeof(uint32_t) * 8;
static const int VB_UVS_STRIDE = sizeof(float) * 8;
static const int VB_COL_STRIDE = sizeof(uint32_t) * 4;

static const int VB_POS_SIZE = VB_POS_STRIDE * BATCH_QUAD_LIMIT;
static const int VB_UVS_SIZE = VB_UVS_STRIDE * BATCH_QUAD_LIMIT;
static const int VB_COL_SIZE = VB_COL_STRIDE * BATCH_QUAD_LIMIT;

// ================================================================================================
// Renderer singleton.

namespace {

struct RendererInstance {
    Shader shaders[4];
    Vector<recti> scissorStack;
    uint32_t* quadIndices;

    uint8_t* batchPos;
    uint8_t* batchCol;
    uint8_t* batchUvs;
    int quadsLeft;

    TileRect roundedBox;
};

static RendererInstance* RI;

};  // anonymous namespace

// ================================================================================================
// Creation/destruction.

static void createBatchData() {
    RI->batchPos =
        static_cast<uint8_t*>(malloc(VB_POS_SIZE + VB_UVS_SIZE + VB_COL_SIZE));
    RI->batchUvs = RI->batchPos + VB_POS_SIZE;
    RI->batchCol = RI->batchUvs + VB_UVS_SIZE;
    RI->quadsLeft = BATCH_QUAD_LIMIT;
}

static void createQuadIndices() {
    RI->quadIndices =
        static_cast<uint32_t*>(malloc(sizeof(uint32_t) * BATCH_QUAD_LIMIT * 6));
    for (uint32_t *p = RI->quadIndices, i = 0; i < BATCH_QUAD_LIMIT * 4;
         i += 4) {
        *p = i + 0;
        ++p;
        *p = i + 3;
        ++p;
        *p = i + 1;
        ++p;
        *p = i + 0;
        ++p;
        *p = i + 2;
        ++p;
        *p = i + 3;
        ++p;
    }
}

static void loadShaders() {
    const char* colorShaderVert =
        "varying vec4 c;"
        "void main() { c = gl_Color; gl_Position = "
        "gl_ModelViewProjectionMatrix * gl_Vertex; }";

    const char* colorShaderFrag =
        "varying vec4 c;"
        "void main() { gl_FragColor = c; }";

    const char* colorHsvShaderFrag =
        "varying vec4 c;"
        "void main() { vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);"
        "vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);"
        "gl_FragColor = vec4(c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), "
        "c.y), c.a); }";

    const char* textureShaderVert =
        "varying vec4 c; varying vec2 uvs;"
        "void main() { c = gl_Color; uvs = gl_MultiTexCoord0.xy; gl_Position = "
        "gl_ModelViewProjectionMatrix * gl_Vertex; }";

    const char* textureShaderFrag =
        "uniform sampler2D tex; varying vec4 c; varying vec2 uvs;"
        "void main() { gl_FragColor = c * texture2D(tex, uvs); }";

    const char* alphaTexShaderFrag =
        "uniform sampler2D tex; varying vec4 c; varying vec2 uvs;"
        "void main() { gl_FragColor = vec4(c.rgb, c.a * texture2D(tex, "
        "uvs).r); }";

    if (Shader::isSupported()) {
        Shader& col = RI->shaders[Renderer::SH_COLOR];
        col.load(colorShaderVert, colorShaderFrag, nullptr,
                 "Renderer::colorShader");

        Shader& hsv = RI->shaders[Renderer::SH_COLOR_HSV];
        hsv.load(colorShaderVert, colorHsvShaderFrag, nullptr,
                 "Renderer::hsvShader");

        Shader& tex = RI->shaders[Renderer::SH_TEXTURE];
        tex.load(textureShaderVert, textureShaderFrag, nullptr,
                 "Renderer::textureShader");

        Shader& atex = RI->shaders[Renderer::SH_TEXTURE_ALPHA];
        atex.load(textureShaderVert, alphaTexShaderFrag, nullptr,
                  "Renderer::alphaTexShader");

        tex.bind();
        Shader::uniform1i(tex.getUniformLocation("tex"), 0);

        atex.bind();
        Shader::uniform1i(atex.getUniformLocation("tex"), 0);

        Shader::unbind();
    }
}

void Renderer::create() {
    RI = new RendererInstance;

    createBatchData();
    createQuadIndices();
    loadShaders();

    Canvas roundedBox(16, 16, 1.0f);
    roundedBox.setColor(1.0f);
    roundedBox.box(0, 0, 16, 16, 4.0f);

    RI->roundedBox.texture = roundedBox.createTexture(false);
    RI->roundedBox.border = 6;

    VortexCheckGlError();
}

void Renderer::destroy() {
    free(RI->batchPos);
    free(RI->quadIndices);

    delete RI;
    RI = nullptr;
}

void Renderer::startFrame() {
    vec2i view = GuiMain::getViewSize();
    glLoadIdentity();
    glOrtho(0, view.x, view.y, 0, -1, 1);
}

void Renderer::endFrame() {
    glDisable(GL_SCISSOR_TEST);
    RI->scissorStack.clear();
}

// ================================================================================================
// Render state.

void Renderer::bindTexture(TextureHandle texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
}

void Renderer::unbindTexture() { glBindTexture(GL_TEXTURE_2D, 0); }

void Renderer::bindShader(DefaultShader shader) {
    if (Shader::isSupported()) {
        RI->shaders[shader].bind();
    }
}

void Renderer::setColor(colorf color) {
    glColor4f(color.r, color.g, color.b, color.a);
}

void Renderer::setColor(uint32_t color) {
    uint8_t* c = reinterpret_cast<uint8_t*>(&color);
    glColor4ub(c[0], c[1], c[2], c[3]);
}

void Renderer::resetColor() { glColor4ub(255, 255, 255, 255); }

void Renderer::pushScissorRect(const recti& r) {
    pushScissorRect(r.x, r.y, r.w, r.h);
}

void Renderer::pushScissorRect(int x, int y, int w, int h) {
    auto& stack = RI->scissorStack;
    w = max(w, 0), h = max(h, 0);
    if (stack.empty()) {
        // The new scissor region is the first scissor region, use it as-is.
        glEnable(GL_SCISSOR_TEST);
        stack.push_back({x, y, w, h});
    } else if (stack.size() < 256) {
        // Calculate the intersection of the current and new scissor region.
        recti last = stack.back();
        int r = min(last.x + last.w, x + w);
        int b = min(last.y + last.h, y + h);
        x = max(last.x, x), w = max(0, r - x);
        y = max(last.y, y), h = max(0, b - y);
        stack.push_back({x, y, w, h});
    }

    // Apply the new scissor region.
    vec2i view = GuiMain::getViewSize();
    glScissor(x, view.y - (y + h), w, h);
}

void Renderer::popScissorRect() {
    auto& stack = RI->scissorStack;
    if (stack.size() == 1) {
        stack.pop_back();
        glDisable(GL_SCISSOR_TEST);
    } else if (stack.size() > 0) {
        stack.pop_back();
        recti r = stack.back();
        vec2i view = GuiMain::getViewSize();
        glScissor(r.x, view.y - (r.y + r.h), r.w, r.h);
    }
}

// ================================================================================================
// Core rendering functions.

static int FlushQuads(GLint vertexType, const void* pos,
                      const float* uvs = nullptr,
                      const uint32_t* col = nullptr) {
    glDrawElements(GL_TRIANGLES, BATCH_QUAD_LIMIT * 6, GL_UNSIGNED_INT,
                   RI->quadIndices);
    pos = reinterpret_cast<const int*>(pos) + BATCH_QUAD_LIMIT * 8;
    glVertexPointer(2, vertexType, 0, pos);
    if (uvs) {
        uvs += BATCH_QUAD_LIMIT * 8;
        glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    }
    if (col) {
        col += BATCH_QUAD_LIMIT * 4;
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, col);
    }
    return BATCH_QUAD_LIMIT;
}

void Renderer::drawQuads(int numQuads, const int* pos) {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_INT, 0, pos);

    while (numQuads > BATCH_QUAD_LIMIT) numQuads -= FlushQuads(GL_INT, pos);
    glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT,
                   RI->quadIndices);
}

void Renderer::drawQuads(int numQuads, const int* pos, const uint32_t* col) {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_INT, 0, pos);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, col);

    while (numQuads > BATCH_QUAD_LIMIT)
        numQuads -= FlushQuads(GL_INT, pos, nullptr, col);
    glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT,
                   RI->quadIndices);
}

void Renderer::drawQuads(int numQuads, const int* pos, const float* uvs) {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_INT, 0, pos);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);

    while (numQuads > BATCH_QUAD_LIMIT)
        numQuads -= FlushQuads(GL_INT, pos, uvs);
    glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT,
                   RI->quadIndices);
}

void Renderer::drawQuads(int numQuads, const int* pos, const float* uvs,
                         const uint32_t* col) {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_INT, 0, pos);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, col);

    while (numQuads > BATCH_QUAD_LIMIT)
        numQuads -= FlushQuads(GL_INT, pos, uvs, col);
    glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT,
                   RI->quadIndices);
}

void Renderer::drawQuads(int numQuads, const float* pos, const float* uvs,
                         const uint32_t* col) {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, pos);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glColorPointer(4, GL_INT, 0, col);

    while (numQuads > BATCH_QUAD_LIMIT)
        numQuads -= FlushQuads(GL_FLOAT, pos, uvs, col);
    glDrawElements(GL_TRIANGLES, numQuads * 6, GL_UNSIGNED_INT,
                   RI->quadIndices);
}

void Renderer::drawTris(int numTris, const uint32_t* indices, const int* pos) {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_INT, 0, pos);

    glDrawElements(GL_TRIANGLES, numTris * 3, GL_UNSIGNED_INT, indices);
}

void Renderer::drawTris(int numTris, const uint32_t* indices, const int* pos,
                        const float* uvs) {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_INT, 0, pos);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);

    glDrawElements(GL_TRIANGLES, numTris * 3, GL_UNSIGNED_INT, indices);
}

// ================================================================================================
// Batch rendering.

static void FlushIC() {
    Renderer::drawQuads(BATCH_QUAD_LIMIT - RI->quadsLeft,
                        reinterpret_cast<int*>(RI->batchPos),
                        reinterpret_cast<uint32_t*>(RI->batchCol));
    RI->quadsLeft = BATCH_QUAD_LIMIT;
}

static void FlushIT() {
    Renderer::drawQuads(BATCH_QUAD_LIMIT - RI->quadsLeft,
                        reinterpret_cast<int*>(RI->batchPos),
                        reinterpret_cast<float*>(RI->batchUvs));
    RI->quadsLeft = BATCH_QUAD_LIMIT;
}

static void FlushITC() {
    Renderer::drawQuads(BATCH_QUAD_LIMIT - RI->quadsLeft,
                        reinterpret_cast<int*>(RI->batchPos),
                        reinterpret_cast<float*>(RI->batchUvs),
                        reinterpret_cast<uint32_t*>(RI->batchCol));
    RI->quadsLeft = BATCH_QUAD_LIMIT;
}

void QuadBatchC::push(int numQuads) {
    if (RI->quadsLeft < numQuads) FlushIC();
    int offset = BATCH_QUAD_LIMIT - RI->quadsLeft;
    RI->quadsLeft -= numQuads;

    pos = reinterpret_cast<int*>(RI->batchPos + offset * VB_POS_STRIDE);
    col = reinterpret_cast<uint32_t*>(RI->batchCol + offset * VB_COL_STRIDE);
}

void QuadBatchT::push(int numQuads) {
    if (RI->quadsLeft < numQuads) FlushIT();
    int offset = BATCH_QUAD_LIMIT - RI->quadsLeft;
    RI->quadsLeft -= numQuads;

    pos = reinterpret_cast<int*>(RI->batchPos + offset * VB_POS_STRIDE);
    uvs = reinterpret_cast<float*>(RI->batchUvs + offset * VB_UVS_STRIDE);
}

void QuadBatchTC::push(int numQuads) {
    if (RI->quadsLeft < numQuads) FlushITC();
    int offset = BATCH_QUAD_LIMIT - RI->quadsLeft;
    RI->quadsLeft -= numQuads;

    pos = reinterpret_cast<int*>(RI->batchPos + offset * VB_POS_STRIDE);
    uvs = reinterpret_cast<float*>(RI->batchUvs + offset * VB_UVS_STRIDE);
    col = reinterpret_cast<uint32_t*>(RI->batchCol + offset * VB_COL_STRIDE);
}

void QuadBatchC::flush() { FlushIC(); }

void QuadBatchT::flush() { FlushIT(); }

void QuadBatchTC::flush() { FlushITC(); }

QuadBatchC Renderer::batchC() {
    return {reinterpret_cast<int*>(RI->batchPos),
            reinterpret_cast<uint32_t*>(RI->batchCol)};
}

QuadBatchT Renderer::batchT() {
    return {reinterpret_cast<int*>(RI->batchPos),
            reinterpret_cast<float*>(RI->batchUvs)};
}

QuadBatchTC Renderer::batchTC() {
    return {reinterpret_cast<int*>(RI->batchPos),
            reinterpret_cast<float*>(RI->batchUvs),
            reinterpret_cast<uint32_t*>(RI->batchCol)};
}

const TileRect& Renderer::getRoundedBox() { return RI->roundedBox; }

};  // namespace Vortex
