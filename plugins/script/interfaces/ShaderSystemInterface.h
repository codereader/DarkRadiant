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
	// The contained shader (can be empty)
	MaterialPtr _material;
public:
    ScriptMaterial(const MaterialPtr& material) :
		_material(material)
	{}

	operator const MaterialPtr&() const {
		return _material;
	}

	std::string getName() {
		return _material ? _material->getName() : "";
	}

	std::string getShaderFileName() {
		return _material ? _material->getShaderFileName() : "";
	}

    void setShaderFileName(const std::string& filename)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setShaderFileName(filename);
    }

	std::string getDescription() {
		return _material ? _material->getDescription() : "";
	}

	std::string getDefinition() {
		return _material ? _material->getDefinition() : "";
	}

	bool isVisible() {
		return _material ? _material->isVisible() : false;
	}

	bool isAmbientLight() {
		return _material ? _material->isAmbientLight() : false;
	}

	bool isBlendLight() {
		return _material ? _material->isBlendLight() : false;
	}

	bool isFogLight() {
		return _material ? _material->isFogLight() : false;
	}

	bool isNull() const {
		return _material == nullptr;
	}

    std::string getEditorImageExpressionString()
    {
        return _material && _material->getEditorImageExpression() ? 
            _material->getEditorImageExpression()->getExpressionString() : std::string();
    }

    void setEditorImageExpressionFromString(const std::string& editorImagePath)
    {
        throwIfMaterialCannotBeModified();
        _material->setEditorImageExpressionFromString(editorImagePath);
    }

private:
    void throwIfMaterialCannotBeModified()
    {
        if (!_material || !GlobalMaterialManager().materialCanBeModified(_material->getName()))
        {
            throw std::runtime_error("Material cannot be modified");
        }
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
