/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "renderstate.h"
#include "render/backend/OpenGLStateBucket.h"
#include "render/backend/OpenGLStateBucketAdd.h"
#include "render/backend/OpenGLShader.h"
#include "render/backend/OpenGLStateMap.h"
#include "render/backend/GLProgramFactory.h"

#include "debugging/debugging.h"
#include "warnings.h"

#include "ishaders.h"
#include "irender.h"
#include "itextures.h"
#include "igl.h"
#include "iglrender.h"
#include "renderable.h"
#include "qerplugin.h"
#include "iregistry.h"

#include <set>
#include <vector>
#include <list>
#include <map>
#include <sstream>

#include "math/matrix.h"
#include "math/aabb.h"
#include "generic/callback.h"
#include "texturelib.h"
#include "string/string.h"
#include "container/hashfunc.h"
#include "container/cache.h"
#include "generic/reference.h"
#include "moduleobservers.h"
#include "stream/filestream.h"
#include "stream/stringstream.h"
#include "os/file.h"
#include "preferences.h"
#include "plugin.h"

#include <boost/algorithm/string/case_conv.hpp>

/* GLOBALS */

namespace {
	
	// Name of default pointlight shader, retrieved from game descriptor.
	std::string _defaultLightShaderName;
	
}

/* 
 * Map of OpenGLState references, with access functions. TODO: make this part
 * of OpenGLShaderCache, after the class is refactored into files.
 */
OpenGLStates g_state_sorted;

void insertSortedState(const OpenGLStates::value_type& val) {
	g_state_sorted.insert(val);	
}

void eraseSortedState(const OpenGLStates::key_type& key) {
	g_state_sorted.erase(key);
}

#define DEBUG_RENDER 0

inline void debug_string(const char* string)
{
#if(DEBUG_RENDER)
  globalOutputStream() << string << "\n";
#endif
}

inline void debug_int(const char* comment, int i)
{
#if(DEBUG_RENDER)
  globalOutputStream() << comment << " " << i << "\n";
#endif
}

inline void debug_colour(const char* comment)
{
#if(DEBUG_RENDER)
  Vector4 v;
  glGetFloatv(GL_CURRENT_COLOR, reinterpret_cast<float*>(&v));
  globalOutputStream() << comment << " colour: "
    << v[0] << " "
    << v[1] << " "
    << v[2] << " "
    << v[3];
  if(glIsEnabled(GL_COLOR_ARRAY))
  {
    globalOutputStream() << " ARRAY";
  }
  if(glIsEnabled(GL_COLOR_MATERIAL))
  {
    globalOutputStream() << " MATERIAL";
  }
  globalOutputStream() << "\n";
#endif
}

#include "timer.h"

StringOutputStream g_renderer_stats;
std::size_t g_count_prims;
std::size_t g_count_states;
std::size_t g_count_transforms;
Timer g_timer;

inline void count_prim()
{
  ++g_count_prims;
}

inline void count_state()
{
  ++g_count_states;
}

inline void count_transform()
{
  ++g_count_transforms;
}

void Renderer_ResetStats()
{
  g_count_prims = 0;
  g_count_states = 0;
  g_count_transforms = 0;
  g_timer.start();
}

const char* Renderer_GetStats()
{
  g_renderer_stats.clear();
  g_renderer_stats << "prims: " << Unsigned(g_count_prims)
    << " | states: " << Unsigned(g_count_states)
    << " | transforms: " << Unsigned(g_count_transforms)
    << " | msec: " << g_timer.elapsed_msec();
  return g_renderer_stats.c_str();
}


void printShaderLog(GLhandleARB object)
{
  GLint log_length = 0;
  glGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &log_length);
  
  Array<char> log(log_length);
  glGetInfoLogARB(object, log_length, &log_length, log.data());
  
  globalErrorStream() << StringRange(log.begin(), log.begin() + log_length) << "\n";
}

