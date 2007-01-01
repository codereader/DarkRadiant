#ifndef SHADERTEMPLATE_H_
#define SHADERTEMPLATE_H_

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <list>
#include "ishaders.h"

#include "parser/DefTokeniser.h"
#include "stringio.h"
#include "stream/stringstream.h"
#include "os/path.h"

#include <boost/algorithm/string/case_conv.hpp>

typedef std::list<std::string> StringList;
typedef std::pair<std::string, std::string> BlendFuncExpression;

enum LayerTypeId
{
  LAYER_NONE,
  LAYER_BLEND,
  LAYER_DIFFUSEMAP,
  LAYER_BUMPMAP,
  LAYER_SPECULARMAP
};

/* -------------------------------------- LayerTemplate -----------------------------
 */
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
	std::size_t m_refcount;
  
	// Template name
	std::string _name;
  
public:
  LayerTemplate 	m_currentLayer;
  StringList 	m_params;

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
	: m_refcount(0),
	  _name(name),
	  m_textureName(""),
      fogLight(false),
      ambientLight(false),
      blendLight(false)
	{
    	m_nFlags = 0;
    	m_fTrans = 1.0f;    
	}

  void IncRef()
  {
    ++m_refcount;
  }
  void DecRef() 
  {
    ASSERT_MESSAGE(m_refcount != 0, "shader reference-count going below zero");
    if(--m_refcount == 0)
    {
      delete this;
    }
  }

  std::size_t refcount()
  {
    return m_refcount;
  }

	std::string getName() const {
    	return _name;
	}
	
	void setName(const std::string& name) {
		_name = name;
	}

	/* Parse functions. These are NOT recursive descent parsers, instead they
	 * each run through a list of keywords of interest, setting internal state
	 * if appropriate and then returning true or false depending on whether they
	 * found anything or not. */
	bool parseDoom3(parser::DefTokeniser&);
	bool parseShaderFlags(parser::DefTokeniser&, const std::string&);
	bool parseLightFlags(parser::DefTokeniser&, const std::string&);
	bool parseBlendShortcuts(parser::DefTokeniser&, const std::string&);
	bool parseBlendType(parser::DefTokeniser&, const std::string&);
	bool parseBlendMaps(parser::DefTokeniser&, const std::string&);
	bool parseClamp(parser::DefTokeniser&, const std::string&);
	bool parseMap(parser::DefTokeniser&);
	bool saveLayer();
  
  class MapLayerTemplate
  {
    std::string m_texture;
    BlendFuncExpression m_blendFunc;
    bool m_clampToBorder;
    std::string m_alphaTest;
  public:
    MapLayerTemplate(const std::string& texture, const BlendFuncExpression& blendFunc, bool clampToBorder, const std::string& alphaTest) :
      m_texture(texture),
      m_blendFunc(blendFunc),
      m_clampToBorder(false),
      m_alphaTest(alphaTest)
    {
    }
    const std::string& texture() const
    {
      return m_texture;
    }
    const BlendFuncExpression& blendFunc() const
    {
      return m_blendFunc;
    }
    bool clampToBorder() const
    {
      return m_clampToBorder;
    }
    const std::string& alphaTest() const
    {
      return m_alphaTest;
    }
  };
  typedef std::vector<MapLayerTemplate> MapLayers;
  MapLayers m_layers;
};

#endif /*SHADERTEMPLATE_H_*/
