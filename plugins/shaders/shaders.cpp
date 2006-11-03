/*
Copyright (c) 2001, Loki software, inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list 
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Neither the name of Loki software nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY 
DIRECT,INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

//
// Shaders Manager Plugin
//
// Leonardo Zide (leo@lokigames.com)
//

#include "shaders.h"
#include "MissingXMLNodeException.h"
#include "parser/ParseException.h"
#include "parser/DefTokeniser.h"

#include "ShaderTemplate.h"

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <list>

#include "ifilesystem.h"
#include "ishaders.h"
#include "iscriplib.h"
#include "itextures.h"
#include "qerplugin.h"
#include "irender.h"

#include <glib/gslist.h>

#include "debugging/debugging.h"
#include "string/pooledstring.h"
#include "math/vector.h"
#include "generic/callback.h"
#include "generic/referencecounted.h"
#include "stream/memstream.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"
#include "os/path.h"
#include "os/dir.h"
#include "os/file.h"
#include "stringio.h"
#include "shaderlib.h"
#include "texturelib.h"
#include "cmdlib.h"
#include "moduleobservers.h"
#include "archivelib.h"
#include "imagelib.h"

#include "xmlutil/Node.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>

/* GLOBALS */

namespace {

	// The shader extension (e.g. MTR) loaded from the gamedescriptor
	std::string g_shadersExtension;
	
}

bool g_enableDefaultShaders = true;
ShaderLanguage g_shaderLanguage = SHADERLANGUAGE_QUAKE3;
bool g_useShaderList = true;
_QERPlugImageTable* g_bitmapModule = 0;
const char* g_texturePrefix = "textures/";



void ActiveShaders_IteratorBegin();
bool ActiveShaders_IteratorAtEnd();
IShader *ActiveShaders_IteratorCurrent();
void ActiveShaders_IteratorIncrement();
Callback g_ActiveShadersChangedNotify;

void FreeShaders();
void loadShaderFile (const char *filename);
qtexture_t *Texture_ForName (const char *filename);


/*!
NOTE TTimo: there is an important distinction between SHADER_NOT_FOUND and SHADER_NOTEX:
SHADER_NOT_FOUND means we didn't find the raw texture or the shader for this
SHADER_NOTEX means we recognize this as a shader script, but we are missing the texture to represent it
this was in the initial design of the shader code since early GtkRadiant alpha, and got sort of foxed in 1.2 and put back in
*/

Image* loadBitmap(void* environment, const char* name)
{
  DirectoryArchiveFile file(name, name);
  if(!file.failed())
  {
    return g_bitmapModule->loadImage(file);
  }
  return 0;
}

inline byte* getPixel(byte* pixels, int width, int height, int x, int y)
{
  return pixels + (((((y + height) % height) * width) + ((x + width) % width)) * 4);
}

class KernelElement
{
public:
  int x, y;
  float w;
};