void createShader(GLhandleARB program, const char* filename, GLenum type)
{
  GLhandleARB shader = glCreateShaderObjectARB(type);
  GlobalOpenGL_debugAssertNoErrors();

  // load shader
  {
    std::size_t size = file_size(filename);
    FileInputStream file(filename);
    ASSERT_MESSAGE(!file.failed(), "failed to open " << makeQuoted(filename));
    Array<GLcharARB> buffer(size);
    size = file.read(reinterpret_cast<StreamBase::byte_type*>(buffer.data()), size);

    const GLcharARB* string = buffer.data();
    GLint length = GLint(size);
    glShaderSourceARB(shader, 1, &string, &length);
  }

  // compile shader
  {
    glCompileShaderARB(shader);
    
    GLint compiled = 0;
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);

    if(!compiled)
    {
      printShaderLog(shader);
    }

    ASSERT_MESSAGE(compiled, "shader compile failed: " << makeQuoted(filename));
  }
  
  // attach shader
  glAttachObjectARB(program, shader);
  
  glDeleteObjectARB(shader);

  GlobalOpenGL_debugAssertNoErrors();
}

// Create an ARB GL Program by calling glProgramStringARB with the contents of
// a file.
void createARBProgram(const char* filename, GLenum type)
{
  std::size_t size = file_size(filename);
  FileInputStream file(filename);
  ASSERT_MESSAGE(!file.failed(), "failed to open " << makeQuoted(filename));
  Array<GLcharARB> buffer(size);
  size = file.read(reinterpret_cast<StreamBase::byte_type*>(buffer.data()), size);

  glProgramStringARB(type, GL_PROGRAM_FORMAT_ASCII_ARB, GLsizei(size), buffer.data());

  if(GL_INVALID_OPERATION == glGetError())
  {
    GLint errPos;
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
    const GLubyte* errString = glGetString(GL_PROGRAM_ERROR_STRING_ARB);

    globalErrorStream() << reinterpret_cast<const char*>(filename) << ":" <<  errPos << "\n" << reinterpret_cast<const char*>(errString);

    ERROR_MESSAGE("error in gl program");
  }
}


bool g_vertexArray_enabled = false;
bool g_normalArray_enabled = false;
bool g_texcoordArray_enabled = false;
bool g_colorArray_enabled = false;

void OpenGLState_constructDefault(OpenGLState& state)
{
  state.m_state = RENDER_DEFAULT;

  state.m_texture = 0;
  state.m_texture1 = 0;
  state.m_texture2 = 0;
  state.m_texture3 = 0;
  state.m_texture4 = 0;
  state.m_texture5 = 0;
  state.m_texture6 = 0;
  state.m_texture7 = 0;

  state.m_colour[0] = 1;
  state.m_colour[1] = 1;
  state.m_colour[2] = 1;
  state.m_colour[3] = 1;

  state.m_depthfunc = GL_LESS;

  state.m_blend_src = GL_SRC_ALPHA;
  state.m_blend_dst = GL_ONE_MINUS_SRC_ALPHA;

  state.m_alphafunc = GL_ALWAYS;
  state.m_alpharef = 0;

  state.m_linewidth = 1;
  state.m_pointsize = 1;

  state.m_linestipple_factor = 1;
  state.m_linestipple_pattern = 0xaaaa;

  state.m_fog = OpenGLFogState();
}





#define LIGHT_SHADER_DEBUG 0

#if LIGHT_SHADER_DEBUG
typedef std::vector<Shader*> LightDebugShaders;
LightDebugShaders g_lightDebugShaders;
#endif

class CountLights
{
  std::size_t m_count;
public:
  typedef RendererLight& first_argument_type;

  CountLights() : m_count(0)
  {
  }
  void operator()(const RendererLight& light)
  {
    ++m_count;
  }
  std::size_t count() const
  {
    return m_count;
  }
};



inline bool lightEnabled(const RendererLight& light, const LightCullable& cullable)
{
  return cullable.testLight(light);
}

typedef std::set<RendererLight*> RendererLights;

#define DEBUG_LIGHT_SYNC 0

class LinearLightList : public LightList
{
  LightCullable& m_cullable;
  RendererLights& m_allLights;
  Callback m_evaluateChanged;

