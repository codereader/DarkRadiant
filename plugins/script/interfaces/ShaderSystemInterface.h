#pragma once

#include <pybind11/pybind11.h>

#include "ishaders.h"
#include "iscript.h"

namespace script
{

/**
 * This class represents a single Shader as seen by the Python script.
 */
class ScriptShader
{
	// The contained shader (can be NULL)
	MaterialPtr _shader;
public:
	ScriptShader(const MaterialPtr& shader) :
		_shader(shader)
	{}

	operator const MaterialPtr&() const {
		return _shader;
	}

	std::string getName() {
		return (_shader != NULL) ? _shader->getName() : "";
	}

	std::string getShaderFileName() {
		return (_shader != NULL) ? _shader->getShaderFileName() : "";
	}

	std::string getDescription() {
		return (_shader != NULL) ? _shader->getDescription() : "";
	}

	std::string getDefinition() {
		return (_shader != NULL) ? _shader->getDefinition() : "";
	}

	bool isVisible() {
		return (_shader != NULL) ? _shader->isVisible() : false;
	}

	bool isAmbientLight() {
		return (_shader != NULL) ? _shader->isAmbientLight() : false;
	}

	bool isBlendLight() {
		return (_shader != NULL) ? _shader->isBlendLight() : false;
	}

	bool isFogLight() {
		return (_shader != NULL) ? _shader->isFogLight() : false;
	}

	bool isNull() const {
		return _shader == NULL;
	}
};

class ShaderVisitor
{
public:
    virtual ~ShaderVisitor() {}
    virtual void visit(const MaterialPtr& shader) = 0;
};

// Wrap around the EntityClassVisitor interface
class ShaderVisitorWrapper :
	public ShaderVisitor
{
public:
    void visit(const MaterialPtr& shader) override
    {
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,			/* Return type */
			ShaderVisitor,    /* Parent class */
			visit,			/* Name of function in C++ (must match Python name) */
			ScriptShader(shader)			/* Argument(s) */
		);
	}
};

/**
 * greebo: This class provides the script interface for the GlobalMaterialManager module.
 */
class ShaderSystemInterface :
	public IScriptInterface
{
public:
	void foreachShader(ShaderVisitor& visitor);
	ScriptShader getMaterialForName(const std::string& name);

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
