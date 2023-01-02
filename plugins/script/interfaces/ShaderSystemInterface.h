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
    struct Transformation
    {
        IShaderLayer::TransformType type;
        std::string expression1;
        std::string expression2;
    };

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

    std::string getMapExpressionString()
    {
        return _layer && _layer->getMapExpression() ? _layer->getMapExpression()->getExpressionString() : std::string();
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

    std::vector<Transformation> getTransformations()
    {
        std::vector<Transformation> list;

        if (!_layer) return list;

        for (const auto& transform : _layer->getTransformations())
        {
            list.emplace_back(Transformation
            {
                transform.type,
                transform.expression1 ? transform.expression1->getExpressionString() : "",
                transform.expression2 ? transform.expression2->getExpressionString() : "",
            });
        }

        return list;
    }

    Vector2 getRenderMapSize()
    {
        return _layer ? _layer->getRenderMapSize() : Vector2(0,0);
    }

    std::string getAlphaTestExpressionString()
    {
        return _layer && _layer->getAlphaTestExpression() ?
            _layer->getAlphaTestExpression()->getExpressionString() : std::string();
    }
    
    std::string getConditionExpressionString()
    {
        return _layer && _layer->getConditionExpression() ?
            _layer->getConditionExpression()->getExpressionString() : std::string();
    }

    std::string getVertexProgram()
    {
        return _layer ? _layer->getVertexProgram() : std::string();
    }
    
    std::string getFragmentProgram()
    {
        return _layer ? _layer->getFragmentProgram() : std::string();
    }

    std::size_t getNumVertexParms()
    {
        return _layer ? _layer->getNumVertexParms() : 0;
    }

    std::size_t getNumFragmentMaps()
    {
        return _layer ? _layer->getNumFragmentMaps() : 0;
    }

    struct VertexParm
    {
        std::size_t index;
        std::vector<std::string> expressions;
    };

    VertexParm getVertexParm(int index)
    {
        VertexParm parm;

        if (_layer && index >= 0 && index < 4)
        {
            const auto& vp = _layer->getVertexParm(index);
            
            parm.index = vp.index;

            for (auto i = 0; i < 4; ++i)
            {
                if (vp.expressions[i])
                {
                    parm.expressions.emplace_back(vp.expressions[i]->getExpressionString());
                }
            }
        }

        return parm;
    }

    struct FragmentMap
    {
        int index = -1;
        std::vector<std::string> options;
        std::string mapExpression;
    };

    FragmentMap getFragmentMap(int index)
    {
        FragmentMap map;

        if (_layer && index >= 0 && index < _layer->getNumFragmentMaps())
        {
            const auto& fm = _layer->getFragmentMap(index);

            map.index = fm.index;
            map.options = fm.options;
            map.mapExpression = fm.map ? fm.map->getExpressionString() : std::string();
        }

        return map;
    }

    float getPrivatePolygonOffset()
    {
        return _layer ? _layer->getPrivatePolygonOffset() : 0;
    }
};