  typedef std::list<RendererLight*> Lights;
  mutable Lights m_lights;
  mutable bool m_lightsChanged;
public:
  LinearLightList(LightCullable& cullable, RendererLights& lights, const Callback& evaluateChanged) :
    m_cullable(cullable), m_allLights(lights), m_evaluateChanged(evaluateChanged)
  {
    m_lightsChanged = true;
  }
  void evaluateLights() const
  {
    m_evaluateChanged();
    if(m_lightsChanged)
    {
      m_lightsChanged = false;

      m_lights.clear();
      m_cullable.clearLights();
      for(RendererLights::const_iterator i = m_allLights.begin(); i != m_allLights.end(); ++i)
      {
        if(lightEnabled(*(*i), m_cullable))
        {
          m_lights.push_back(*i);
          m_cullable.insertLight(*(*i));
        }
      }
    }
#if(DEBUG_LIGHT_SYNC)
    else
    {
      Lights lights;
      for(RendererLights::const_iterator i = m_allLights.begin(); i != m_allLights.end(); ++i)
      {
        if(lightEnabled(*(*i), m_cullable))
        {
          lights.push_back(*i);
        }
      }
      ASSERT_MESSAGE(
        !std::lexicographical_compare(lights.begin(), lights.end(), m_lights.begin(), m_lights.end())
        && !std::lexicographical_compare(m_lights.begin(), m_lights.end(), lights.begin(), lights.end()),
        "lights out of sync"
      );
    }
#endif
  }
  void forEachLight(const RendererLightCallback& callback) const
  {
    evaluateLights();

    for(Lights::const_iterator i = m_lights.begin(); i != m_lights.end(); ++i)
    {
      callback(*(*i));
    }
  }
  void lightsChanged() const
  {
    m_lightsChanged = true;
  }
};

inline void setFogState(const OpenGLFogState& state)
{
	glFogi(GL_FOG_MODE, state.mode);
	glFogf(GL_FOG_DENSITY, state.density);
	glFogf(GL_FOG_START, state.start);
	glFogf(GL_FOG_END, state.end);
	glFogi(GL_FOG_INDEX, state.index);
	glFogfv(GL_FOG_COLOR, state.colour);
}

#define DEBUG_SHADERS 0

class OpenGLShaderCache : public ShaderCache, public TexturesCacheObserver, public ModuleObserver
{
  class CreateOpenGLShader
  {
    OpenGLShaderCache* m_cache;
  public:
    explicit CreateOpenGLShader(OpenGLShaderCache* cache = 0)
      : m_cache(cache)
    {
    }
    OpenGLShader* construct(const CopiedString& name)
    {
      OpenGLShader* shader = new OpenGLShader;
      if(m_cache->realised())
      {
        shader->realise(name);
      }
      return shader;
    }
    void destroy(OpenGLShader* shader)
    {
      if(m_cache->realised())
      {
        shader->unrealise();
      }
      delete shader;
    }
  };

  typedef HashedCache<CopiedString, OpenGLShader, HashString, std::equal_to<CopiedString>, CreateOpenGLShader> Shaders;
  Shaders m_shaders;
  std::size_t m_unrealised;

  bool m_lightingEnabled;
  bool m_lightingSupported;
  bool m_useShaderLanguage;

public:
  OpenGLShaderCache()
    : m_shaders(CreateOpenGLShader(this)),
    m_unrealised(3), // wait until shaders, gl-context and textures are realised before creating any render-states
    m_lightingEnabled(true),
    m_lightingSupported(false),
    m_useShaderLanguage(false),
    m_lightsChanged(true),
    m_traverseRenderablesMutex(false)
  {
  }
  ~OpenGLShaderCache()
  {
    for(Shaders::iterator i = m_shaders.begin(); i != m_shaders.end(); ++i)
    {
      globalOutputStream() << "leaked shader: " << makeQuoted((*i).key.c_str()) << "\n";
    }
  }

	/* Capture the given shader.
	 */
	Shader* capture(const std::string& name) {
		return m_shaders.capture(name.c_str()).get();	
	}

	/* Release the given shader.
	 */
	void release(const std::string& name) {
		m_shaders.release(name.c_str());
	}
  