Image& convertHeightmapToNormalmap(Image& heightmap, float scale)
{
  int w = heightmap.getWidth();
  int h = heightmap.getHeight();
  
  Image& normalmap = *(new RGBAImage(heightmap.getWidth(), heightmap.getHeight()));
  
  byte* in = heightmap.getRGBAPixels();
  byte* out = normalmap.getRGBAPixels();

#if 1
  // no filtering
  const int kernelSize = 2;
  KernelElement kernel_du[kernelSize] = {
    {-1, 0,-0.5f },
    { 1, 0, 0.5f }
  };
  KernelElement kernel_dv[kernelSize] = {
    { 0, 1, 0.5f },
    { 0,-1,-0.5f }
  };
#else
  // 3x3 Prewitt
  const int kernelSize = 6;
  KernelElement kernel_du[kernelSize] = {
    {-1, 1,-1.0f },
    {-1, 0,-1.0f },
    {-1,-1,-1.0f },
    { 1, 1, 1.0f },
    { 1, 0, 1.0f },
    { 1,-1, 1.0f }
  };
  KernelElement kernel_dv[kernelSize] = {
    {-1, 1, 1.0f },
    { 0, 1, 1.0f },
    { 1, 1, 1.0f },
    {-1,-1,-1.0f },
    { 0,-1,-1.0f },
    { 1,-1,-1.0f }
  };
#endif

  int x, y = 0;
  while( y < h )
  {
    x = 0;
    while( x < w )
    {
      float du = 0;
      for(KernelElement* i = kernel_du; i != kernel_du + kernelSize; ++i)
      {
        du += (getPixel(in, w, h, x + (*i).x, y + (*i).y)[0] / 255.0) * (*i).w;
      }
      float dv = 0;
      for(KernelElement* i = kernel_dv; i != kernel_dv + kernelSize; ++i)
      {
        dv += (getPixel(in, w, h, x + (*i).x, y + (*i).y)[0] / 255.0) * (*i).w;
      }

      float nx = -du * scale;
      float ny = -dv * scale;
      float nz = 1.0;

      // Normalize      
      float norm = 1.0/sqrt(nx*nx + ny*ny + nz*nz);
      out[0] = float_to_integer(((nx * norm) + 1) * 127.5);
      out[1] = float_to_integer(((ny * norm) + 1) * 127.5);
      out[2] = float_to_integer(((nz * norm) + 1) * 127.5);
      out[3] = 255;
     
      x++;
      out += 4;
    }
    
    y++;
  }
  
  return normalmap;
}

Image* loadHeightmap(void* environment, const char* name)
{
  Image* heightmap = GlobalTexturesCache().loadImage(name);
  if(heightmap != 0)
  {
    Image& normalmap = convertHeightmapToNormalmap(*heightmap, *reinterpret_cast<float*>(environment));
    heightmap->release();
    return &normalmap;
  }
  return 0;
}


Image* loadSpecial(void* environment, const char* name)
{
  if(*name == '_') // special image
  {
    StringOutputStream bitmapName(256);
    bitmapName << GlobalRadiant().getAppPath() << "bitmaps/" << name + 1 << ".bmp";
    Image* image = loadBitmap(environment, bitmapName.c_str());
    if(image != 0)
    {
      return image;
    }
  }
  return GlobalTexturesCache().loadImage(name);
}

typedef SmartPointer<ShaderTemplate> ShaderTemplatePointer;
typedef std::map<CopiedString, ShaderTemplatePointer> ShaderTemplateMap;

ShaderTemplateMap g_shaders;
ShaderTemplateMap g_shaderTemplates;

ShaderTemplate* findTemplate(const char* name)
{
  ShaderTemplateMap::iterator i = g_shaderTemplates.find(name);
  if(i != g_shaderTemplates.end())
  {
    return (*i).second.get();
  }
  return 0;
}

class ShaderDefinition
{
public:
  ShaderDefinition(ShaderTemplate* shaderTemplate, const ShaderArguments& args, const char* filename)
    : shaderTemplate(shaderTemplate), args(args), filename(filename)
  {
  }
  ShaderTemplate* shaderTemplate;
  ShaderArguments args;
  const char* filename;
};

typedef std::map<CopiedString, ShaderDefinition> ShaderDefinitionMap;

ShaderDefinitionMap g_shaderDefinitions;

const char* evaluateShaderValue(const char* value, const ShaderParameters& params, const ShaderArguments& args)
{
  ShaderArguments::const_iterator j = args.begin();
  for(ShaderParameters::const_iterator i = params.begin(); i != params.end(); ++i, ++j)
  {
    const char* other = (*i).c_str();
    if(string_equal(value, other))
    {
      return (*j).c_str();
    }
  }
  return value;
}

///\todo BlendFunc parsing
BlendFunc evaluateBlendFunc(const BlendFuncExpression& blendFunc, const ShaderParameters& params, const ShaderArguments& args)
{
  return BlendFunc(BLEND_ONE, BLEND_ZERO);
}

