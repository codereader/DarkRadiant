#ifndef SHADERTEMPLATE_H_
#define SHADERTEMPLATE_H_

#include "ishaders.h"
#include "parser/DefTokeniser.h"

#include <map>
#include <boost/shared_ptr.hpp>

typedef std::pair<std::string, std::string> BlendFuncExpression;

enum LayerTypeId
{
  LAYER_NONE,
  LAYER_BLEND,
  LAYER_DIFFUSEMAP,
  LAYER_BUMPMAP,
  LAYER_SPECULARMAP
};

class LayerTemplate
{
public:
  LayerTypeId m_type;
  std::string m_texture;
  BlendFuncExpression m_blendFunc;
  bool m_clampToBorder;
  std::string m_alphaTest;
  std::string m_heightmapScale;

  LayerTemplate() : 
  	m_type(LAYER_NONE), 
  	m_blendFunc("GL_ONE", "GL_ZERO"), 
  	m_clampToBorder(false), 
  	m_alphaTest("-1"), 
  	m_heightmapScale("0")
  {
  }
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

	// Image maps
	std::string m_textureName;
	std::string m_diffuse;
	std::string m_bump;
	std::string m_heightmapScale;
	std::string m_specular;
	std::string m_lightFalloffImage;

	/* Light type booleans */	
	bool fogLight;
	bool ambientLight;
	bool blendLight;

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
	  m_textureName(""),
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

	// Parse an image map expression, possibly recursively if addnormals() or
	// other functions are used	
	bool parseMap(parser::DefTokeniser&);

	// Parse an addnormals() expression
	void parseAddNormals(parser::DefTokeniser&);

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
