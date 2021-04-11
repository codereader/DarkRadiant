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

    float getSortRequest()
    {
        return _material ? _material->getSortRequest() : -1;
    }

    void setSortRequest(float sortRequest)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setSortRequest(sortRequest);
    }

    void setSortRequest(Material::SortRequest sortRequest) // overload needed for python
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setSortRequest(static_cast<float>(sortRequest));
    }

    void resetSortRequest()
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->resetSortRequest();
    }

    float getPolygonOffset()
    {
        return _material ? _material->getPolygonOffset() : 0;
    }

    void setPolygonOffset(float offset)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setPolygonOffset(offset);
    }

    void clearPolygonOffset()
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->clearMaterialFlag(Material::FLAG_POLYGONOFFSET);
    }

    ClampType getClampType()
    {
        return _material ? _material->getClampType() : CLAMP_REPEAT;
    }

    void setClampType(ClampType type)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setClampType(type);
    }

    Material::CullType getCullType()
    {
        return _material ? _material->getCullType() : Material::CULL_BACK;
    }

    void setCullType(Material::CullType type)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setCullType(type);
    }

    int getMaterialFlags()
    {
        return _material ? _material->getMaterialFlags() : 0;
    }

    void setMaterialFlag(Material::Flags flag)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setMaterialFlag(flag);
    }

    void clearMaterialFlag(Material::Flags flag)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->clearMaterialFlag(flag);
    }

    int getSurfaceFlags()
    {
        return _material ? _material->getSurfaceFlags() : 0;
    }

    void setSurfaceFlag(Material::SurfaceFlags flag)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setSurfaceFlag(flag);
    }

    void clearSurfaceFlag(Material::SurfaceFlags flag)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->clearSurfaceFlag(flag);
    }

    Material::SurfaceType getSurfaceType()
    {
        return _material ? _material->getSurfaceType() : Material::SURFTYPE_DEFAULT;
    }

    void setSurfaceType(Material::SurfaceType type)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setSurfaceType(type);
    }

    Material::DeformType getDeformType()
    {
        return _material ? _material->getDeformType() : Material::DEFORM_NONE;
    }

    std::string getDeformExpressionString(std::size_t index)
    {
        return _material && _material->getDeformExpression(index) ? 
            _material->getDeformExpression(index)->getExpressionString() : std::string();
    }

    std::string getDeformDeclName()
    {
        return _material ? _material->getDeformDeclName() : std::string();
    }

    int getSpectrum()
    {
        return _material ? _material->getSpectrum() : 0;
    }

    void setSpectrum(int spectrum)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setSpectrum(spectrum);
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
