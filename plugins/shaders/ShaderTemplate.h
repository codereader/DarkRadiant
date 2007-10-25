#ifndef SHADERTEMPLATE_H_
#define SHADERTEMPLATE_H_

#include "MapExpression.h"

#include "ishaders.h"
#include "parser/DefTokeniser.h"

#include <map>
#include <boost/shared_ptr.hpp>

typedef std::pair<std::string, std::string> BlendFuncExpression;

namespace shaders { class IMapExpression; }

enum LayerTypeId
{
  LAYER_NONE,
  LAYER_BLEND,
  LAYER_DIFFUSEMAP,
  LAYER_BUMPMAP,
  LAYER_SPECULARMAP
};

/**
 * Parsed structure for a single material stage.
 */
struct LayerTemplate
{
	// The Map Expression for this stage
	shaders::MapExpressionPtr mapExpr;

  LayerTypeId m_type;
  BlendFuncExpression m_blendFunc;
  bool m_clampToBorder;
  std::string m_alphaTest;
  std::string m_heightmapScale;

	// Constructor
	LayerTemplate() 
	: mapExpr(shaders::MapExpressionPtr()),
	  m_type(LAYER_NONE), 
	  m_blendFunc("GL_ONE", "GL_ZERO"), 
  	  m_clampToBorder(false), 
  	  m_alphaTest("-1"), 
  	  m_heightmapScale("0")
	{ }
};

/**
 * Data structure storing parsed material information from a material decl. This
 * class parses the decl using a tokeniser and stores the relevant information
 * internally, for later use by a CShader.
 */
class ShaderTemplate
{
	// Template name
	std::string _name;
  
public:

	// The current layer (used by the parsing functions)
	LayerTemplate 	m_currentLayer;
  
  	// Vector of LayerTemplates representing each stage in the material
  	typedef std::vector<LayerTemplate> Layers;
	Layers m_layers;

	// Map expressions
	shaders::MapExpressionPtr _texture;
	shaders::MapExpressionPtr _diffuse;
	shaders::MapExpressionPtr _bump;
	shaders::MapExpressionPtr _specular;
	shaders::MapExpressionPtr _lightFalloff;

	/* Light type booleans */	
	bool fogLight;
	bool ambientLight;
	bool blendLight;

	// The description tag of the material
	std::string description;

  int m_nFlags;
  float m_fTrans;

  // alphafunc stuff
  IShader::EAlphaFunc m_AlphaFunc;
  float m_AlphaRef;
  // cull stuff
  IShader::ECull m_Cull;

	/** 
	 * Constructor. Accepts the name to use for this template.
	 */
	ShaderTemplate(const std::string& name) 
	: _name(name),
      fogLight(false),
      ambientLight(false),
      blendLight(false)
	{
    	m_nFlags = 0;
    	m_fTrans = 1.0f;    
	}

	/**
	 * Get the name of this shader template.
	 */
	std::string getName() const {
    	return _name;
	}
	
	/**
	 * Set the name of this shader template.
	 */
	void setName(const std::string& name) {
		_name = name;
	}

	/**
	 * Parse a Doom 3 material decl. This is the master parse function, it
	 * returns no value but exceptions may be thrown at any stage of the 
	 * parsing.
	 */
	void parseDoom3(parser::DefTokeniser&);

	bool parseShaderFlags(parser::DefTokeniser&, const std::string&);
	bool parseLightFlags(parser::DefTokeniser&, const std::string&);
	bool parseBlendShortcuts(parser::DefTokeniser&, const std::string&);
	bool parseBlendType(parser::DefTokeniser&, const std::string&);
	bool parseBlendMaps(parser::DefTokeniser&, const std::string&);
	bool parseClamp(parser::DefTokeniser&, const std::string&);
	bool saveLayer();
  
};

/* TYPEDEFS */

// Pointer to ShaderTemplate
typedef boost::shared_ptr<ShaderTemplate> ShaderTemplatePtr;

// Map of named ShaderTemplates
typedef std::map<std::string, ShaderTemplatePtr> ShaderTemplateMap;

#endif /*SHADERTEMPLATE_H_*/
