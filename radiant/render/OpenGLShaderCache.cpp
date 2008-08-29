#include "OpenGLShaderCache.h"

#include "ishaders.h"
#include "math/matrix.h"
#include "modulesystem/StaticModule.h"
#include "backend/GLProgramFactory.h"

#include <boost/weak_ptr.hpp>

namespace render {

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

/**
 * Main constructor.
 */
OpenGLShaderCache::OpenGLShaderCache() : 
	_realised(false),
	m_lightingEnabled(true),
	m_lightingSupported(false),
	m_lightsChanged(true),
	m_traverseRenderablesMutex(false)
{}
	
/* Capture the given shader.
 */
ShaderPtr OpenGLShaderCache::capture(const std::string& name) {
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
	if (_realised) {
		shd->realise(name);
	}
		
	// Return the new shader
	return shd;
}

/*
 * Render all states in the ShaderCache along with their renderables. This
 * is where the actual OpenGL rendering starts.
 */
void OpenGLShaderCache::render(RenderStateFlags globalstate, 
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

    if (GLEW_VERSION_1_3) {
		glActiveTexture(GL_TEXTURE0);
		glClientActiveTexture(GL_TEXTURE0);
    }

    if (GLEW_ARB_shader_objects) {
		glUseProgramObjectARB(0);
		glDisableVertexAttribArrayARB(c_attr_TexCoord0);
		glDisableVertexAttribArrayARB(c_attr_Tangent);
		glDisableVertexAttribArrayARB(c_attr_Binormal);
    }

    if (globalstate & RENDER_TEXTURE) {
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
	for (OpenGLStates::iterator i = _state_sorted.begin(); 
		i != _state_sorted.end(); 
		++i)
	{
		// Render the OpenGLShaderPass
		i->second->render(current, globalstate, viewer);
	}
}
	
void OpenGLShaderCache::realise() {
    if (_realised) {
    	return; // already realised
    }
    
    _realised = true;
	
	if (lightingSupported() && lightingEnabled()) {
		// Realise the GLPrograms
		GLProgramFactory::realise();
	}

	// Realise the OpenGLShader objects
	for (ShaderMap::iterator i = _shaders.begin(); 
		 i != _shaders.end(); ++i)
	{
		OpenGLShaderPtr sp = i->second.lock();
		if (sp != NULL) {
			sp->realise(i->first);
		}
	}
}
	
void OpenGLShaderCache::unrealise() {
    if (!_realised) {
    	return;
    }
    
    _realised = false;
    
	// Unrealise the OpenGLShader objects
	for (ShaderMap::iterator i = _shaders.begin(); 
		 i != _shaders.end(); 
		 ++i)
	{
		OpenGLShaderPtr sp = i->second.lock();
		if (sp != NULL) {
			sp->unrealise();
		}
    }
	
	if(GlobalOpenGL().contextValid && lightingSupported() && lightingEnabled()) {
		// Unrealise the GLPrograms
		render::GLProgramFactory::unrealise();
	}
}

bool OpenGLShaderCache::lightingEnabled() const {
	return m_lightingEnabled;
}
	
bool OpenGLShaderCache::lightingSupported() const {
	return m_lightingSupported;
}
	
void OpenGLShaderCache::setLighting(bool supported, bool enabled) {
	bool refresh = (m_lightingSupported && m_lightingEnabled) != (supported && enabled);

	if (refresh) {
		unrealise();
		GlobalShaderSystem().setLightingEnabled(supported && enabled);
	}

	m_lightingSupported = supported;
	m_lightingEnabled = enabled;

	if (refresh) {
		realise();
	}
}
	
void OpenGLShaderCache::extensionsInitialised() {
	setLighting(GLEW_VERSION_1_3 && 
				GLEW_ARB_vertex_program && 
				GLEW_ARB_fragment_program && 
				GLEW_ARB_shader_objects && 
				GLEW_ARB_vertex_shader && 
				GLEW_ARB_fragment_shader && 
				GLEW_ARB_shading_language_100, 
				m_lightingEnabled);

    if (!lightingSupported()) {
    	
		globalOutputStream() << "Lighting mode requires OpenGL features not supported by your graphics drivers:\n";
		
		if (!GLEW_VERSION_1_3) {
			globalOutputStream() << "  GL version 1.3 or better\n";
		}
		
		if (!GLEW_ARB_vertex_program) {
			globalOutputStream() << "  GL_ARB_vertex_program\n";
		}
		
		if (!GLEW_ARB_fragment_program) {
			globalOutputStream() << "  GL_ARB_fragment_program\n";
		}
		
		if (!GLEW_ARB_shader_objects) {
			globalOutputStream() << "  GL_ARB_shader_objects\n";
		}
		
		if (!GLEW_ARB_vertex_shader) {
			globalOutputStream() << "  GL_ARB_vertex_shader\n";
		}
		
		if (!GLEW_ARB_fragment_shader) {
			globalOutputStream() << "  GL_ARB_fragment_shader\n";
		}
		
		if (!GLEW_ARB_shading_language_100) {
			globalOutputStream() << "  GL_ARB_shading_language_100\n";
		}
	}
}

void OpenGLShaderCache::setLightingEnabled(bool enabled) {
	setLighting(m_lightingSupported, enabled);
}

const LightList& OpenGLShaderCache::attach(LightCullable& cullable) {
	return (*m_lightLists.insert(LightLists::value_type(&cullable, LinearLightList(cullable, m_lights, EvaluateChangedCaller(*this)))).first).second;
}

void OpenGLShaderCache::detach(LightCullable& cullable) {
	m_lightLists.erase(&cullable);
}

void OpenGLShaderCache::changed(LightCullable& cullable) {
	LightLists::iterator i = m_lightLists.find(&cullable);
	ASSERT_MESSAGE(i != m_lightLists.end(), "cullable not attached");
	i->second.lightsChanged();
}

void OpenGLShaderCache::attach(RendererLight& light) {
    ASSERT_MESSAGE(m_lights.find(&light) == m_lights.end(), "light could not be attached");
    m_lights.insert(&light);
    changed(light);
}

void OpenGLShaderCache::detach(RendererLight& light) {
    ASSERT_MESSAGE(m_lights.find(&light) != m_lights.end(), "light could not be detached");
    m_lights.erase(&light);
    changed(light);
}

void OpenGLShaderCache::changed(RendererLight& light) {
    m_lightsChanged = true;
}

void OpenGLShaderCache::evaluateChanged() {
    if (m_lightsChanged) {
    	m_lightsChanged = false;
    	for (LightLists::iterator i = m_lightLists.begin(); i != m_lightLists.end(); ++i) {
    		i->second.lightsChanged();
    	}
	}
}

void OpenGLShaderCache::insertSortedState(const OpenGLStates::value_type& val) {
	_state_sorted.insert(val);	
}

void OpenGLShaderCache::eraseSortedState(const OpenGLStates::key_type& key) {
	_state_sorted.erase(key);
}

// renderables
void OpenGLShaderCache::attachRenderable(const Renderable& renderable) {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "attaching renderable during traversal");
    ASSERT_MESSAGE(m_renderables.find(&renderable) == m_renderables.end(), "renderable could not be attached");
    m_renderables.insert(&renderable);
}

void OpenGLShaderCache::detachRenderable(const Renderable& renderable) {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "detaching renderable during traversal");
    ASSERT_MESSAGE(m_renderables.find(&renderable) != m_renderables.end(), "renderable could not be detached");
    m_renderables.erase(&renderable);
}