  void render(RenderStateFlags globalstate, const Matrix4& modelview, const Matrix4& projection, const Vector3& viewer)
  {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(reinterpret_cast<const float*>(&projection));
  #if 0
    //qglGetFloatv(GL_PROJECTION_MATRIX, reinterpret_cast<float*>(&projection));
  #endif

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(reinterpret_cast<const float*>(&modelview));
  #if 0
    //qglGetFloatv(GL_MODELVIEW_MATRIX, reinterpret_cast<float*>(&modelview));
  #endif
 
    ASSERT_MESSAGE(realised(), "render states are not realised");

    // global settings that are not set in renderstates
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glPolygonOffset(-1, 1);
    {
      const GLubyte pattern[132] = {
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
 	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
 	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
 	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
 	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
	      0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55
     };
      glPolygonStipple(pattern);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    g_vertexArray_enabled = true;
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    if(GLEW_VERSION_1_3)
    {
      glActiveTexture(GL_TEXTURE0);
      glClientActiveTexture(GL_TEXTURE0);
    }

    if(GLEW_ARB_shader_objects)
    {
      glUseProgramObjectARB(0);
      glDisableVertexAttribArrayARB(c_attr_TexCoord0);
      glDisableVertexAttribArrayARB(c_attr_Tangent);
      glDisableVertexAttribArrayARB(c_attr_Binormal);
    }

    if(globalstate & RENDER_TEXTURE)
    {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    OpenGLState current;
    OpenGLState_constructDefault(current);
    current.m_sort = OpenGLState::eSortFirst;

    // default renderstate settings
    glLineStipple(current.m_linestipple_factor, current.m_linestipple_pattern);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    g_texcoordArray_enabled = false;
    glDisableClientState(GL_COLOR_ARRAY);
    g_colorArray_enabled = false;
    glDisableClientState(GL_NORMAL_ARRAY);
    g_normalArray_enabled = false;
    glDisable(GL_BLEND);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_POLYGON_STIPPLE);
    glDisable(GL_POLYGON_OFFSET_LINE);

    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(1,1,1,1);
    glDepthFunc(GL_LESS);
    glAlphaFunc(GL_ALWAYS, 0);
    glLineWidth(1);
    glPointSize(1);

	  glHint(GL_FOG_HINT, GL_NICEST);
    glDisable(GL_FOG);
    setFogState(OpenGLFogState());

    GlobalOpenGL_debugAssertNoErrors();

    debug_string("begin rendering");
    for(OpenGLStates::iterator i = g_state_sorted.begin(); i != g_state_sorted.end(); ++i)
    {
      (*i).second->render(current, globalstate, viewer);
    }
    debug_string("end rendering");
  }
  void realise()
  {
    if(--m_unrealised == 0)
    {
      if(lightingSupported() && lightingEnabled())
      {
		// Realise the GLPrograms
		render::GLProgramFactory::realise();
      }

      for(Shaders::iterator i = m_shaders.begin(); i != m_shaders.end(); ++i)
      {
        if(!(*i).value.empty())
        {
          (*i).value->realise(i->key);
        }
      }
    }
  }
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
      for(Shaders::iterator i = m_shaders.begin(); i != m_shaders.end(); ++i)
      {
        if(!(*i).value.empty())
        {
          (*i).value->unrealise();
        }
      }
      if(GlobalOpenGL().contextValid && lightingSupported() && lightingEnabled())
      {
		// Unrealise the GLPrograms
		render::GLProgramFactory::unrealise();
      }
    }
  }
  bool realised()
  {
    return m_unrealised == 0;
  }


  bool lightingEnabled() const
  {
    return m_lightingEnabled;
  }
  bool lightingSupported() const
  {
    return m_lightingSupported;
  }
  bool useShaderLanguage() const
  {
    return m_useShaderLanguage;
  }
  void setLighting(bool supported, bool enabled)
  {
    bool refresh = (m_lightingSupported && m_lightingEnabled) != (supported && enabled);

    if(refresh)
    {
      unrealise();
      GlobalShaderSystem().setLightingEnabled(supported && enabled);
    }

    m_lightingSupported = supported;
    m_lightingEnabled = enabled;

    if(refresh)
    {
      realise();
    }
  }
  void extensionsInitialised()
  {
    setLighting(GLEW_VERSION_1_3
      && GLEW_ARB_vertex_program
      && GLEW_ARB_fragment_program
      && GLEW_ARB_shader_objects
      && GLEW_ARB_vertex_shader
      && GLEW_ARB_fragment_shader
      && GLEW_ARB_shading_language_100,
      m_lightingEnabled
    );

    if(!lightingSupported())
    {
      globalOutputStream() << "Lighting mode requires OpenGL features not supported by your graphics drivers:\n";
      if(!GLEW_VERSION_1_3)
      {
        globalOutputStream() << "  GL version 1.3 or better\n";
      }
      if(!GLEW_ARB_vertex_program)
      {
        globalOutputStream() << "  GL_ARB_vertex_program\n";
      }
      if(!GLEW_ARB_fragment_program)
      {
        globalOutputStream() << "  GL_ARB_fragment_program\n";
      }
      if(!GLEW_ARB_shader_objects)
      {
        globalOutputStream() << "  GL_ARB_shader_objects\n";
      }
      if(!GLEW_ARB_vertex_shader)
      {
        globalOutputStream() << "  GL_ARB_vertex_shader\n";
      }
      if(!GLEW_ARB_fragment_shader)
      {
        globalOutputStream() << "  GL_ARB_fragment_shader\n";
      }
      if(!GLEW_ARB_shading_language_100)
      {
        globalOutputStream() << "  GL_ARB_shading_language_100\n";
      }
    }
  }
  void setLightingEnabled(bool enabled)
  {
    setLighting(m_lightingSupported, enabled);
  }

  // light culling

  RendererLights m_lights;
  bool m_lightsChanged;
  typedef std::map<LightCullable*, LinearLightList> LightLists;
  LightLists m_lightLists;

  const LightList& attach(LightCullable& cullable)
  {
    return (*m_lightLists.insert(LightLists::value_type(&cullable, LinearLightList(cullable, m_lights, EvaluateChangedCaller(*this)))).first).second;
  }
  void detach(LightCullable& cullable)
  {
    m_lightLists.erase(&cullable);
  }
  void changed(LightCullable& cullable)
  {
    LightLists::iterator i = m_lightLists.find(&cullable);
    ASSERT_MESSAGE(i != m_lightLists.end(), "cullable not attached");
    (*i).second.lightsChanged();
  }
  void attach(RendererLight& light)
  {
    ASSERT_MESSAGE(m_lights.find(&light) == m_lights.end(), "light could not be attached");
    m_lights.insert(&light);
    changed(light);
  }
  void detach(RendererLight& light)
  {
    ASSERT_MESSAGE(m_lights.find(&light) != m_lights.end(), "light could not be detached");
    m_lights.erase(&light);
    changed(light);
  }
  void changed(RendererLight& light)
  {
    m_lightsChanged = true;
  }
  void evaluateChanged()
  {
    if(m_lightsChanged)
    {
      m_lightsChanged = false;
      for(LightLists::iterator i = m_lightLists.begin(); i != m_lightLists.end(); ++i)
      {
        (*i).second.lightsChanged();
      }
    }
  }
  typedef MemberCaller<OpenGLShaderCache, &OpenGLShaderCache::evaluateChanged> EvaluateChangedCaller;

  typedef std::set<const Renderable*> Renderables; 
  Renderables m_renderables;
  mutable bool m_traverseRenderablesMutex;

  // renderables
  void attachRenderable(const Renderable& renderable)
  {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "attaching renderable during traversal");
    ASSERT_MESSAGE(m_renderables.find(&renderable) == m_renderables.end(), "renderable could not be attached");
    m_renderables.insert(&renderable);
  }
  void detachRenderable(const Renderable& renderable)
  {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "detaching renderable during traversal");
    ASSERT_MESSAGE(m_renderables.find(&renderable) != m_renderables.end(), "renderable could not be detached");
    m_renderables.erase(&renderable);
  }
  void forEachRenderable(const RenderableCallback& callback) const
  {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "for-each during traversal");
    m_traverseRenderablesMutex = true;
    for(Renderables::const_iterator i = m_renderables.begin(); i != m_renderables.end(); ++i)
    {
      callback(*(*i));
    }
    m_traverseRenderablesMutex = false;
  }
};

