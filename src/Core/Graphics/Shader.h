#pragma once

#include <Precomp.h>

namespace AV {

class ShaderInstance
{
public:
	~ShaderInstance();
	ShaderInstance();

	bool load(
		const char* vertexCode,
		const char* fragmentCode,
		const char* shaderName,
		const char* defines = nullptr,
		const char* vertexFilename = nullptr,
		const char* fragmentFilename = nullptr);

	int getUniformLocation(const char* name) const;

	uint getProgram() const;

private:
	uint myProgram;
	uint myVert;
	uint myFrag;
};

} // namespace AV