qtexture_t* evaluateTexture(const TextureExpression& texture, const ShaderParameters& params, const ShaderArguments& args, const LoadImageCallback& loader = GlobalTexturesCache().defaultLoader())
{
  StringOutputStream result(64);
  const char* expression = texture.c_str();
  const char* end = expression + string_length(expression);
  if(!string_empty(expression))
  {
    for(;;)
    {
      const char* best = end;
      const char* bestParam = 0;
      const char* bestArg = 0;
      ShaderArguments::const_iterator j = args.begin();
      for(ShaderParameters::const_iterator i = params.begin(); i != params.end(); ++i, ++j)
      {
        const char* found = strstr(expression, (*i).c_str());
        if(found != 0 && found < best)
        {
          best = found;
          bestParam = (*i).c_str();
          bestArg = (*j).c_str();
        }
      }
      if(best != end)
      {
        result << StringRange(expression, best);
        result << PathCleaned(bestArg);
        expression = best + string_length(bestParam);
      }
      else
      {
        break;
      }
    }
    result << expression;
  }
  return GlobalTexturesCache().capture(loader, result.c_str());
}

float evaluateFloat(const ShaderValue& value, const ShaderParameters& params, const ShaderArguments& args)
{
  const char* result = evaluateShaderValue(value.c_str(), params, args);
  float f;
  if(!string_parse_float(result, f))
  {
    globalErrorStream() << "parsing float value failed: " << makeQuoted(result) << "\n";
  }
  return f;
}

BlendFactor evaluateBlendFactor(const ShaderValue& value, const ShaderParameters& params, const ShaderArguments& args)
{
  const char* result = evaluateShaderValue(value.c_str(), params, args);

  if(string_equal_nocase(result, "gl_zero"))
  {
    return BLEND_ZERO;
  }
  if(string_equal_nocase(result, "gl_one"))
  {
    return BLEND_ONE;
  }
  if(string_equal_nocase(result, "gl_src_color"))
  {
    return BLEND_SRC_COLOUR;
  }
  if(string_equal_nocase(result, "gl_one_minus_src_color"))
  {
    return BLEND_ONE_MINUS_SRC_COLOUR;
  }
  if(string_equal_nocase(result, "gl_src_alpha"))
  {
    return BLEND_SRC_ALPHA;
  }
  if(string_equal_nocase(result, "gl_one_minus_src_alpha"))
  {
    return BLEND_ONE_MINUS_SRC_ALPHA;
  }
  if(string_equal_nocase(result, "gl_dst_color"))
  {
    return BLEND_DST_COLOUR;
  }
  if(string_equal_nocase(result, "gl_one_minus_dst_color"))
  {
    return BLEND_ONE_MINUS_DST_COLOUR;
  }
  if(string_equal_nocase(result, "gl_dst_alpha"))
  {
    return BLEND_DST_ALPHA;
  }
  if(string_equal_nocase(result, "gl_one_minus_dst_alpha"))
  {
    return BLEND_ONE_MINUS_DST_ALPHA;
  }
  if(string_equal_nocase(result, "gl_src_alpha_saturate"))
  {
    return BLEND_SRC_ALPHA_SATURATE;
  }

  globalErrorStream() << "parsing blend-factor value failed: " << makeQuoted(result) << "\n";
  return BLEND_ZERO;
}

class CShader : public IShader
{
  std::size_t m_refcount;

  const ShaderTemplate& m_template;
  const ShaderArguments& m_args;
  const char* m_filename;
  // name is shader-name, otherwise texture-name (if not a real shader)
  CopiedString m_Name;

  qtexture_t* m_pTexture;
  qtexture_t* m_notfound;
  qtexture_t* m_pDiffuse;
  float m_heightmapScale;
  qtexture_t* m_pBump;
  qtexture_t* m_pSpecular;
  qtexture_t* m_pLightFalloffImage;
  BlendFunc m_blendFunc;

  bool m_bInUse;


public:
  static bool m_lightingEnabled;

  CShader(const ShaderDefinition& definition) :
    m_refcount(0),
    m_template(*definition.shaderTemplate),
    m_args(definition.args),
    m_filename(definition.filename),
    m_blendFunc(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA),
    m_bInUse(false)
  {
    m_pTexture = 0;
    m_pDiffuse = 0;
    m_pBump = 0;
    m_pSpecular = 0;

    m_notfound = 0;

    realise();
  }
  virtual ~CShader()
  {
    unrealise();

    ASSERT_MESSAGE(m_refcount == 0, "deleting active shader");
  }

