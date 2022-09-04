#pragma once

#include "MapExpression.h"
#include "Doom3ShaderLayer.h"

#include "ishaders.h"
#include "parser/DefTokeniser.h"
#include "math/Vector4.h"
#include "decl/EditableDeclaration.h"

#include <memory>

namespace shaders { class MapExpression; }

namespace shaders
{

class IShaderTemplate :
    public decl::IDeclaration
{
public:
    virtual ~IShaderTemplate() {}
};

/**
 * Data structure storing parsed material information from a material decl. This
 * class parses the decl using a tokeniser and stores the relevant information
 * internally, for later use by a CShader.
 */
class ShaderTemplate final :
    public decl::EditableDeclaration<IShaderTemplate>
{
private:
	static const int SORT_UNDEFINED = -99999;	// undefined sort number, will be replaced after parsing

	// Template name
	std::string _name;

	// Temporary current layer (used by the parsing functions)
	Doom3ShaderLayer::Ptr _currentLayer;

    sigc::signal<void> _sigTemplateChanged;
    bool _suppressChangeSignal;

public:
    using Ptr = std::shared_ptr<ShaderTemplate>;

    // Shared parsing constants
    constexpr static const char* DiscardedDelimiters = parser::WHITESPACE;
    constexpr static const char* KeptDelimiters = "{}(),"; // add the comma character to the kept delimiters

  	// Vector of LayerTemplates representing each stage in the material
    std::vector<Doom3ShaderLayer::Ptr> _layers;

    // Editorimage texture
	MapExpressionPtr _editorTex;

	// Map expressions
	MapExpressionPtr _lightFalloff;
    IShaderLayer::MapType _lightFalloffCubeMapType;

	/* Light type booleans */
	bool fogLight;
	bool ambientLight;
	bool blendLight;
	bool _cubicLight;

	// The description tag of the material
	std::string description;

	// Material flags
	int _materialFlags;

	// cull type
	Material::CullType _cullType;

	// texure repeat type
	ClampType _clampType;

	// Surface flags (nonsolid, areaportal, etc.)
	int _surfaceFlags;

	Material::SurfaceType _surfaceType;

	Material::DeformType _deformType;
    std::vector<IShaderExpression::Ptr> _deformExpressions;
    std::string _deformDeclName;

	// The spectrum this shader is responding to (or emitting in the case of light materials)
	int _spectrum;

    // Sort position (e.g. sort decal == 2)
    float _sortReq;

    // Polygon offset
    float _polygonOffset;

	Material::DecalInfo _decalInfo;

	// Whether this material renders opaque, perforated, etc.
	Material::Coverage _coverage;

    std::string _renderBumpArguments;
    std::string _renderBumpFlatArguments;

    int _parseFlags;

    // The string value specified by the guisurf keyword, if other than entity[2]3]
    std::string _guiDeclName;

    // The three ambient rim colour expressions (empty if not defined)
    IShaderExpression::Ptr _ambientRimColour[3];

    // Private copy ctor, used for cloning
    ShaderTemplate(const ShaderTemplate& other);

public:

    /**
     * \brief
     * Construct a ShaderTemplate.
     */
	ShaderTemplate(const std::string& name) :
        decl::EditableDeclaration<IShaderTemplate>(decl::Type::Material, name),
        _name(name),
        _suppressChangeSignal(false),
        _lightFalloffCubeMapType(IShaderLayer::MapType::Map),
        fogLight(false),
        ambientLight(false),
        blendLight(false),
        _cubicLight(false),
        _materialFlags(0),
        _cullType(Material::CULL_BACK),
        _clampType(CLAMP_REPEAT),
        _surfaceFlags(0),
        _surfaceType(Material::SURFTYPE_DEFAULT),
        _deformType(Material::DEFORM_NONE),
        _spectrum(0),
        _sortReq(SORT_UNDEFINED),	// will be set to default values after the shader has been parsed
        _polygonOffset(0.0f),
        _coverage(Material::MC_UNDETERMINED),
        _parseFlags(0)
	{
        clear();
	}

    void clear();

    // Clone a new instance from this template
    std::shared_ptr<ShaderTemplate> clone();

	/**
	 * Get the name of this shader template.
	 */
	const std::string& getName() const
	{
    	return _name;
	}

	/**
	 * Set the name of this shader template.
	 */
	void setName(const std::string& name)
	{
		_name = name;
	}

	const std::string& getDescription()
	{
		ensureParsed();
		return description;
	}

    void setDescription(const std::string& newDescription)
	{
		ensureParsed();
		description = newDescription;

        onTemplateChanged();
	}

	int getMaterialFlags()
	{
		ensureParsed();
		return _materialFlags;
	}

    void setMaterialFlag(Material::Flags flag)
    {
        ensureParsed();
        _materialFlags |= flag;
        evaluateMacroUsage(); // material flags influence macro usage

        if (flag & Material::FLAG_TRANSLUCENT)
        {
            // Translucent implies noshadows
            _materialFlags |= Material::FLAG_NOSHADOWS;

            // Re-evaluate the material coverage
            _coverage = Material::MC_UNDETERMINED;
            determineCoverage();
        }

        onTemplateChanged();
    }

    void clearMaterialFlag(Material::Flags flag)
    {
        ensureParsed();

        // It's not possible to clear the noshadows flag as long as translucent is active
        if (flag == Material::FLAG_NOSHADOWS && _materialFlags & Material::FLAG_TRANSLUCENT)
        {
            return;
        }

        _materialFlags &= ~flag;
        evaluateMacroUsage(); // material flags influence macro usage

        if (flag & Material::FLAG_TRANSLUCENT)
        {
            // Re-evaluate the material coverage
            _coverage = Material::MC_UNDETERMINED;
            determineCoverage();
        }

        onTemplateChanged();
    }

	Material::CullType getCullType()
	{
		ensureParsed();
		return _cullType;
	}

    void setCullType(Material::CullType type)
    {
        ensureParsed();
        _cullType = type;

        onTemplateChanged();
    }

	ClampType getClampType()
	{
		ensureParsed();
		return _clampType;
	}

    void setClampType(ClampType type)
    {
        ensureParsed();
        _clampType = type;

        onTemplateChanged();
    }

	int getSurfaceFlags()
	{
		ensureParsed();
		return _surfaceFlags;
	}

    void setSurfaceFlag(Material::SurfaceFlags flag)
    {
        ensureParsed();
        _surfaceFlags |= flag;
        evaluateMacroUsage(); // surface flags influence macro usage
        onTemplateChanged();
    }

    void clearSurfaceFlag(Material::SurfaceFlags flag)
    {
        ensureParsed();
        _surfaceFlags &= ~flag;
        evaluateMacroUsage(); // surface flags influence macro usage
        onTemplateChanged();
    }

	Material::SurfaceType getSurfaceType()
	{
		ensureParsed();
		return _surfaceType;
	}

    void setSurfaceType(Material::SurfaceType type)
    {
        ensureParsed();
        _surfaceType = type;
        onTemplateChanged();
    }

	Material::DeformType getDeformType()
	{
		ensureParsed();
		return _deformType;
	}

    IShaderExpression::Ptr getDeformExpression(std::size_t index)
    {
        ensureParsed();

        assert(index >= 0 && index < 3);
        return index < _deformExpressions.size() ? _deformExpressions[index] : IShaderExpression::Ptr();
    }

    std::string getDeformDeclName()
    {
        ensureParsed();
        return _deformDeclName;
    }

	int getSpectrum()
	{
		ensureParsed();
		return _spectrum;
	}

    void setSpectrum(int spectrum)
    {
        ensureParsed();
        _spectrum = spectrum;

        onTemplateChanged();
    }

	const Material::DecalInfo& getDecalInfo()
	{
		ensureParsed();
		return _decalInfo;
	}

    void setDecalInfo(const Material::DecalInfo& info);

	Material::Coverage getCoverage()
	{
		ensureParsed();
		return _coverage;
	}

	const std::vector<Doom3ShaderLayer::Ptr>& getLayers()
	{
		ensureParsed();
		return _layers;
	}

	bool isFogLight()
	{
		ensureParsed();
		return fogLight;
	}

	bool isAmbientLight()
	{
		ensureParsed();
		return ambientLight;
	}

	bool isBlendLight()
	{
		ensureParsed();
		return blendLight;
	}
    
    bool isCubicLight()
	{
		ensureParsed();
		return _cubicLight;
	}

    void setIsAmbientLight(bool newValue)
    {
        ensureParsed();
        ambientLight = newValue;

        onTemplateChanged();
    }

    void setIsBlendLight(bool newValue)
    {
        ensureParsed();
        blendLight = newValue;

        onTemplateChanged();
    }

    void setIsFogLight(bool newValue)
    {
        ensureParsed();
        fogLight = newValue;

        onTemplateChanged();
    }

    void setIsCubicLight(bool newValue)
    {
        ensureParsed();
        _cubicLight = newValue;

        onTemplateChanged();
    }

    float getSortRequest()
    {
		ensureParsed();
        return _sortReq;
    }

    void setSortRequest(float sortRequest)
    {
        ensureParsed();

        _materialFlags |= Material::FLAG_HAS_SORT_DEFINED;
        _sortReq = sortRequest;
        evaluateMacroUsage(); // sort request influences macro usage

        onTemplateChanged();
    }

    void resetSortRequest()
    {
        ensureParsed();

        _materialFlags &= ~Material::FLAG_HAS_SORT_DEFINED;

        // Translucent materials need to be drawn after opaque ones, if not explicitly specified otherwise
        if (_materialFlags & Material::FLAG_TRANSLUCENT)
        {
            _sortReq = Material::SORT_MEDIUM;
        }
        else
        {
            _sortReq = Material::SORT_OPAQUE;
        }

        evaluateMacroUsage(); // sort request influences macro usage

        onTemplateChanged();
    }

    float getPolygonOffset()
    {
		ensureParsed();
        return _polygonOffset;
    }

    void setPolygonOffset(float offset)
    {
        ensureParsed();
        setMaterialFlag(Material::FLAG_POLYGONOFFSET);
        _polygonOffset = offset;

        evaluateMacroUsage(); // polygon offset influences macro usage

        onTemplateChanged();
    }

    const std::string& getBlockContents()
	{
        return getBlockSyntax().contents;
	}

    /**
     * \brief
     * Return the named bindable corresponding to the editor preview texture
     * (qer_editorimage).
     */
	const MapExpressionPtr& getEditorTexture();

    void setEditorImageExpressionFromString(const std::string& expression);

	const MapExpressionPtr& getLightFalloff()
	{
		ensureParsed();
		return _lightFalloff;
	}

    void setLightFalloffExpressionFromString(const std::string& expressionString)
    {
        ensureParsed();
        _lightFalloff = !expressionString.empty() ? 
            MapExpression::createForString(expressionString) : MapExpressionPtr();

        onTemplateChanged();
    }
    
    IShaderLayer::MapType getLightFalloffCubeMapType()
    {
        ensureParsed();
        return _lightFalloffCubeMapType;
    }

    void setLightFalloffCubeMapType(IShaderLayer::MapType type)
    {
        ensureParsed();
        _lightFalloffCubeMapType = type;

        onTemplateChanged();
    }

    std::size_t addLayer(IShaderLayer::Type type);
    void removeLayer(std::size_t index);
    void swapLayerPosition(std::size_t first, std::size_t second);
    std::size_t duplicateLayer(std::size_t index);

	// Add a specific layer to this template
	void addLayer(IShaderLayer::Type type, const MapExpressionPtr& mapExpr);

	// Returns true if this shader template includes a diffusemap stage
	bool hasDiffusemap();

    // Parser hints
    int getParseFlags();

    // renderbump argument string
    std::string getRenderBumpArguments();
    
    // renderbumpflat argument string
    std::string getRenderBumpFlatArguments();

    const std::string& getGuiSurfArgument()
    {
        ensureParsed();
        return _guiDeclName;
    }

    IShaderExpression::Ptr getAmbientRimColourExpression(std::size_t index)
    {
        assert(index < 3);
        return _ambientRimColour[index];
    }

    void onTemplateChanged()
    {
        if (_suppressChangeSignal) return;

        onParsedContentsChanged();
        _sigTemplateChanged.emit();
    }

    // Invoked when one of the shader layers has been modified
    void onLayerChanged()
    {
        onTemplateChanged();
    }

    sigc::signal<void>& sig_TemplateChanged()
    {
        return _sigTemplateChanged;
    }

protected:
    const char* getKeptDelimiters() const override
    {
        return KeptDelimiters;
    }

    void onBeginParsing() override;

    /**
     * Parse a Doom 3 material decl. This is the master parse function, it
     * returns no value but exceptions may be thrown at any stage of the
     * parsing.
     */
    void parseFromTokens(parser::DefTokeniser& tokeniser) override;

    void onSyntaxBlockAssigned(const decl::DeclarationBlockSyntax& block) override;

    std::string generateSyntax() override;

private:
    // Add the given layer and assigns editor preview layer if applicable
	void addLayer(const Doom3ShaderLayer::Ptr& layer);

    // Parse helpers. These scan for possible matches, this is not a
    // recursive-descent parser. Each of these helpers return true 
	// if the token was recognised and parsed
	bool parseShaderFlags(parser::DefTokeniser&, const std::string&);
	bool parseLightKeywords(parser::DefTokeniser&, const std::string&);
	bool parseBlendShortcuts(parser::DefTokeniser&, const std::string&);
	bool parseBlendType(parser::DefTokeniser&, const std::string&);
	bool parseBlendMaps(parser::DefTokeniser&, const std::string&);
    bool parseStageModifiers(parser::DefTokeniser&, const std::string&);
	bool parseSurfaceFlags(parser::DefTokeniser&, const std::string&);
	bool parseMaterialType(parser::DefTokeniser&, const std::string&);
	bool parseCondition(parser::DefTokeniser&, const std::string&);
	IShaderExpression::Ptr parseSingleExpressionTerm(parser::DefTokeniser& tokeniser);
	void parseRenderMapSize(parser::DefTokeniser&, bool optional);

	bool saveLayer();
    void determineCoverage();
    // Checks if the settings of this material justify the use of any macro keywords like DECAL_MACRO
    // Returns true if a macro flag changed, in which case a changed signal should be emitted
    bool evaluateMacroUsage();
};

}