// Editable shader stage interface
class ScriptEditableMaterialStage :
    public ScriptMaterialStage
{
private:
    IEditableShaderLayer::Ptr _layer;

public:
    ScriptEditableMaterialStage(const IEditableShaderLayer::Ptr& layer) :
        ScriptMaterialStage(layer),
        _layer(layer)
    {}

    void setStageFlag(IShaderLayer::Flags flag)
    {
        if (_layer) _layer->setStageFlag(flag);
    }

    void clearStageFlag(IShaderLayer::Flags flag)
    {
        if (_layer) _layer->clearStageFlag(flag);
    }

    void setMapType(IShaderLayer::MapType mapType)
    {
        if (_layer) _layer->setMapType(mapType);
    }

    void setBlendFuncStrings(const std::pair<std::string, std::string>& pair)
    {
        if (_layer) _layer->setBlendFuncStrings(pair);
    }

    void setAlphaTestExpressionFromString(const std::string& expression)
    {
        if (_layer) _layer->setAlphaTestExpressionFromString(expression);
    }

    void setMapExpressionFromString(const std::string& expression)
    {
        if (_layer) _layer->setMapExpressionFromString(expression);
    }

    std::size_t addTransformation(IShaderLayer::TransformType type, const std::string& expression1, const std::string& expression2)
    {
        return _layer ? _layer->addTransformation(type, expression1, expression2) : -1;
    }

    void removeTransformation(std::size_t index)
    {
        if (_layer) _layer->removeTransformation(index);
    }

    void updateTransformation(std::size_t index, IShaderLayer::TransformType type,
        const std::string& expression1, const std::string& expression2)
    {
        if (_layer) _layer->updateTransformation(index, type, expression1, expression2);
    }
    
    void setColourExpressionFromString(IShaderLayer::ColourComponentSelector component, const std::string& expression)
    {
        if (_layer) _layer->setColourExpressionFromString(component, expression);
    }

    void setConditionExpressionFromString(const std::string& expression)
    {
        if (_layer) _layer->setConditionExpressionFromString(expression);
    }

    void setTexGenType(IShaderLayer::TexGenType type)
    {
        if (_layer) _layer->setTexGenType(type);
    }

    void setTexGenExpressionFromString(std::size_t index, const std::string& expression)
    {
        if (_layer) _layer->setTexGenExpressionFromString(index, expression);
    }

    void setVertexColourMode(IShaderLayer::VertexColourMode mode)
    {
        if (_layer) _layer->setVertexColourMode(mode);
    }

    void setClampType(ClampType clampType)
    {
        if (_layer) _layer->setClampType(clampType);
    }

    void setPrivatePolygonOffset(double offset)
    {
        if (_layer) _layer->setPrivatePolygonOffset(offset);
    }

    void setRenderMapSize(const Vector2& size)
    {
        if (_layer) _layer->setRenderMapSize(size);
    }

    void setSoundMapWaveForm(bool waveForm)
    {
        if (_layer) _layer->setSoundMapWaveForm(waveForm);
    }

    void setVideoMapProperties(const std::string& filePath, bool looping)
    {
        if (_layer) _layer->setVideoMapProperties(filePath, looping);
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

        _material->foreachLayer([&](const IShaderLayer::Ptr& layer)
        {
            list.emplace_back(layer);
            return true;
        });

        return list;
    }

    std::size_t getNumStages()
    {
        return _material ? _material->getNumLayers() : 0;
    }

    ScriptMaterialStage getStage(std::size_t index)
    {
        if (!_material || index >= _material->getNumLayers())
        {
            return ScriptMaterialStage(IShaderLayer::Ptr());
        }

        return ScriptMaterialStage(_material->getLayer(index));
    }

    ScriptEditableMaterialStage getEditableStage(std::size_t index)
    {
        throwIfMaterialCannotBeModified();

        return ScriptEditableMaterialStage(index >= 0 && index < getNumStages() ? 
            _material->getEditableLayer(index) : IEditableShaderLayer::Ptr());
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

    void setDecalInfo(const Material::DecalInfo& decalInfo)
    {
        if (_material) _material->setDecalInfo(decalInfo);
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

    Material::FrobStageType getFrobStageType()
    {
        return _material ? _material->getFrobStageType() : Material::FrobStageType::Default;
    }

    void setFrobStageType(Material::FrobStageType type)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setFrobStageType(type);
    }

    std::string getFrobStageMapExpressionString()
    {
        return _material && _material->getFrobStageMapExpression() ? _material->getFrobStageMapExpression()->getExpressionString() : "";
    }

    void setFrobStageMapExpressionFromString(const std::string& expr)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setFrobStageMapExpressionFromString(expr);
    }

    Vector3 getFrobStageRgbParameter(std::size_t index)
    {
        return _material ? _material->getFrobStageRgbParameter(index) : Vector3(0, 0, 0);
    }

    void setFrobStageRgbParameter(std::size_t index, const Vector3& value)
    {
        throwIfMaterialCannotBeModified();
        if (_material) _material->setFrobStageRgbParameter(index, value);
    }

    void setFrobStageParameter(std::size_t index, double value)
    {
        setFrobStageRgbParameter(index, Vector3(value, value, value));
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
