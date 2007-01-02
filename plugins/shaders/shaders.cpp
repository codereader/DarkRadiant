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

#include <map>

#include "ifilesystem.h"
#include "ishaders.h"
#include "itextures.h"
#include "qerplugin.h"
#include "irender.h"
#include "iregistry.h"

#include "debugging/debugging.h"
#include "math/FloatTools.h"
#include "generic/callback.h"
#include "generic/referencecounted.h"
#include "os/path.h"
#include "shaderlib.h"
#include "texturelib.h"
#include "moduleobservers.h"
#include "archivelib.h"
#include "imagelib.h"

#include "xmlutil/Node.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>

/* GLOBALS */

namespace {

	const char* MISSING_BASEPATH_NODE =
	"Failed to find \"/game/filesystem/shaders/basepath\" node \
in game descriptor";
	 
	const char* MISSING_EXTENSION_NODE =
	"Failed to find \"/game/filesystem/shaders/extension\" node \
in game descriptor";
	
}

_QERPlugImageTable* g_bitmapModule = 0;
const char* g_texturePrefix = "textures/";

void ActiveShaders_IteratorBegin();
bool ActiveShaders_IteratorAtEnd();
IShader *ActiveShaders_IteratorCurrent();
void ActiveShaders_IteratorIncrement();
Callback g_ActiveShadersChangedNotify;

void FreeShaders();
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

typedef boost::shared_ptr<ShaderTemplate> ShaderTemplatePtr;
typedef std::map<std::string, ShaderTemplatePtr> ShaderTemplateMap;

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

/**
 * Wrapper class that associates a ShaderTemplate with its filename.
 */
struct ShaderDefinition
{
	// The shader template
	ShaderTemplatePtr shaderTemplate;
	
	// Filename from which the shader was parsed
	std::string filename;

	/* Constructor
	 */
	ShaderDefinition(ShaderTemplatePtr templ, const std::string& fname)
    : shaderTemplate(templ),
      filename(fname)
	{ }
	
};

typedef std::map<std::string, ShaderDefinition> ShaderDefinitionMap;

ShaderDefinitionMap g_shaderDefinitions;

///\todo BlendFunc parsing
BlendFunc evaluateBlendFunc(const BlendFuncExpression& blendFunc)
{
  return BlendFunc(BLEND_ONE, BLEND_ZERO);
}

// Map string blend functions to their enum equivalents
BlendFactor evaluateBlendFactor(const std::string& value)
{
  if(value == "gl_zero")
  {
    return BLEND_ZERO;
  }
  if(value == "gl_one")
  {
    return BLEND_ONE;
  }
  if(value == "gl_src_color")
  {
    return BLEND_SRC_COLOUR;
  }
  if(value == "gl_one_minus_src_color")
  {
    return BLEND_ONE_MINUS_SRC_COLOUR;
  }
  if(value == "gl_src_alpha")
  {
    return BLEND_SRC_ALPHA;
  }
  if(value == "gl_one_minus_src_alpha")
  {
    return BLEND_ONE_MINUS_SRC_ALPHA;
  }
  if(value == "gl_dst_color")
  {
    return BLEND_DST_COLOUR;
  }
  if(value == "gl_one_minus_dst_color")
  {
    return BLEND_ONE_MINUS_DST_COLOUR;
  }
  if(value == "gl_dst_alpha")
  {
    return BLEND_DST_ALPHA;
  }
  if(value == "gl_one_minus_dst_alpha")
  {
    return BLEND_ONE_MINUS_DST_ALPHA;
  }
  if(value == "gl_src_alpha_saturate")
  {
    return BLEND_SRC_ALPHA_SATURATE;
  }

  return BLEND_ZERO;
}

class CShader : public IShader
{
  std::size_t m_refcount;

  const ShaderTemplate& m_template;
  std::string m_filename;
  // name is shader-name, otherwise texture-name (if not a real shader)
  std::string m_Name;

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

	/*
	 * Constructor.
	 */
	CShader(const ShaderDefinition& definition)
	: m_refcount(0),
	  m_template(*definition.shaderTemplate),
      m_filename(definition.filename),
      m_blendFunc(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA),
      m_bInUse(false) 
	{
		assert(definition.shaderTemplate != NULL); // otherwise we have NULL ref
		
		// Initialise texture pointers
	    m_pTexture = 0;
	    m_pDiffuse = 0;
	    m_pBump = 0;
	    m_pSpecular = 0;
	
	    m_notfound = 0;

		// Realise the shader
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
    return m_filename.empty();
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
    return m_filename.c_str();
  }
  // -----------------------------------------

