#pragma once

#include "MapExpression.h"
#include "Doom3ShaderLayer.h"

#include "ishaders.h"
#include "parser/DefTokeniser.h"
#include "math/Vector3.h"

#include <map>
#include <memory>

namespace shaders { class MapExpression; }

namespace shaders
{

/**
 * Data structure storing parsed material information from a material decl. This
 * class parses the decl using a tokeniser and stores the relevant information
 * internally, for later use by a CShader.
 */
class ShaderTemplate final
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

	// Raw material declaration
	std::string _blockContents;
    bool _blockContentsNeedUpdate;

	// Whether the block has been parsed
	bool _parsed;

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
	ShaderTemplate(const std::string& name, const std::string& blockContents) : 
        _name(name),
        _currentLayer(new Doom3ShaderLayer(*this)),
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
        _blockContents(blockContents),
        _blockContentsNeedUpdate(false),
        _parsed(false),
        _parseFlags(0)
	{
		_decalInfo.stayMilliSeconds = 0;
		_decalInfo.fadeMilliSeconds = 0;
		_decalInfo.startColour = Vector4(1,1,1,1);
		_decalInfo.endColour = Vector4(0,0,0,0);
	}

    // Clone a new instance from this template
    std::shared_ptr<ShaderTemplate> clone() const;

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
		if (!_parsed) parseDefinition();
		return description;
	}

    void setDescription(const std::string& newDescription)
	{
		if (!_parsed) parseDefinition();
		description = newDescription;

        onTemplateChanged();
	}

	int getMaterialFlags()
	{
		if (!_parsed) parseDefinition();
		return _materialFlags;
	}

    void setMaterialFlag(Material::Flags flag)
    {
        if (!_parsed) parseDefinition();
        _materialFlags |= flag;
        onTemplateChanged();
    }

    void clearMaterialFlag(Material::Flags flag)
    {
        if (!_parsed) parseDefinition();
        _materialFlags &= ~flag;
        onTemplateChanged();
    }

	Material::CullType getCullType()
	{
		if (!_parsed) parseDefinition();
		return _cullType;
	}

    void setCullType(Material::CullType type)
    {
        if (!_parsed) parseDefinition();
        _cullType = type;

        onTemplateChanged();
    }

	ClampType getClampType()
	{
		if (!_parsed) parseDefinition();
		return _clampType;
	}

    void setClampType(ClampType type)
    {
        if (!_parsed) parseDefinition();
        _clampType = type;

        onTemplateChanged();
    }

	int getSurfaceFlags()
	{
		if (!_parsed) parseDefinition();
		return _surfaceFlags;
	}

    void setSurfaceFlag(Material::SurfaceFlags flag)
    {
        if (!_parsed) parseDefinition();
        _surfaceFlags |= flag;

        onTemplateChanged();
    }

    void clearSurfaceFlag(Material::SurfaceFlags flag)
    {
        if (!_parsed) parseDefinition();
        _surfaceFlags &= ~flag;

        onTemplateChanged();
    }

	Material::SurfaceType getSurfaceType()
	{
		if (!_parsed) parseDefinition();
		return _surfaceType;
	}

    void setSurfaceType(Material::SurfaceType type)
    {
        if (!_parsed) parseDefinition();
        _surfaceType = type;
        onTemplateChanged();
    }

	Material::DeformType getDeformType()
	{
		if (!_parsed) parseDefinition();
		return _deformType;
	}

    IShaderExpression::Ptr getDeformExpression(std::size_t index)
    {
        if (!_parsed) parseDefinition();

        assert(index >= 0 && index < 3);
        return index < _deformExpressions.size() ? _deformExpressions[index] : IShaderExpression::Ptr();
    }

    std::string getDeformDeclName()
    {
        if (!_parsed) parseDefinition();
        return _deformDeclName;
    }

	int getSpectrum()
	{
		if (!_parsed) parseDefinition();
		return _spectrum;
	}

    void setSpectrum(int spectrum)
    {
        if (!_parsed) parseDefinition();
        _spectrum = spectrum;

        onTemplateChanged();
    }

	const Material::DecalInfo& getDecalInfo()
	{
		if (!_parsed) parseDefinition();
		return _decalInfo;
	}

	Material::Coverage getCoverage()
	{
		if (!_parsed) parseDefinition();
		return _coverage;
	}

	const std::vector<Doom3ShaderLayer::Ptr>& getLayers()
	{
		if (!_parsed) parseDefinition();
		return _layers;
	}

	bool isFogLight()
	{
		if (!_parsed) parseDefinition();
		return fogLight;
	}

	bool isAmbientLight()
	{
		if (!_parsed) parseDefinition();
		return ambientLight;
	}

	bool isBlendLight()
	{
		if (!_parsed) parseDefinition();
		return blendLight;
	}
    
    bool isCubicLight()
	{
		if (!_parsed) parseDefinition();
		return _cubicLight;
	}

    void setIsAmbientLight(bool newValue)
    {
        if (!_parsed) parseDefinition();
        ambientLight = newValue;

        onTemplateChanged();
    }

    void setIsBlendLight(bool newValue)
    {
        if (!_parsed) parseDefinition();
        blendLight = newValue;

        onTemplateChanged();
    }

    void setIsFogLight(bool newValue)
    {
        if (!_parsed) parseDefinition();
        fogLight = newValue;

        onTemplateChanged();
    }

    void setIsCubicLight(bool newValue)
    {
        if (!_parsed) parseDefinition();
        _cubicLight = newValue;

        onTemplateChanged();
    }

    float getSortRequest()
    {
		if (!_parsed) parseDefinition();
        return _sortReq;
    }

    void setSortRequest(float sortRequest)
    {
        if (!_parsed) parseDefinition();

        _materialFlags |= Material::FLAG_HAS_SORT_DEFINED;
        _sortReq = sortRequest;

        onTemplateChanged();
    }

    void resetSortRequest()
    {
        if (!_parsed) parseDefinition();

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

        onTemplateChanged();
    }

    float getPolygonOffset()
    {
		if (!_parsed) parseDefinition();
        return _polygonOffset;
    }

    void setPolygonOffset(float offset)
    {
        if (!_parsed) parseDefinition();
        setMaterialFlag(Material::FLAG_POLYGONOFFSET);
        _polygonOffset = offset;

        onTemplateChanged();
    }

	// Sets the raw block definition contents, will be parsed on demand
    void setBlockContents(const std::string& blockContents);

    const std::string& getBlockContents();

    /**
     * \brief
     * Return the named bindable corresponding to the editor preview texture
     * (qer_editorimage).
     */
	const MapExpressionPtr& getEditorTexture();

    void setEditorImageExpressionFromString(const std::string& expression);

	const MapExpressionPtr& getLightFalloff()
	{
		if (!_parsed) parseDefinition();
		return _lightFalloff;
	}

    void setLightFalloffExpressionFromString(const std::string& expressionString)
    {
        if (!_parsed) parseDefinition();
        _lightFalloff = !expressionString.empty() ? 
            MapExpression::createForString(expressionString) : MapExpressionPtr();

        onTemplateChanged();
    }
    
    IShaderLayer::MapType getLightFalloffCubeMapType()
    {
        if (!_parsed) parseDefinition();
        return _lightFalloffCubeMapType;
    }

    void setLightFalloffCubeMapType(IShaderLayer::MapType type)
    {
        if (!_parsed) parseDefinition();
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
        if (!_parsed) parseDefinition();
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

        _blockContentsNeedUpdate = true;
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

private:

	// Add the given layer and assigns editor preview layer if applicable
	void addLayer(const Doom3ShaderLayer::Ptr& layer);

	/**
	 * Parse a Doom 3 material decl. This is the master parse function, it
	 * returns no value but exceptions may be thrown at any stage of the
	 * parsing.
	 */
	void parseDefinition();

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

	bool saveLayer();
    void determineCoverage();
};

/* TYPEDEFS */

// Pointer to ShaderTemplate
typedef std::shared_ptr<ShaderTemplate> ShaderTemplatePtr;

// Map of named ShaderTemplates
typedef std::map<std::string, ShaderTemplatePtr> ShaderTemplateMap;

}
