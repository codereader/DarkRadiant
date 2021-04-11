#pragma once

#include <pybind11/pybind11.h>

#include "ishaders.h"
#include "iscript.h"
#include "iscriptinterface.h"

namespace script
{

class ScriptMaterialStage
{
private:
    IShaderLayer::Ptr _layer;

public:
    ScriptMaterialStage(const IShaderLayer::Ptr& layer) :
        _layer(layer)
    {}

    IShaderLayer::Type getType()
    {
        return _layer ? _layer->getType() : IShaderLayer::BLEND;
    }

    std::pair<std::string, std::string> getBlendFuncStrings()
    {
        return _layer ? _layer->getBlendFuncStrings() : std::pair<std::string, std::string>();
    }

    int getStageFlags()
    {
        return _layer ? _layer->getStageFlags() : 0;
    }

    ClampType getClampType()
    {
        return _layer ? _layer->getClampType() : CLAMP_REPEAT;
    }

    IShaderLayer::TexGenType getTexGenType()
    {
        return _layer ? _layer->getTexGenType() : IShaderLayer::TEXGEN_NORMAL;
    }

    std::string getTexGenExpressionString(std::size_t index)
    {
        return _layer && index < 2 && _layer->getTexGenExpression(index) ? 
            _layer->getTexGenExpression(index)->getExpressionString() : std::string();
    }

    std::string getColourExpressionString(IShaderLayer::ColourComponentSelector component)
    {
        return _layer && _layer->getColourExpression(component) ?
            _layer->getColourExpression(component)->getExpressionString() : std::string();
    }

    IShaderLayer::VertexColourMode getVertexColourMode()
    {
        return _layer ? _layer->getVertexColourMode() : IShaderLayer::VERTEX_COLOUR_NONE;
    }

    IShaderLayer::MapType getMapType()
    {
        return _layer ? _layer->getMapType() : IShaderLayer::MapType::Map;
    }
};

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

    void setDescription(const std::string& description)
    {
        throwIfMaterialCannotBeModified();
        _material->setDescription(description);
    }

    std::vector<ScriptMaterialStage> getAllStages()
    {
        std::vector<ScriptMaterialStage> list;

        if (!_material) return list;

        for (const auto& layer : _material->getAllLayers())
        {
            list.emplace_back(layer);
        }

        return list;
    }

    std::size_t getNumStages()
    {
        return _material ? _material->getAllLayers().size() : 0;
    }

    ScriptMaterialStage getStage(int index)
    {
        if (!_material) return ScriptMaterialStage(IShaderLayer::Ptr());

        const auto& layers = _material->getAllLayers();
        
        return ScriptMaterialStage(index >= 0 && index < layers.size() ? layers[index] : IShaderLayer::Ptr());
    }

    std::size_t addStage(IShaderLayer::Type type)
    {
        throwIfMaterialCannotBeModified();
        return _material->addLayer(type);
    }

    void removeStage(std::size_t index)
    {
        throwIfMaterialCannotBeModified();
        _material->removeLayer(index);
    }

    std::size_t duplicateStage(std::size_t index)
    {
        throwIfMaterialCannotBeModified();
        return _material->duplicateLayer(index);
    }

    void swapStagePosition(std::size_t first, std::size_t second)
    {
        throwIfMaterialCannotBeModified();
        _material->swapLayerPosition(first, second);
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

    bool isCubicLight() {
        return _material ? _material->isCubicLight() : false;
    }

    void setIsAmbientLight(bool newValue)
    {
        throwIfMaterialCannotBeModified();
        _material->setIsAmbientLight(newValue);
    }

    void setIsBlendLight(bool newValue)
    {
        throwIfMaterialCannotBeModified();
        _material->setIsBlendLight(newValue);
    }

    void setIsFogLight(bool newValue)
    {
        throwIfMaterialCannotBeModified();
        _material->setIsFogLight(newValue);
    }

    void setIsCubicLight(bool newValue)
    {
        throwIfMaterialCannotBeModified();
        _material->setIsCubicLight(newValue);
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

    Material::DecalInfo getDecalInfo()
    {
        return _material ? _material->getDecalInfo() : Material::DecalInfo();
    }

    Material::Coverage getCoverage()
    {
        return _material ? _material->getCoverage() : Material::MC_UNDETERMINED;
    }

    std::string getLightFalloffExpressionString()
    {
        return _material && _material->getLightFalloffExpression() ?
            _material->getLightFalloffExpression()->getExpressionString() : std::string();
    }

    void setLightFalloffExpressionFromString(const std::string& expressionString)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setLightFalloffExpressionFromString(expressionString);
    }

    IShaderLayer::MapType getLightFalloffCubeMapType()
    {
        return _material ? _material->getLightFalloffCubeMapType() : IShaderLayer::MapType::Map;
    }

    void setLightFalloffCubeMapType(IShaderLayer::MapType type)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setLightFalloffCubeMapType(type);
    }

    std::string getRenderBumpArguments()
    {
        return _material ? _material->getRenderBumpArguments() : std::string();
    }

    std::string getRenderBumpFlatArguments()
    {
        return _material ? _material->getRenderBumpFlatArguments() : std::string();
    }

    std::string getGuiSurfArgument()
    {
        return _material ? _material->getGuiSurfArgument() : std::string();
    }

    bool isModified()
    {
        return _material ? _material->isModified() : false;
    }

    void revertModifications()
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->revertModifications();
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