void OpenGLShaderCache::forEachRenderable(const RenderableCallback& callback) const {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "for-each during traversal");
    m_traverseRenderablesMutex = true;
    for (Renderables::const_iterator i = m_renderables.begin(); i != m_renderables.end(); ++i) {
    	callback(*(*i));
	}
	m_traverseRenderablesMutex = false;
}
  
// RegisterableModule implementation
const std::string& OpenGLShaderCache::getName() const {
	static std::string _name(MODULE_SHADERCACHE);
	return _name;
}

const StringSet& OpenGLShaderCache::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_SHADERSYSTEM);
		_dependencies.insert(MODULE_OPENGL_STATE_LIBRARY);
		_dependencies.insert(MODULE_OPENGL);
	}

	return _dependencies;
}

void OpenGLShaderCache::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "ShaderCache::initialiseModule called.\n";
	
	GlobalShaderSystem().attach(*this);
	
	capture("$OVERBRIGHT");
	
	// greebo: Don't realise the module yet, this must wait
	// until the shared GL context has been created (this 
	// happens as soon as the first GL widget has been realised).
}
	
void OpenGLShaderCache::shutdownModule() {
	GlobalShaderSystem().detach(*this);
}

// Define the static ShaderCache module
module::StaticModule<OpenGLShaderCache> openGLShaderCacheModule;

OpenGLShaderCache& getOpenGLShaderCache() {
	return *openGLShaderCacheModule.getModule();
}

} // namespace render