  void realise()
  {
    m_pTexture = GlobalTexturesCache().capture(m_template.m_textureName);

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
        m_heightmapScale = 
        	boost::lexical_cast<float>(m_template.m_heightmapScale);
        loader = LoadImageCallback(&m_heightmapScale, loadHeightmap);
      }

		// Set up the diffuse, bump and specular stages. Bump and specular will be set to defaults
		// _flat and _black respectively, if an image map is not specified in the material.

		m_pDiffuse = GlobalTexturesCache().capture(m_template.m_diffuse);

    	std::string flatName = std::string(GlobalRadiant().getAppPath()) + "bitmaps/_flat.bmp";
		m_pBump = GlobalTexturesCache().capture(m_template.m_bump);
		if (m_pBump == 0 || m_pBump->texture_number == 0) {
			GlobalTexturesCache().release(m_pBump); // release old object first
			m_pBump = GlobalTexturesCache().capture(LoadImageCallback(0, loadBitmap), flatName.c_str());
		}
		
		std::string blackName = std::string(GlobalRadiant().getAppPath()) + "bitmaps/_black.bmp";
		m_pSpecular = GlobalTexturesCache().capture(m_template.m_specular);
		if (m_pSpecular == 0 || m_pSpecular->texture_number == 0) {
			GlobalTexturesCache().release(m_pSpecular);
			m_pSpecular = GlobalTexturesCache().capture(LoadImageCallback(0, loadBitmap), blackName.c_str());
		}
		
		// Get light falloff image
		
		m_pLightFalloffImage = 
			GlobalTexturesCache().capture(m_template.m_lightFalloffImage);

		for(ShaderTemplate::Layers::const_iterator i = m_template.m_layers.begin(); 
			i != m_template.m_layers.end(); 
			++i)
		{
        	m_layers.push_back(evaluateLayer(*i));
		}