  // IShaders implementation -----------------
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

  // get/set the qtexture_t* Radiant uses to represent this shader object
  qtexture_t* getTexture() const
  {
    return m_pTexture;
  }
  qtexture_t* getDiffuse() const
  {
    return m_pDiffuse;
  }

	// Return bumpmap if it exists, otherwise _flat
  qtexture_t* getBump() const
  {
    	return m_pBump;
  }
  qtexture_t* getSpecular() const
  {
    return m_pSpecular;
  }
  // get shader name
  const char* getName() const
  {
    return m_Name.c_str();
  }
  bool IsInUse() const
  {
    return m_bInUse;
  }
  void SetInUse(bool bInUse)
  {
    m_bInUse = bInUse;
    g_ActiveShadersChangedNotify();
  }
  // get the shader flags
  int getFlags() const
  {
    return m_template.m_nFlags;
  }
  // get the transparency value
  float getTrans() const
  {
    return m_template.m_fTrans;
  }
  // test if it's a true shader, or a default shader created to wrap around a texture
  bool IsDefault() const 
  {
    return string_empty(m_filename);
  }
  // get the alphaFunc
  void getAlphaFunc(EAlphaFunc *func, float *ref) { *func = m_template.m_AlphaFunc; *ref = m_template.m_AlphaRef; };
  BlendFunc getBlendFunc() const
  {
    return m_blendFunc;
  }
  // get the cull type
  ECull getCull()
  {
    return m_template.m_Cull;
  };
  // get shader file name (ie the file where this one is defined)
  const char* getShaderFileName() const
  {
    return m_filename;
  }
  // -----------------------------------------

  void realise()
  {
    m_pTexture = evaluateTexture(m_template.m_textureName, m_template.m_params, m_args);

    if(m_pTexture->texture_number == 0)
    {
      m_notfound = m_pTexture;

      {
        StringOutputStream name(256);
        name << GlobalRadiant().getAppPath() << "bitmaps/" << (IsDefault() ? "notex.bmp" : "shadernotex.bmp");
        m_pTexture = GlobalTexturesCache().capture(LoadImageCallback(0, loadBitmap), name.c_str());
      }
    }

    realiseLighting();
  }

  void unrealise()
  {
    GlobalTexturesCache().release(m_pTexture);

    if(m_notfound != 0)
    {
      GlobalTexturesCache().release(m_notfound);
    }

    unrealiseLighting();
  }

	// Parse and load image maps for this shader

