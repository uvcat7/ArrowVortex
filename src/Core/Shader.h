#pragma once

#include <Core/Core.h>

namespace Vortex {

class Shader {
   public:
    ~Shader();
    Shader();

    static void initExtension();
    static bool isSupported();

    bool load(const char* vertexCode, const char* fragmentCode,
              const char* defines = nullptr, const char* shaderName = nullptr,
              const char* vertexFile = nullptr,
              const char* fragmentFile = nullptr);

    int getUniformLocation(const char* name);

    void bind();
    static void unbind();

    // NOTE: bind the shader before using any of the uniform functions.
    static void uniform1i(int loc, int val);
    static void uniform1f(int loc, float val);
    static void uniform2f(int loc, float x, float y);
    static void uniform2f(int loc, const vec2f& vec);
    static void uniform4f(int loc, float x, float y, float z, float w);
    static void uniform4f(int loc, const colorf& color);

    uint32_t program_id_, vertex_shader_id_, fragment_shader_id_;
};

};  // namespace Vortex