static OpenGLShaderCache* g_ShaderCache;

void ShaderCache_extensionsInitialised()
{
  g_ShaderCache->extensionsInitialised();
}

void ShaderCache_setBumpEnabled(bool enabled)
{
  g_ShaderCache->setLightingEnabled(enabled);
}


Vector3 g_DebugShaderColours[256];
Shader* g_defaultPointLight = 0;

void ShaderCache_Construct()
{
  g_ShaderCache = new OpenGLShaderCache;
  GlobalTexturesCache().attach(*g_ShaderCache);
  GlobalShaderSystem().attach(*g_ShaderCache);

	// Get the global default lightshader from the XML gamedescriptor.
	
	xml::NodeList nlDefaultLight = GlobalRegistry().findXPath("game/defaults/lightShader");
	if (nlDefaultLight.size() != 1)
		gtkutil::fatalErrorDialog("Failed to find default lightshader in XML game descriptor.\n\nNode <b>/game/defaults/lightShader</b> not found.");

	_defaultLightShaderName = nlDefaultLight[0].getContent();
	boost::algorithm::to_lower(_defaultLightShaderName);

	g_defaultPointLight = g_ShaderCache->capture(_defaultLightShaderName.c_str());
	g_ShaderCache->capture("$OVERBRIGHT");
}