  void realiseLighting()
  {
      LoadImageCallback loader = GlobalTexturesCache().defaultLoader();
      if(!string_empty(m_template.m_heightmapScale.c_str()))
      {
        m_heightmapScale = evaluateFloat(m_template.m_heightmapScale, m_template.m_params, m_args);
        loader = LoadImageCallback(&m_heightmapScale, loadHeightmap);
      }

		// Set up the diffuse, bump and specular stages. Bump and specular will be set to defaults
		// _flat and _black respectively, if an image map is not specified in the material.

		m_pDiffuse = evaluateTexture(m_template.m_diffuse, m_template.m_params, m_args);

    	std::string flatName = std::string(GlobalRadiant().getAppPath()) + "bitmaps/_flat.bmp";
		m_pBump = evaluateTexture(m_template.m_bump, m_template.m_params, m_args, loader);
		if (m_pBump == 0 || m_pBump->texture_number == 0) {
			GlobalTexturesCache().release(m_pBump); // release old object first
			m_pBump = GlobalTexturesCache().capture(LoadImageCallback(0, loadBitmap), flatName.c_str());
		}
		
		std::string blackName = std::string(GlobalRadiant().getAppPath()) + "bitmaps/_black.bmp";
		m_pSpecular = evaluateTexture(m_template.m_specular, m_template.m_params, m_args);
		if (m_pSpecular == 0 || m_pSpecular->texture_number == 0) {
			GlobalTexturesCache().release(m_pSpecular);
			m_pSpecular = GlobalTexturesCache().capture(LoadImageCallback(0, loadBitmap), blackName.c_str());
		}
		
		// Get light falloff image
		
		m_pLightFalloffImage = evaluateTexture(m_template.m_lightFalloffImage, m_template.m_params, m_args);

      for(ShaderTemplate::MapLayers::const_iterator i = m_template.m_layers.begin(); i != m_template.m_layers.end(); ++i)
      {
        m_layers.push_back(evaluateLayer(*i, m_template.m_params, m_args));
      }

      if(m_layers.size() == 1)
      {
        const BlendFuncExpression& blendFunc = m_template.m_layers.front().blendFunc();
        if(!string_empty(blendFunc.second.c_str()))
        {
          m_blendFunc = BlendFunc(
            evaluateBlendFactor(blendFunc.first.c_str(), m_template.m_params, m_args),
            evaluateBlendFactor(blendFunc.second.c_str(), m_template.m_params, m_args)
          );
        }
        else
        {
          const char* blend = evaluateShaderValue(blendFunc.first.c_str(), m_template.m_params, m_args);

          if(string_equal_nocase(blend, "add"))
          {
            m_blendFunc = BlendFunc(BLEND_ONE, BLEND_ONE);
          }
          else if(string_equal_nocase(blend, "filter"))
          {
            m_blendFunc = BlendFunc(BLEND_DST_COLOUR, BLEND_ZERO);
          }
          else if(string_equal_nocase(blend, "blend"))
          {
            m_blendFunc = BlendFunc(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
          }
          else
          {
            globalErrorStream() << "parsing blend value failed: " << makeQuoted(blend) << "\n";
          }
        }
      }
  }

  void unrealiseLighting()
  {
      GlobalTexturesCache().release(m_pDiffuse);
      GlobalTexturesCache().release(m_pBump);
      GlobalTexturesCache().release(m_pSpecular);

      GlobalTexturesCache().release(m_pLightFalloffImage);

      for(MapLayers::iterator i = m_layers.begin(); i != m_layers.end(); ++i)
      {
        GlobalTexturesCache().release((*i).texture());
      }
      m_layers.clear();

      m_blendFunc = BlendFunc(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
  }

  // set shader name
  void setName(const char* name)
  {
    m_Name = name;
  }

  class MapLayer : public ShaderLayer
  {
    qtexture_t* m_texture;
    BlendFunc m_blendFunc;
    bool m_clampToBorder;
    float m_alphaTest;
  public:
    MapLayer(qtexture_t* texture, BlendFunc blendFunc, bool clampToBorder, float alphaTest) :
      m_texture(texture),
      m_blendFunc(blendFunc),
      m_clampToBorder(false),
      m_alphaTest(alphaTest)
    {
    }
    qtexture_t* texture() const
    {
      return m_texture;
    }
    BlendFunc blendFunc() const
    {
      return m_blendFunc;
    }
    bool clampToBorder() const
    {
      return m_clampToBorder;
    }
    float alphaTest() const
    {
      return m_alphaTest;
    }
  };

  static MapLayer evaluateLayer(const ShaderTemplate::MapLayerTemplate& layerTemplate, const ShaderParameters& params, const ShaderArguments& args)
  {
    return MapLayer(
      evaluateTexture(layerTemplate.texture(), params, args),
      evaluateBlendFunc(layerTemplate.blendFunc(), params, args),
      layerTemplate.clampToBorder(),
      evaluateFloat(layerTemplate.alphaTest(), params, args)
    );
  }

  typedef std::vector<MapLayer> MapLayers;
  MapLayers m_layers;

  const ShaderLayer* firstLayer() const
  {
    if(m_layers.empty())
    {
      return 0;
    }
    return &m_layers.front();
  }
  void forEachLayer(const ShaderLayerCallback& callback) const
  {
    for(MapLayers::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i)
    {
      callback(*i);
    }
  }
  
	/* Required IShader light type predicates */
	
	bool isAmbientLight() const {
		return m_template.ambientLight;
	}

	bool isBlendLight() const {
		return m_template.blendLight;
	}

	bool isFogLight() const {
		return m_template.fogLight;
	}

  qtexture_t* lightFalloffImage() const
  {
    if(!string_empty(m_template.m_lightFalloffImage.c_str()))
    {
      return m_pLightFalloffImage;
    }
    return 0;
  }
};

bool CShader::m_lightingEnabled = false;

typedef SmartPointer<CShader> ShaderPointer;
typedef std::map<CopiedString, ShaderPointer, shader_less_t> shaders_t;

shaders_t g_ActiveShaders;

static shaders_t::iterator g_ActiveShadersIterator;

void ActiveShaders_IteratorBegin()
{
  g_ActiveShadersIterator = g_ActiveShaders.begin();
}

bool ActiveShaders_IteratorAtEnd()
{
  return g_ActiveShadersIterator == g_ActiveShaders.end();
}

IShader *ActiveShaders_IteratorCurrent()
{
  return static_cast<CShader*>(g_ActiveShadersIterator->second);
}

void ActiveShaders_IteratorIncrement()
{
  ++g_ActiveShadersIterator;
}

void debug_check_shaders(shaders_t& shaders)
{
  for(shaders_t::iterator i = shaders.begin(); i != shaders.end(); ++i)
  {
    ASSERT_MESSAGE(i->second->refcount() == 1, "orphan shader still referenced");
  }
}

// will free all GL binded qtextures and shaders
// NOTE: doesn't make much sense out of Radiant exit or called during a reload
void FreeShaders()
{
  // reload shaders
  // empty the actives shaders list
  debug_check_shaders(g_ActiveShaders);
  g_ActiveShaders.clear();
  g_shaders.clear();
  g_shaderTemplates.clear();
  g_shaderDefinitions.clear();
  g_ActiveShadersChangedNotify();
}

std::list<CopiedString> g_shaderFilenames;

typedef FreeCaller1<const char*, loadShaderFile> LoadShaderFileCaller;

CShader* Try_Shader_ForName(const char* name)
{
  {
    shaders_t::iterator i = g_ActiveShaders.find(name);
    if(i != g_ActiveShaders.end())
    {
      return (*i).second;
    }
  }

  // not found, create it
  ShaderDefinitionMap::iterator i = g_shaderDefinitions.find(name);
  if(i == g_shaderDefinitions.end())
  {
    ShaderTemplatePointer shaderTemplate(new ShaderTemplate());
    shaderTemplate->CreateDefault(name);
    g_shaderTemplates.insert(ShaderTemplateMap::value_type(shaderTemplate->getName(), shaderTemplate));

    i = g_shaderDefinitions.insert(ShaderDefinitionMap::value_type(name, ShaderDefinition(shaderTemplate.get(), ShaderArguments(), ""))).first;
  }

  ShaderPointer pShader(new CShader((*i).second));
  pShader->setName(name);
  g_ActiveShaders.insert(shaders_t::value_type(name, pShader));
  g_ActiveShadersChangedNotify();
  return pShader;
}

// the list of scripts/*.shader files we need to work with
// those are listed in shaderlist file
GSList *l_shaderfiles = 0;

GSList* Shaders_getShaderFileList()
{
  return l_shaderfiles;
}

void ShaderList_addShaderFile(const char* dirstring)
{
	l_shaderfiles = g_slist_append(l_shaderfiles, strdup(dirstring));
}

typedef FreeCaller1<const char*, ShaderList_addShaderFile> AddShaderFileCaller;

void FreeShaderList()
{
  while(l_shaderfiles != 0)
  {
    free(l_shaderfiles->data);
    l_shaderfiles = g_slist_remove(l_shaderfiles, l_shaderfiles->data);
  }
}

/* Normalises a given (raw) shadername (slash conversion, extension removal, ...) 
 * and inserts a new shader 
 */
ShaderTemplatePointer parseShaderName(std::string& rawName) 
{
	boost::algorithm::replace_all(rawName, "\\", "/"); // use forward slashes
	boost::algorithm::to_lower(rawName); // use lowercase

	// Remove the extension if present
	size_t dotPos = rawName.rfind(".");
	if (dotPos != std::string::npos) {
		rawName = rawName.substr(0, dotPos);
	}
	
	ShaderTemplatePointer shaderTemplate(new ShaderTemplate());
    shaderTemplate->setName(rawName.c_str());
	g_shaders.insert(ShaderTemplateMap::value_type(shaderTemplate->getName(), shaderTemplate));
	
	return shaderTemplate;
} 


/* Parses through a table definition within a material file.
 * It just skips over the whole content 
 */
void parseShaderTable(parser::DefTokeniser& tokeniser) 
{
	// This is the name of the table
	tokeniser.nextToken();
	
	tokeniser.assertNextToken("{");
	unsigned short int openBraces = 1; 
	
	// Continue parsing till the table is over, i.e. the according closing brace is found
	while (openBraces>0 && tokeniser.hasMoreTokens()) {
		const std::string token = tokeniser.nextToken();
		
		if (token == "{") {
			openBraces++;
		}
		
		if (token == "}") {
			openBraces--;
		}
	}	
}

/* Parses the contents of a material definition's stage, maybe called recursively
 */
void parseShaderStage(parser::DefTokeniser& tokeniser, 
					  ShaderTemplatePointer& shaderTemplate, 
					  const char* filename) 
{
	// Call the shader parser	
	bool result = shaderTemplate->parseDoom3(tokeniser);
	
	if (result) {
		// do we already have this shader?
        if (!g_shaderDefinitions.insert(ShaderDefinitionMap::value_type(shaderTemplate->getName(),
            	ShaderDefinition(shaderTemplate.get(), ShaderArguments(), filename))).second) {
        	throw parser::ParseException(std::string("shader ") + shaderTemplate->getName() + " is already in memory");
        }
    }
    else {
    	throw parser::ParseException(std::string("Error while parsing") + shaderTemplate->getName());
    }
}

/* Parses through the shader file and processes the tokens delivered by DefTokeniser. 
 */ 
void parseShaderFile(const std::string& inStr, const char* filename)
{
	g_shaderFilenames.push_back(filename);
	filename = g_shaderFilenames.back().c_str();
	
	parser::DefTokeniser tokeniser(inStr, " \t\n\v\r", "{}(),");	
	
	while (tokeniser.hasMoreTokens()) {
		// Load the first token, it should be a name
		std::string token = tokeniser.nextToken();		
		
		if (token == "table") {
			parseShaderTable(tokeniser);		 		
		} 
		else if (token[0] == '{' || token[0] == '}') {
			// This is not supposed to happen, as the shaderName is still undefined
			throw parser::ParseException("Missing shadername."); 
		} 
		else {			
			// We are still outside of any braces, so this must be a shader name			
			ShaderTemplatePointer shaderTemplate = parseShaderName(token);
			tokeniser.assertNextToken("{");
			parseShaderStage(tokeniser, shaderTemplate, filename);
		}
	}
}

/* Loads a given material file and parses its contents 
 */
void loadShaderFile(const char* filename)
{
  ArchiveTextFile* file = GlobalFileSystem().openTextFile(filename);

  if(file != 0) {
    globalOutputStream() << "Parsing shaderfile " << filename << "\n";        
    parseShaderFile(file->getString(), filename);           
    file->release();
  } 
  else {
  	throw parser::ParseException(std::string("Unable to read shaderfile: ") + filename);  	
  }
}

/** Load the shaders from the MTR files.
 */
void Shaders_Load()
{
	// Get the shaders path and extension from the XML game file

	xml::NodeList nlShaderPath = GlobalRadiant().registry().findXPath("game/filesystem/shaders/basepath");
	if (nlShaderPath.size() != 1)
		throw shaders::MissingXMLNodeException("Failed to find \"/game/filesystem/shaders/basepath\" node in game descriptor");

	xml::NodeList nlShaderExt = GlobalRadiant().registry().findXPath("game/filesystem/shaders/extension");
	if (nlShaderExt.size() != 1)
		throw shaders::MissingXMLNodeException("Failed to find \"/game/filesystem/shaders/extension\" node in game descriptor");

	// Load the shader files from the VFS

	std::string sPath = nlShaderPath[0].getContent();
	if (!boost::algorithm::ends_with(sPath, "/"))
		sPath += "/";
		
	g_shadersExtension = nlShaderExt[0].getContent();
	
	GlobalFileSystem().forEachFile(sPath.c_str(), g_shadersExtension.c_str(), AddShaderFileCaller(), 0);

    GSList *lst = l_shaderfiles;
    StringOutputStream shadername(256);
    globalOutputStream() << "Begin: parsing shader files... \n";
    while(lst) {
		shadername << sPath.c_str() << reinterpret_cast<const char*>(lst->data);
		try {
			loadShaderFile(shadername.c_str());
		}
        catch (parser::ParseException e) {
        	globalOutputStream() << "Warning: in shaderfile: " << shadername.c_str() << ": " << e.what() << "\n";
        }        
      shadername.clear();
      lst = lst->next;
    }
	globalOutputStream() << "End: parsing shader files. \n";
}

void Shaders_Free()
{
  FreeShaders();
  FreeShaderList();
  g_shaderFilenames.clear();
}

ModuleObservers g_observers;

std::size_t g_shaders_unrealised = 1; // wait until filesystem and is realised before loading anything
bool Shaders_realised()
{
  return g_shaders_unrealised == 0;
}
void Shaders_Realise()
{
  if(--g_shaders_unrealised == 0)
  {
    Shaders_Load();
    g_observers.realise();
  }
}
void Shaders_Unrealise()
{
  if(++g_shaders_unrealised == 1)
  {
    g_observers.unrealise();
    Shaders_Free();
  }
}

void Shaders_Refresh() 
{
  Shaders_Unrealise();
  Shaders_Realise();
}

class Quake3ShaderSystem : public ShaderSystem, public ModuleObserver
{
public:
  void realise()
  {
    Shaders_Realise();
  }
  void unrealise()
  {
    Shaders_Unrealise();
  }
  void refresh()
  {
    Shaders_Refresh();
  }

	// Is the shader system realised
	bool isRealised() {
		return g_shaders_unrealised == 0;
	}

	// Return a shader by name
	IShader* getShaderForName(const std::string& name) {
		IShader *pShader = Try_Shader_ForName(name.c_str());
		pShader->IncRef();
		return pShader;
	}

  void foreachShaderName(const ShaderNameCallback& callback)
  {
    for(ShaderDefinitionMap::const_iterator i = g_shaderDefinitions.begin(); i != g_shaderDefinitions.end(); ++i)
    {
      callback((*i).first.c_str());
    }
  }

  void beginActiveShadersIterator()
  {
    ActiveShaders_IteratorBegin();
  }
  bool endActiveShadersIterator()
  {
    return ActiveShaders_IteratorAtEnd();
  }
  IShader* dereferenceActiveShadersIterator()
  {
    return ActiveShaders_IteratorCurrent();
  }
  void incrementActiveShadersIterator()
  {
    ActiveShaders_IteratorIncrement();
  }
  void setActiveShadersChangedNotify(const Callback& notify)
  {
    g_ActiveShadersChangedNotify = notify;
  }

  void attach(ModuleObserver& observer)
  {
    g_observers.attach(observer);
  }
  void detach(ModuleObserver& observer)
  {
    g_observers.detach(observer);
  }

  void setLightingEnabled(bool enabled)
  {
    if(CShader::m_lightingEnabled != enabled)
    {
      for(shaders_t::const_iterator i = g_ActiveShaders.begin(); i != g_ActiveShaders.end(); ++i)
      {
        (*i).second->unrealiseLighting();
      }
      CShader::m_lightingEnabled = enabled;
      for(shaders_t::const_iterator i = g_ActiveShaders.begin(); i != g_ActiveShaders.end(); ++i)
      {
        (*i).second->realiseLighting();
      }
    }
  }

  const char* getTexturePrefix() const
  {
    return g_texturePrefix;
  }
};

Quake3ShaderSystem g_Quake3ShaderSystem;

ShaderSystem& GetShaderSystem()
{
  return g_Quake3ShaderSystem;
}

void Shaders_Construct()
{
  GlobalFileSystem().attach(g_Quake3ShaderSystem);
}
void Shaders_Destroy()
{
  GlobalFileSystem().detach(g_Quake3ShaderSystem);

  if(Shaders_realised())
  {
    Shaders_Free();
  }
}
