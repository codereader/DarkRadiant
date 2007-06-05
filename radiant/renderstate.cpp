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
#include "igl.h"
#include "iglrender.h"
#include "renderable.h"
#include "iradiant.h"

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
#include "generic/reference.h"
#include "moduleobservers.h"
#include "stream/filestream.h"
#include "stream/stringstream.h"
#include "os/file.h"
#include "plugin.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/weak_ptr.hpp>

/* GLOBALS */

namespace {
	
	// Polygon stipple pattern
	const GLubyte POLYGON_STIPPLE_PATTERN[132] = {
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

#define LIGHT_SHADER_DEBUG 0

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

#define DEBUG_SHADERS 0

class OpenGLShaderCache : public ShaderCache, public ModuleObserver
{
	// Map of named Shader objects
	typedef boost::shared_ptr<OpenGLShader> OpenGLShaderPtr;
	typedef boost::weak_ptr<OpenGLShader> OpenGLShaderWeakPtr;
	typedef std::map<std::string, OpenGLShaderWeakPtr> ShaderMap;
	ShaderMap _shaders;
	
	// Stupid unrealised counter
	std::size_t m_unrealised;

  bool m_lightingEnabled;
  bool m_lightingSupported;

public:

	/**
	 * Main constructor.
	 */
	OpenGLShaderCache()
    : m_unrealised(2), // wait until shaders, gl-context and textures are realised before creating any render-states
    				 // greebo: I set this down to the value 2 after removing TexturesCache 
    				 // (What the heck *is* this anyway, a hardcoded egg-timer?)
	  m_lightingEnabled(true),
	  m_lightingSupported(false),
	  m_lightsChanged(true),
	  m_traverseRenderablesMutex(false)
	{ }

	/* Capture the given shader.
	 */
	ShaderPtr capture(const std::string& name) {
		
		// Usual ritual, check cache and return if found, otherwise create/
		// insert/return.
		ShaderMap::const_iterator i = _shaders.find(name);
		if (i != _shaders.end()) {
			// Try to lock pointer, which will fail if the object has been
			// deleted
			OpenGLShaderPtr sp = i->second.lock();
			if (sp)
				return sp;
		}

		// Either the shader was not found, or the weak pointer failed to lock
		// because the shader had been deleted. Either way, create a new shader
		// and insert into the cache.
		OpenGLShaderPtr shd(new OpenGLShader());
		_shaders[name] = shd;
			
		// Realise the shader if the cache is realised
		if (realised())
			shd->realise(name);
			
		// Return the new shader
		return shd;
	}

  	/*
  	 * Render all states in the ShaderCache along with their renderables. This
  	 * is where the actual OpenGL rendering starts.
  	 */
	void render(RenderStateFlags globalstate, 
				const Matrix4& modelview, 
				const Matrix4& projection, 
				const Vector3& viewer)
	{
		// Set the projection and modelview matrices
	    glMatrixMode(GL_PROJECTION);
	    glLoadMatrixd(projection);

	    glMatrixMode(GL_MODELVIEW);
	    glLoadMatrixd(modelview);
 
	    // global settings that are not set in renderstates
	    glFrontFace(GL_CW);
	    glCullFace(GL_BACK);
	    glPolygonOffset(-1, 1);

		// Set polygon stipple pattern from constant
		glPolygonStipple(POLYGON_STIPPLE_PATTERN);

	    glEnableClientState(GL_VERTEX_ARRAY);
	    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	    if(GLEW_VERSION_1_3) {
			glActiveTexture(GL_TEXTURE0);
			glClientActiveTexture(GL_TEXTURE0);
	    }

	    if(GLEW_ARB_shader_objects) {
			glUseProgramObjectARB(0);
			glDisableVertexAttribArrayARB(c_attr_TexCoord0);
			glDisableVertexAttribArrayARB(c_attr_Tangent);
			glDisableVertexAttribArrayARB(c_attr_Binormal);
	    }

	    if(globalstate & RENDER_TEXTURE) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    }

		// Construct default OpenGL state
		OpenGLState current;
		current.m_sort = OpenGLState::eSortFirst;

	    // default renderstate settings
	    glLineStipple(current.m_linestipple_factor, 
	    			  current.m_linestipple_pattern);
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    glDisable(GL_LIGHTING);
	    glDisable(GL_TEXTURE_2D);
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	    glDisableClientState(GL_COLOR_ARRAY);
	    glDisableClientState(GL_NORMAL_ARRAY);
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

		// Iterate over the sorted mapping between OpenGLStates and their
		// OpenGLStateBuckets (containing the renderable geometry), and render
		// the contents of each bucket. Each state bucket is passed a reference
		// to the "current" state, which it can change.
		for(OpenGLStates::iterator i = g_state_sorted.begin(); 
			i != g_state_sorted.end(); 
			++i)
		{
			// Render the OpenGLStateBucket
			i->second->render(current, globalstate, viewer);
		}
  }
  void realise()
  {
    if(--m_unrealised == 0)
    {
		if(lightingSupported() && lightingEnabled()) {
			// Realise the GLPrograms
			render::GLProgramFactory::realise();
		}

		// Realise the OpenGLShader objects
		for (ShaderMap::iterator i = _shaders.begin(); 
			 i != _shaders.end(); 
			 ++i)
		{
			OpenGLShaderPtr sp = i->second.lock();
			if (sp)
				sp->realise(i->first);
        }
    }
  }
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
		// Unrealise the OpenGLShader objects
		for (ShaderMap::iterator i = _shaders.begin(); 
			 i != _shaders.end(); 
			 ++i)
		{
			OpenGLShaderPtr sp = i->second.lock();
			if (sp)
				sp->unrealise();
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

void ShaderCache_Construct()
{
  g_ShaderCache = new OpenGLShaderCache;
  GlobalShaderSystem().attach(*g_ShaderCache);

	g_ShaderCache->capture("$OVERBRIGHT");
}

void ShaderCache_Destroy()
{
	GlobalShaderSystem().detach(*g_ShaderCache);
	delete g_ShaderCache;
}

ShaderCache* GetShaderCache()
{
  return g_ShaderCache;
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

class ShaderCacheDependencies : public GlobalShadersModuleRef, public GlobalOpenGLStateLibraryModuleRef
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


