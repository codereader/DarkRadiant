#pragma once

#include <pybind11/pybind11.h>

#include "ishaders.h"
#include "iscript.h"
#include "iscriptinterface.h"

namespace script
{

/**
 * This class represents a single Material as seen by the Python script.
 */
class ScriptMaterial
{
	// The contained shader (can be NULL)
	MaterialPtr _shader;
public:
    ScriptMaterial(const MaterialPtr& shader) :
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

class MaterialVisitor
{
public:
    virtual ~MaterialVisitor() {}
    virtual void visit(const MaterialPtr& shader) = 0;
};

// Wrap around the EntityClassVisitor interface
class MaterialVisitorWrapper :
	public MaterialVisitor
{
public:
    void visit(const MaterialPtr& shader) override
    {
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,			/* Return type */
            MaterialVisitor,    /* Parent class */
			visit,			/* Name of function in C++ (must match Python name) */
			ScriptMaterial(shader)			/* Argument(s) */
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
	void foreachMaterial(MaterialVisitor& visitor);
	ScriptMaterial getMaterial(const std::string& name);
    bool materialExists(const std::string& name);
    bool materialCanBeModified(const std::string& name);
    ScriptMaterial createEmptyMaterial(const std::string& name);
    ScriptMaterial copyMaterial(const std::string& nameOfOriginal, const std::string& nameOfCopy);
    bool renameMaterial(const std::string& oldName, const std::string& newName);
    void removeMaterial(const std::string& name);
    void saveMaterial(const std::string& name);

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