      if(m_layers.size() == 1)
      {
        const BlendFuncExpression& blendFunc = 
        	m_template.m_layers.front().m_blendFunc;
        
		// If explicit blend function (2-components), evaluate it, otherwise
		// use a standard one
        if(!blendFunc.second.empty()) {
			m_blendFunc = BlendFunc(evaluateBlendFactor(blendFunc.first),
									evaluateBlendFactor(blendFunc.second));
        }
        else {
			if(blendFunc.first == "add") {
				m_blendFunc = BlendFunc(BLEND_ONE, BLEND_ONE);
			}
			else if(blendFunc.first == "filter") {
				m_blendFunc = BlendFunc(BLEND_DST_COLOUR, BLEND_ZERO);
			}
			else if(blendFunc.first == "blend") {
				m_blendFunc = BlendFunc(BLEND_SRC_ALPHA, 
										BLEND_ONE_MINUS_SRC_ALPHA);
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
  void setName(const std::string& name)
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

	static MapLayer evaluateLayer(const LayerTemplate& layerTemplate) 
	{
    	return MapLayer(
      		GlobalTexturesCache().capture(layerTemplate.m_texture),
      		evaluateBlendFunc(layerTemplate.m_blendFunc),
      		layerTemplate.m_clampToBorder,
      		boost::lexical_cast<float>(layerTemplate.m_alphaTest)
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

#ifdef _DEBUG

// IShader stream insertion
std::ostream& operator<< (std::ostream& st, const IShader& sh) {
	st << "IShader: { name=" << sh.getName() << " "
	   << "filename=" << sh.getShaderFileName() << " "
	   << "}";
	return st;
}

#endif

bool CShader::m_lightingEnabled = false;

typedef SmartPointer<CShader> ShaderPointer;
typedef std::map<std::string, ShaderPointer> shaders_t;

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

/**
 * Lookup a named shader and return its CShader object.
 */
CShader* Try_Shader_ForName(const std::string& name)
{
  {
    shaders_t::iterator i = g_ActiveShaders.find(name);
    if(i != g_ActiveShaders.end())
    {
      return (*i).second;
    }
  }

	// Search for a matching ShaderDefinition. If none is found, create a 
	// default one and return this instead (this is how unrecognised textures
	// get rendered with notex.bmp).
	ShaderDefinitionMap::iterator i = g_shaderDefinitions.find(name);
	if(i == g_shaderDefinitions.end()) {
		ShaderTemplatePtr shaderTemplate(new ShaderTemplate(name));
		g_shaderTemplates[name] = shaderTemplate;

		// Create and insert new ShaderDefinition wrapper
		ShaderDefinition def(shaderTemplate, "");
		i = g_shaderDefinitions.insert(
							ShaderDefinitionMap::value_type(name, def)).first;
	}

	ShaderPointer pShader(new CShader(i->second));
	pShader->setName(name);
	g_ActiveShaders.insert(shaders_t::value_type(name, pShader));
	g_ActiveShadersChangedNotify();
	return pShader;
}

/* Normalises a given (raw) shadername (slash conversion, extension removal, ...) 
 * and inserts a new shader 
 */
ShaderTemplatePtr parseShaderName(std::string& rawName) 
{
	boost::algorithm::replace_all(rawName, "\\", "/"); // use forward slashes
	boost::algorithm::to_lower(rawName); // use lowercase

	// Remove the extension if present
	size_t dotPos = rawName.rfind(".");
	if (dotPos != std::string::npos) {
		rawName = rawName.substr(0, dotPos);
	}
	
	ShaderTemplatePtr shaderTemplate(new ShaderTemplate(rawName));
	g_shaders[rawName] = shaderTemplate;

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

/**
 * Parses the contents of a material definition. The shader name and opening
 * brace "{" will already have been removed when this function is called.
 * 
 * @param tokeniser
 * DefTokeniser to retrieve tokens from.
 * 
 * @param shaderTemplate
 * An empty ShaderTemplate which will parse the token stream and populate
 * itself.
 * 
 * @param filename
 * The name of the shader file we are parsing.
 */
void parseShaderDecl(parser::DefTokeniser& tokeniser, 
					  ShaderTemplatePtr shaderTemplate, 
					  const std::string& filename) 
{
	// Get the ShaderTemplate to populate itself by parsing tokens from the
	// DefTokeniser. This may throw exceptions.	
	shaderTemplate->parseDoom3(tokeniser);
	
	// Construct the ShaderDefinition wrapper class
	ShaderDefinition def(shaderTemplate, filename);
	
	// Get the parsed shader name
	std::string name = shaderTemplate->getName();
	
	// Insert into the definitions map, if not already present
    if (!g_shaderDefinitions.insert(
    				ShaderDefinitionMap::value_type(name, def)).second) 
	{
    	std::cout << "[shaders] " << filename << ": shader " << name
    			  << " already defined." << std::endl;
    }
}

/* Parses through the shader file and processes the tokens delivered by 
 * DefTokeniser. 
 */ 
void parseShaderFile(const std::string& inStr, const std::string& filename)
{
	// Create the tokeniser
	parser::DefTokeniser tokeniser(inStr, " \t\n\v\r", "{}(),");
	
	while (tokeniser.hasMoreTokens()) {
		// Load the first token, it should be a name
		std::string token = tokeniser.nextToken();		

		if (token == "table") {
			parseShaderTable(tokeniser);		 		
		} 
		else if (token[0] == '{' || token[0] == '}') {
			// This is not supposed to happen, as the shaderName is still 
			// undefined
			std::cerr << "[shaders] Missing shadername in " 
					  << filename << std::endl;
			return; 
		} 
		else {			
			// We are still outside of any braces, so this must be a shader name			
			ShaderTemplatePtr shaderTemplate = parseShaderName(token);

			// Try to parse the rest of the decl, catching and printing any
			// exception
			try {
				tokeniser.assertNextToken("{");
				parseShaderDecl(tokeniser, shaderTemplate, filename);
			}
			catch (parser::ParseException e) {
				std::cerr << "[shaders] " << filename << ": Failed to parse \"" 
						  << shaderTemplate->getName() << "\" (" << e.what() 
						  << ")" << std::endl;
				return;
			}
		}
	}
}

#include "ShaderFileLoader.h"

/** Load the shaders from the MTR files.
 */
void Shaders_Load()
{
	// Get the shaders path and extension from the XML game file
	xml::NodeList nlShaderPath = 
		GlobalRegistry().findXPath("game/filesystem/shaders/basepath");
	if (nlShaderPath.size() != 1)
		throw shaders::MissingXMLNodeException(MISSING_BASEPATH_NODE);

	xml::NodeList nlShaderExt = 
		GlobalRegistry().findXPath("game/filesystem/shaders/extension");
	if (nlShaderExt.size() != 1)
		throw shaders::MissingXMLNodeException(MISSING_EXTENSION_NODE);

	// Load the shader files from the VFS
	std::string sPath = nlShaderPath[0].getContent();
	if (!boost::algorithm::ends_with(sPath, "/"))
		sPath += "/";
		
	std::string extension = nlShaderExt[0].getContent();
	
	// Load each file from the global filesystem
	shaders::ShaderFileLoader ldr(sPath);
	GlobalFileSystem().forEachFile(sPath.c_str(), 
								   extension.c_str(), 
								   makeCallback1(ldr), 
								   0);
}

void Shaders_Free()
{
	FreeShaders();
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
