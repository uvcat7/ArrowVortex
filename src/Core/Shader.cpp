#include <Core/Shader.h>

#include <Core/Utils.h>
#include <Core/StringUtils.h>

#include <System/File.h>
#include <System/Debug.h>
#include <System/OpenGL.h>

namespace Vortex {

// ================================================================================================
// Shader extensions.

static bool sSupported = false;

#define EXT(name, result) static result(APIENTRY* name)

#define PROC(name)                                   \
    name = (decltype(name))wglGetProcAddress(#name); \
    if (!name) {                                     \
        missing.push_back(#name);                    \
        ++numMissing;                                \
    }
#define PROC_OPT(name)                               \
    name = (decltype(name))wglGetProcAddress(#name); \
    if (!name) {                                     \
        missing.push_back(#name);                    \
    }

// #define PROC(name) name = nullptr; if(!name) { missing.push_back(#name);
// ++numMissing; } #define PROC_OPT(name) name = nullptr; if(!name) {
// missing.push_back(#name); }

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84

EXT(glCreateShader, GLint)(GLenum type);
EXT(glDeleteShader, void)(GLuint shader);
EXT(glCompileShader, void)(GLuint shader);
EXT(glShaderSource, void)(GLuint shader, GLsizei count, const char* const* str,
                          const GLint* len);
EXT(glGetShaderiv, void)(GLuint shader, GLenum pname, GLint* param);
EXT(glGetShaderInfoLog, void)(GLuint shader, GLsizei bufSize, GLsizei* len,
                              char* str);

EXT(glGetUniformLocation, GLint)(GLuint program, const char* name);
EXT(glUniform1i, void)(GLint loc, GLint v0);
EXT(glUniform1f, void)(GLint loc, GLfloat v0);
EXT(glUniform2f, void)(GLint loc, GLfloat v0, GLfloat v1);
EXT(glUniform4f, void)(GLint loc, GLfloat v0, GLfloat v1, GLfloat v2,
                       GLfloat v3);

EXT(glCreateProgram, GLuint)(void);
EXT(glUseProgram, void)(GLuint program);
EXT(glDeleteProgram, void)(GLuint program);
EXT(glLinkProgram, void)(GLuint program);
EXT(glAttachShader, void)(GLuint program, GLuint shader);
EXT(glGetProgramiv, void)(GLuint program, GLenum pname, GLint* param);
EXT(glGetProgramInfoLog, void)(GLuint program, GLsizei bufSize, GLsizei* len,
                               char* str);

void Shader::initExtension() {
    Vector<std::string> missing;
    int numMissing = 0;

    PROC(glCreateShader);
    PROC(glDeleteShader);
    PROC(glCompileShader);
    PROC(glShaderSource);
    PROC(glGetShaderiv);
    PROC_OPT(glGetShaderInfoLog);

    PROC(glGetUniformLocation);
    PROC(glUniform1i);
    PROC(glUniform1f);
    PROC(glUniform2f);
    PROC(glUniform4f);

    PROC(glCreateProgram);
    PROC(glUseProgram);
    PROC(glDeleteProgram);
    PROC(glLinkProgram);
    PROC(glAttachShader);
    PROC(glGetProgramiv);
    PROC_OPT(glGetProgramInfoLog);

    sSupported = (numMissing == 0);

    if (sSupported) {
        Debug::log("shader support :: OK\n");
    } else {
        Debug::blockBegin(sSupported ? Debug::WARNING : Debug::ERROR,
                          "some shader extensions are not supported");
        for (auto& s : missing) Debug::log("missing: %s\n", s.c_str());
        Debug::log("shader support :: MISSING\n");
        Debug::blockEnd();
    }
}

bool Shader::isSupported() { return sSupported; }

// ================================================================================================
// Shader :: implementation.

static bool Error(std::string& log, const char* desc, const char* name,
                  const char* f1, const char* f2) {
    Debug::blockBegin(Debug::ERROR, desc);
    while (log.length() && (log.back() == '\0' || log.back() == '\n'))
        Str::pop_back(log);
    if (name) Debug::log("shader: %s\n", name);
    if (f1 && f2) {
        Debug::log("vert-file: %s\n", f1);
        Debug::log("frag-file: %s\n", f2);
    } else if (f1) {
        Debug::log("file: %s\n", f1);
    }
    Debug::log("log:\n%s\n", log.c_str());
    Debug::blockEnd();
    return false;
}

static void Destroy(uint32_t& program, uint32_t& vert, uint32_t& frag) {
    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
    if (vert) {
        glDeleteShader(vert);
        vert = 0;
    }
    if (frag) {
        glDeleteShader(frag);
        frag = 0;
    }
}

static GLuint Compile(int type, const char* code, std::string& log) {
    GLint compiled = 0;
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        if (glGetShaderInfoLog) {
            int size = 1;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
            log.clear();
            log.resize(size);
            glGetShaderInfoLog(shader, size, nullptr, &log[0]);
        }
        glDeleteShader(shader);
        shader = 0;
    }
    return shader;
}

static GLuint Link(GLuint vert, GLuint frag, std::string& log) {
    GLint linked = 0;
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        if (glGetProgramInfoLog) {
            int size = 1;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
            log.clear();
            log.resize(size);
            glGetProgramInfoLog(program, size, nullptr, &log[0]);
        }
        glDeleteProgram(program);
        program = 0;
    }
    return program;
}

Shader::~Shader() {
    Destroy(program_id_, vertex_shader_id_, fragment_shader_id_);
}

Shader::Shader()
    : program_id_(0), vertex_shader_id_(0), fragment_shader_id_(0) {}

bool Shader::load(const char* vertexCode, const char* fragmentCode,
                  const char* def, const char* shaderName,
                  const char* vertexFile, const char* fragmentFile) {
    Destroy(program_id_, vertex_shader_id_, fragment_shader_id_);

    std::string log;

    vertex_shader_id_ = Compile(GL_VERTEX_SHADER, vertexCode, log);
    if (!vertex_shader_id_)
        return Error(log, "could not compile vertex shader", shaderName,
                     vertexFile, nullptr);
    VortexCheckGlError();

    fragment_shader_id_ = Compile(GL_FRAGMENT_SHADER, fragmentCode, log);
    if (!fragment_shader_id_)
        return Error(log, "could not compile fragment shader", shaderName,
                     fragmentFile, nullptr);
    VortexCheckGlError();

    program_id_ = Link(vertex_shader_id_, fragment_shader_id_, log);
    if (!program_id_)
        return Error(log, "could not link shaders", shaderName, vertexFile,
                     fragmentFile);
    VortexCheckGlError();

    return (program_id_ != 0);
}

void Shader::bind() { glUseProgram(program_id_); }

void Shader::unbind() { glUseProgram(0); }

int Shader::getUniformLocation(const char* name) {
    return glGetUniformLocation(program_id_, name);
}

void Shader::uniform1i(int loc, int val) { glUniform1i(loc, val); }

void Shader::uniform1f(int loc, float val) { glUniform1f(loc, val); }

void Shader::uniform2f(int loc, float x, float y) { glUniform2f(loc, x, y); }

void Shader::uniform2f(int loc, const vec2f& vec) {
    glUniform2f(loc, vec.x, vec.y);
}

void Shader::uniform4f(int loc, float x, float y, float z, float w) {
    glUniform4f(loc, x, y, z, w);
}

void Shader::uniform4f(int loc, const colorf& c) {
    glUniform4f(loc, c.r, c.g, c.b, c.a);
}

};  // namespace Vortex
