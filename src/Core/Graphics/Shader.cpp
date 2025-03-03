#include <Precomp.h>

#include <Core/Utils/String.h>

#include <Core/System/Log.h>
#include <Core/System/Debug.h>

#include <Core/Graphics/Shader.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// ShaderInstance :: ShaderInstance.

ShaderInstance::~ShaderInstance()
{
	// Destroy(myProgram, myVert, myFrag);
}

ShaderInstance::ShaderInstance() : myProgram(0), myVert(0), myFrag(0)
{
}

bool ShaderInstance::load(
	const char* vertexCode,
	const char* fragmentCode,
	const char* shaderName,
	const char* defines,
	const char* vertexFilename,
	const char* fragmentFilename)
{
	return true;
}

int ShaderInstance::getUniformLocation(const char* name) const
{
	return 0; // Glx::glGetUniformLocation(myProgram, name);
}

uint ShaderInstance::getProgram() const
{
	return myProgram;
}

} // namespace AV