void ShaderCache_Destroy()
{
    g_ShaderCache->release(_defaultLightShaderName.c_str());
    g_ShaderCache->release("$OVERBRIGHT");
    g_defaultPointLight = 0;

	GlobalShaderSystem().detach(*g_ShaderCache);
	GlobalTexturesCache().detach(*g_ShaderCache);
	delete g_ShaderCache;
}

ShaderCache* GetShaderCache()
{
  return g_ShaderCache;
}

inline void setTextureState(GLint& current, const GLint& texture, GLenum textureUnit)
{
  if(texture != current)
  {
    glActiveTexture(textureUnit);
    glClientActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
    GlobalOpenGL_debugAssertNoErrors();
    current = texture;
  }
}

inline void setTextureState(GLint& current, const GLint& texture)
{
  if(texture != current)
  {
    glBindTexture(GL_TEXTURE_2D, texture);
    GlobalOpenGL_debugAssertNoErrors();
    current = texture;
  }
}

inline void setState(unsigned int state, unsigned int delta, unsigned int flag, GLenum glflag)
{
  if(delta & state & flag)
  {
    glEnable(glflag);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & flag)
  {
    glDisable(glflag);
    GlobalOpenGL_debugAssertNoErrors();
  }
}

void OpenGLState_apply(const OpenGLState& self, OpenGLState& current, unsigned int globalstate)
{
  debug_int("sort", int(self.m_sort));
  debug_int("texture", self.m_texture);
  debug_int("state", self.m_state);
  debug_int("address", int(std::size_t(&self)));

  count_state();

  if(self.m_state & RENDER_OVERRIDE)
  {
    globalstate |= RENDER_FILL | RENDER_DEPTHWRITE;
  }

  const unsigned int state = self.m_state & globalstate;
  const unsigned int delta = state ^ current.m_state;

  GlobalOpenGL_debugAssertNoErrors();

  GLProgram* program = (state & RENDER_PROGRAM) != 0 ? self.m_program : 0;

  if(program != current.m_program)
  {
    if(current.m_program != 0)
    {
      current.m_program->disable();
      glColor4fv(current.m_colour);
      debug_colour("cleaning program");
    }

    current.m_program = program;

    if(current.m_program != 0)
    {
      current.m_program->enable();
    }
  }

  if(delta & state & RENDER_FILL)
  {
    //qglPolygonMode (GL_BACK, GL_LINE);
    //qglPolygonMode (GL_FRONT, GL_FILL);
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_FILL)
  {
    glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_OFFSETLINE, GL_POLYGON_OFFSET_LINE);

  if(delta & state & RENDER_LIGHTING)
  {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    //qglEnable(GL_RESCALE_NORMAL);
    glEnableClientState(GL_NORMAL_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
    g_normalArray_enabled = true;
  }
  else if(delta & ~state & RENDER_LIGHTING)
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    //qglDisable(GL_RESCALE_NORMAL);
    glDisableClientState(GL_NORMAL_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
    g_normalArray_enabled = false;
  }

  if(delta & state & RENDER_TEXTURE)
  {
    GlobalOpenGL_debugAssertNoErrors();

    if(GLEW_VERSION_1_3)
    {
      glActiveTexture(GL_TEXTURE0);
      glClientActiveTexture(GL_TEXTURE0);
    }

    glEnable(GL_TEXTURE_2D);

    glColor4f(1,1,1,self.m_colour[3]);
    debug_colour("setting texture");

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
    g_texcoordArray_enabled = true;
  }
  else if(delta & ~state & RENDER_TEXTURE)
  {
    if(GLEW_VERSION_1_3)
    {
      glActiveTexture(GL_TEXTURE0);
      glClientActiveTexture(GL_TEXTURE0);
    }

    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    GlobalOpenGL_debugAssertNoErrors();
    g_texcoordArray_enabled = false;
  }

  if(delta & state & RENDER_BLEND)
  {
// FIXME: some .TGA are buggy, have a completely empty alpha channel
// if such brushes are rendered in this loop they would be totally transparent with GL_MODULATE
// so I decided using GL_DECAL instead
// if an empty-alpha-channel or nearly-empty texture is used. It will be blank-transparent.
// this could get better if you can get glTexEnviv (GL_TEXTURE_ENV, to work .. patches are welcome

    glEnable(GL_BLEND);
    if(GLEW_VERSION_1_3)
    {
      glActiveTexture(GL_TEXTURE0);
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_BLEND)
  {
    glDisable(GL_BLEND);
    if(GLEW_VERSION_1_3)
    {
      glActiveTexture(GL_TEXTURE0);
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_CULLFACE, GL_CULL_FACE);

  if(delta & state & RENDER_SMOOTH)
  {
    glShadeModel(GL_SMOOTH);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_SMOOTH)
  {
    glShadeModel(GL_FLAT);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_SCALED, GL_NORMALIZE); // not GL_RESCALE_NORMAL

  setState(state, delta, RENDER_DEPTHTEST, GL_DEPTH_TEST);

  if(delta & state & RENDER_DEPTHWRITE)
  {
    glDepthMask(GL_TRUE);

#if DEBUG_RENDER
    GLboolean depthEnabled;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthEnabled);
    ASSERT_MESSAGE(depthEnabled, "failed to set depth buffer mask bit");
#endif
    debug_string("enabled depth-buffer writing");

    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_DEPTHWRITE)
  {
    glDepthMask(GL_FALSE);

#if DEBUG_RENDER
    GLboolean depthEnabled;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthEnabled);
    ASSERT_MESSAGE(!depthEnabled, "failed to set depth buffer mask bit");
#endif
    debug_string("disabled depth-buffer writing");

    GlobalOpenGL_debugAssertNoErrors();
  }

  if(delta & state & RENDER_COLOURWRITE)
  {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_COLOURWRITE)
  {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_ALPHATEST, GL_ALPHA_TEST);
  
  if(delta & state & RENDER_COLOURARRAY)
  {
    glEnableClientState(GL_COLOR_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
    debug_colour("enabling color_array");
    g_colorArray_enabled = true;
  }
  else if(delta & ~state & RENDER_COLOURARRAY)
  {
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4fv(self.m_colour);
    debug_colour("cleaning color_array");
    GlobalOpenGL_debugAssertNoErrors();
    g_colorArray_enabled = false;
  }

  if(delta & ~state & RENDER_COLOURCHANGE)
  {
    glColor4fv(self.m_colour);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_LINESTIPPLE, GL_LINE_STIPPLE);
  setState(state, delta, RENDER_LINESMOOTH, GL_LINE_SMOOTH);

  setState(state, delta, RENDER_POLYGONSTIPPLE, GL_POLYGON_STIPPLE);
  setState(state, delta, RENDER_POLYGONSMOOTH, GL_POLYGON_SMOOTH);

  setState(state, delta, RENDER_FOG, GL_FOG);

  if((state & RENDER_FOG) != 0)
  {
    setFogState(self.m_fog);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_fog = self.m_fog;
  }

  if(state & RENDER_DEPTHTEST && self.m_depthfunc != current.m_depthfunc)
  {
    glDepthFunc(self.m_depthfunc);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_depthfunc = self.m_depthfunc;
  }

  if(state & RENDER_LINESTIPPLE
    && (self.m_linestipple_factor != current.m_linestipple_factor
    || self.m_linestipple_pattern != current.m_linestipple_pattern))
  {
    glLineStipple(self.m_linestipple_factor, self.m_linestipple_pattern);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_linestipple_factor = self.m_linestipple_factor;
    current.m_linestipple_pattern = self.m_linestipple_pattern;
  }


  if(state & RENDER_ALPHATEST
    && ( self.m_alphafunc != current.m_alphafunc
    || self.m_alpharef != current.m_alpharef ) )
  {
    glAlphaFunc(self.m_alphafunc, self.m_alpharef);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_alphafunc = self.m_alphafunc;
    current.m_alpharef = self.m_alpharef;
  }

  {
    GLint texture0 = 0;
    GLint texture1 = 0;
    GLint texture2 = 0;
    GLint texture3 = 0;
    GLint texture4 = 0;
    GLint texture5 = 0;
    GLint texture6 = 0;
    GLint texture7 = 0;
    //if(state & RENDER_TEXTURE) != 0)
    {
      texture0 = self.m_texture;
      texture1 = self.m_texture1;
      texture2 = self.m_texture2;
      texture3 = self.m_texture3;
      texture4 = self.m_texture4;
      texture5 = self.m_texture5;
      texture6 = self.m_texture6;
      texture7 = self.m_texture7;
    }

    if(GLEW_VERSION_1_3)
    {
      setTextureState(current.m_texture, texture0, GL_TEXTURE0);
      setTextureState(current.m_texture1, texture1, GL_TEXTURE1);
      setTextureState(current.m_texture2, texture2, GL_TEXTURE2);
      setTextureState(current.m_texture3, texture3, GL_TEXTURE3);
      setTextureState(current.m_texture4, texture4, GL_TEXTURE4);
      setTextureState(current.m_texture5, texture5, GL_TEXTURE5);
      setTextureState(current.m_texture6, texture6, GL_TEXTURE6);
      setTextureState(current.m_texture7, texture7, GL_TEXTURE7);
    }
    else
    {
      setTextureState(current.m_texture, texture0);
    }
  }


  if(state & RENDER_TEXTURE && self.m_colour[3] != current.m_colour[3])
  {
    debug_colour("setting alpha");
    glColor4f(1,1,1,self.m_colour[3]);
    GlobalOpenGL_debugAssertNoErrors();
  }

  if(!(state & RENDER_TEXTURE)
    && (self.m_colour[0] != current.m_colour[0]
    || self.m_colour[1] != current.m_colour[1]
    || self.m_colour[2] != current.m_colour[2]
    || self.m_colour[3] != current.m_colour[3]))
  {
    glColor4fv(self.m_colour);
    debug_colour("setting non-texture");
    GlobalOpenGL_debugAssertNoErrors();
  }
  current.m_colour = self.m_colour;

  if(state & RENDER_BLEND
    && (self.m_blend_src != current.m_blend_src || self.m_blend_dst != current.m_blend_dst))
  {
    glBlendFunc(self.m_blend_src, self.m_blend_dst);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_blend_src = self.m_blend_src;
    current.m_blend_dst = self.m_blend_dst;
  }

  if(!(state & RENDER_FILL)
    && self.m_linewidth != current.m_linewidth)
  {
    glLineWidth(self.m_linewidth);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_linewidth = self.m_linewidth;
  }

  if(!(state & RENDER_FILL)
    && self.m_pointsize != current.m_pointsize)
  {
    glPointSize(self.m_pointsize);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_pointsize = self.m_pointsize;
  }

  current.m_state = state;

  GlobalOpenGL_debugAssertNoErrors();
}

OpenGLStateMap* g_openglStates = 0;

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class OpenGLStateLibraryAPI
{
  OpenGLStateMap m_stateMap;
public:
  typedef OpenGLStateLibrary Type;
  STRING_CONSTANT(Name, "*");

  OpenGLStateLibraryAPI()
  {
    g_openglStates = &m_stateMap;
  }
  ~OpenGLStateLibraryAPI()
  {
    g_openglStates = 0;
  }
  OpenGLStateLibrary* getTable()
  {
    return &m_stateMap;
  }
};

typedef SingletonModule<OpenGLStateLibraryAPI> OpenGLStateLibraryModule;
typedef Static<OpenGLStateLibraryModule> StaticOpenGLStateLibraryModule;
StaticRegisterModule staticRegisterOpenGLStateLibrary(StaticOpenGLStateLibraryModule::instance());

class ShaderCacheDependencies : public GlobalShadersModuleRef, public GlobalTexturesModuleRef, public GlobalOpenGLStateLibraryModuleRef
{
public:
  ShaderCacheDependencies() :
    GlobalShadersModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("shaders"))
  {
  }
};

class ShaderCacheAPI
{
  ShaderCache* m_shaderCache;
public:
  typedef ShaderCache Type;
  STRING_CONSTANT(Name, "*");

  ShaderCacheAPI()
  {
    ShaderCache_Construct();

    m_shaderCache = GetShaderCache();
  }
  ~ShaderCacheAPI()
  {
    ShaderCache_Destroy();
  }
  ShaderCache* getTable()
  {
    return m_shaderCache;
  }
};

typedef SingletonModule<ShaderCacheAPI, ShaderCacheDependencies> ShaderCacheModule;
typedef Static<ShaderCacheModule> StaticShaderCacheModule;
StaticRegisterModule staticRegisterShaderCache(StaticShaderCacheModule::instance());


