#pragma once

#include "MapExpression.h"
#include "Doom3ShaderLayer.h"

#include "ishaders.h"
#include "parser/DefTokeniser.h"
#include "math/Vector3.h"

#include <map>
#include <boost/shared_ptr.hpp>

namespace shaders { class MapExpression; }

namespace shaders
{

/**
 * Data structure storing parsed material information from a material decl. This
 * class parses the decl using a tokeniser and stores the relevant information
 * internally, for later use by a CShader.
 */
class ShaderTemplate
{
private:
	static const int SORT_UNDEFINED = -99999;	// undefined sort number, will be replaced after parsing

	// Template name
	std::string _name;

	// Temporary current layer (used by the parsing functions)
	Doom3ShaderLayerPtr _currentLayer;

public:

  	// Vector of LayerTemplates representing each stage in the material
  	typedef std::vector<Doom3ShaderLayerPtr> Layers;

private:
	Layers _layers;

    // Editorimage texture
	NamedBindablePtr _editorTex;

	// Map expressions
	shaders::MapExpressionPtr _lightFalloff;

	/* Light type booleans */
	bool fogLight;
	bool ambientLight;
	bool blendLight;

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

	// The spectrum this shader is responding to (or emitting in the case of light materials)
	int _spectrum;

    // Sort position (e.g. sort decal == 2)
    int _sortReq;

    // Polygon offset
    float _polygonOffset;

	Material::DecalInfo _decalInfo;

	// Whether this material renders opaque, perforated, etc.
	Material::Coverage _coverage;

	// Raw material declaration
	std::string _blockContents;

	// Whether the block has been parsed
	bool _parsed;

public:

    /**
     * \brief
     * Construct a ShaderTemplate.
     */
	ShaderTemplate(const std::string& name, const std::string& blockContents)
	: _name(name),
      _currentLayer(new Doom3ShaderLayer(*this)),
      fogLight(false),
      ambientLight(false),
      blendLight(false),
	  _materialFlags(0),
	  _cullType(Material::CULL_BACK),
	  _clampType(CLAMP_REPEAT),
	  _surfaceFlags(0),
	  _surfaceType(Material::SURFTYPE_DEFAULT),
	  _deformType(Material::DEFORM_NONE),
	  _spectrum(-1),
      _sortReq(SORT_UNDEFINED),	// will be set to default values after the shader has been parsed
      _polygonOffset(0.0f),
	  _coverage(Material::MC_UNDETERMINED),
	  _blockContents(blockContents),
	  _parsed(false)
	{
		_decalInfo.stayMilliSeconds = 0;
		_decalInfo.fadeMilliSeconds = 0;
		_decalInfo.startColour = Vector4(1,1,1,1);
		_decalInfo.endColour = Vector4(0,0,0,0);
	}

	/**
	 * Get the name of this shader template.
	 */
	std::string getName() const
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

	int getMaterialFlags()
	{
		if (!_parsed) parseDefinition();
		return _materialFlags;
	}

	Material::CullType getCullType()
	{
		if (!_parsed) parseDefinition();
		return _cullType;
	}

	ClampType getClampType()
	{
		if (!_parsed) parseDefinition();
		return _clampType;
	}

	int getSurfaceFlags()
	{
		if (!_parsed) parseDefinition();
		return _surfaceFlags;
	}

	Material::SurfaceType getSurfaceType()
	{
		if (!_parsed) parseDefinition();
		return _surfaceType;
	}

	Material::DeformType getDeformType()
	{
		if (!_parsed) parseDefinition();
		return _deformType;
	}

	int getSpectrum()
	{
		if (!_parsed) parseDefinition();
		return _spectrum;
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

	const Layers& getLayers()
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

    int getSortRequest()
    {
		if (!_parsed) parseDefinition();
        return _sortReq;
    }

    float getPolygonOffset()
    {
		if (!_parsed) parseDefinition();
        return _polygonOffset;
    }

	// Sets the raw block definition contents, will be parsed on demand
	void setBlockContents(const std::string& blockContents)
	{
		_blockContents = blockContents;
	}

	const std::string& getBlockContents() const
	{
		return _blockContents;
	}

    /**
     * \brief
     * Return the named bindable corresponding to the editor preview texture
     * (qer_editorimage).
     */
	NamedBindablePtr getEditorTexture();

	const shaders::MapExpressionPtr& getLightFalloff()
	{
		if (!_parsed) parseDefinition();
		return _lightFalloff;
	}

	// Add a specific layer to this template
	void addLayer(ShaderLayer::Type type, const MapExpressionPtr& mapExpr);

	// Returns true if this shader template includes a diffusemap stage
	bool hasDiffusemap();

private:

	// Add the given layer and assigns editor preview layer if applicable
	void addLayer(const Doom3ShaderLayerPtr& layer);

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
	bool parseCondition(parser::DefTokeniser&, const std::string&);
	IShaderExpressionPtr parseSingleExpressionTerm(parser::DefTokeniser& tokeniser);

	bool saveLayer();

};

/* TYPEDEFS */

// Pointer to ShaderTemplate
typedef boost::shared_ptr<ShaderTemplate> ShaderTemplatePtr;

// Map of named ShaderTemplates
typedef std::map<std::string, ShaderTemplatePtr> ShaderTemplateMap;

}
