#ifndef SHADERTEMPLATE_H_
#define SHADERTEMPLATE_H_

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <list>
#include "ishaders.h"

#include "parser/DefTokeniser.h"
#include "stringio.h"
#include "string/pooledstring.h"
#include "stream/stringstream.h"
#include "os/path.h"

#include <boost/algorithm/string/case_conv.hpp>

class ShaderPoolContext
{
};
typedef Static<StringPool, ShaderPoolContext> ShaderPool;
typedef PooledString<ShaderPool> ShaderString;
typedef ShaderString ShaderValue;
typedef ShaderString ShaderVariable;

typedef std::list<ShaderVariable> ShaderParameters;
typedef std::list<ShaderVariable> ShaderArguments;
typedef std::pair<ShaderVariable, ShaderVariable> BlendFuncExpression;

typedef CopiedString TextureExpression;

enum ShaderLanguage
{
  SHADERLANGUAGE_QUAKE3, 
  SHADERLANGUAGE_DOOM3, 
  SHADERLANGUAGE_QUAKE4
};

extern ShaderLanguage g_shaderLanguage;
extern bool g_enableDefaultShaders;
extern bool g_useShaderList;

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
  TextureExpression m_texture;
  BlendFuncExpression m_blendFunc;
  bool m_clampToBorder;
  ShaderValue m_alphaTest;
  ShaderValue m_heightmapScale;

  LayerTemplate() : 
  	m_type(LAYER_NONE), 
  	m_blendFunc("GL_ONE", "GL_ZERO"), 
  	m_clampToBorder(false), 
  	m_alphaTest("-1"), 
  	m_heightmapScale("0")
  {
  }
};

/* -------------------------------------- ShaderTemplate -----------------------------
 */

class ShaderTemplate
{
  std::size_t m_refcount;
  CopiedString m_Name;
public:
  LayerTemplate 	m_currentLayer;
  ShaderParameters 	m_params;

  TextureExpression m_textureName;
  TextureExpression m_diffuse;
  TextureExpression m_bump;
  ShaderValue m_heightmapScale;
  TextureExpression m_specular;
  TextureExpression m_lightFalloffImage;

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

  ShaderTemplate() :
    m_refcount(0),
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

  const char* getName() const
  {
    return m_Name.c_str();
  }
  void setName(const char* name)
  {
    m_Name = name;
  }

  // -----------------------------------------  
  bool parseDoom3(parser::DefTokeniser&);
  bool parseShaderFlags(parser::DefTokeniser&, const std::string&);
  bool parseLightFlags(parser::DefTokeniser&, const std::string&);
  bool parseBlendShortcuts(parser::DefTokeniser&, const std::string&);
  bool parseBlendType(parser::DefTokeniser&, const std::string&);
  bool parseBlendMaps(parser::DefTokeniser&, const std::string&);
  bool parseClamp(parser::DefTokeniser&, const std::string&);
  bool parseMap(parser::DefTokeniser&);
  bool saveLayer();
  
  void CreateDefault(const char *name)
  {
    if(g_enableDefaultShaders)
    {
      m_textureName = name;
    }
    else
    {
      m_textureName = "";
    }
    setName(name);
  }


  class MapLayerTemplate
  {
    TextureExpression m_texture;
    BlendFuncExpression m_blendFunc;
    bool m_clampToBorder;
    ShaderValue m_alphaTest;
  public:
    MapLayerTemplate(const TextureExpression& texture, const BlendFuncExpression& blendFunc, bool clampToBorder, const ShaderValue& alphaTest) :
      m_texture(texture),
      m_blendFunc(blendFunc),
      m_clampToBorder(false),
      m_alphaTest(alphaTest)
    {
    }
    const TextureExpression& texture() const
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
    const ShaderValue& alphaTest() const
    {
      return m_alphaTest;
    }
  };
  typedef std::vector<MapLayerTemplate> MapLayers;
  MapLayers m_layers;
};

#endif /*SHADERTEMPLATE_H_*/
