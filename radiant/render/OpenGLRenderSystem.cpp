#include "OpenGLRenderSystem.h"

#include "ishaders.h"
#include "itextstream.h"
#include "math/Matrix4.h"
#include "modulesystem/StaticModule.h"
#include "backend/GLProgramFactory.h"

#include <boost/weak_ptr.hpp>
#include <boost/bind.hpp>

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
OpenGLRenderSystem::OpenGLRenderSystem() :
	_realised(false),
	_currentShaderProgram(SHADER_PROGRAM_NONE),
	_shadersAvailable(false),
	_time(0),
	m_lightsChanged(true),
	m_traverseRenderablesMutex(false)
{
	// For the static default rendersystem, the MaterialManager is not existent yet,
	// hence it will be attached in initialiseModule().
	if (module::getRegistry().moduleExists(MODULE_SHADERSYSTEM))
	{
		GlobalMaterialManager().attach(*this);
	}
}

OpenGLRenderSystem::~OpenGLRenderSystem()
{
	if (module::getRegistry().moduleExists(MODULE_SHADERSYSTEM))
	{
		GlobalMaterialManager().detach(*this);
	}
}

ShaderPtr OpenGLRenderSystem::capture(const std::string& name)
{
	// Usual ritual, check cache and return if found, otherwise create/
	// insert/return.
	ShaderMap::const_iterator i = _shaders.find(name);

	if (i != _shaders.end())
	{
        return i->second;
	}

	// Either the shader was not found, or the weak pointer failed to lock
	// because the shader had been deleted. Either way, create a new shader
	// and insert into the cache.
	OpenGLShaderPtr shd(new OpenGLShader(*this));
	_shaders[name] = shd;

	// Realise the shader if the cache is realised
	if (_realised)
	{
		shd->realise(name);
	}

	// Return the new shader
	return shd;
}

/*
 * Render all states in the ShaderCache along with their renderables. This
 * is where the actual OpenGL rendering starts.
 */
void OpenGLRenderSystem::render(RenderStateFlags globalstate,
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

    if (globalstate & RENDER_TEXTURE_2D) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

	// Construct default OpenGL state
	OpenGLState current;

    // Set up initial GL state. This MUST MATCH the defaults in the OpenGLState
    // object, otherwise required state changes may not occur.
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

    // RENDER_DEPTHWRITE defaults to 0
    glDepthMask(GL_FALSE);

    // RENDER_MASKCOLOUR defaults to 0
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDisable(GL_ALPHA_TEST);

    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_POLYGON_STIPPLE);
    glDisable(GL_POLYGON_OFFSET_LINE);
	glDisable(GL_POLYGON_OFFSET_FILL); // greebo: otherwise tiny gap lines between brushes are visible

    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(1,1,1,1);
    glDepthFunc(current.getDepthFunc());
    glAlphaFunc(GL_ALWAYS, 0);
    glLineWidth(1);
    glPointSize(1);

	glHint(GL_FOG_HINT, GL_NICEST);
    glDisable(GL_FOG);

    // Iterate over the sorted mapping between OpenGLStates and their
    // OpenGLShaderPasses (containing the renderable geometry), and render the
    // contents of each bucket. Each pass is passed a reference to the "current"
    // state, which it can change.
	for (OpenGLStates::iterator i = _state_sorted.begin();
		i != _state_sorted.end();
		++i)
	{
        // Render the OpenGLShaderPass
        if (!i->second->empty())
        {
            i->second->render(current, globalstate, viewer, _time);
        }
	}
}

void OpenGLRenderSystem::realise()
{
    if (_realised) {
        return; // already realised
    }

    _realised = true;

    if (shaderProgramsAvailable()
        && getCurrentShaderProgram() != SHADER_PROGRAM_NONE)
    {
        // Realise the GLPrograms
        GLProgramFactory::instance().realise();
    }

    // Realise the OpenGLShader objects
    for (ShaderMap::iterator i = _shaders.begin(); i != _shaders.end(); ++i)
    {
        OpenGLShaderPtr sp = i->second;
        assert(sp);

        sp->realise(i->first);
    }
}

void OpenGLRenderSystem::unrealise()
{
    if (!_realised) {
    	return;
    }

    _realised = false;

	// Unrealise the OpenGLShader objects
	for (ShaderMap::iterator i = _shaders.begin(); i != _shaders.end(); ++i)
	{
        OpenGLShaderPtr sp = i->second;
        assert(sp);

        sp->unrealise();
    }

	if (GlobalOpenGL().contextValid()
        && shaderProgramsAvailable()
        && getCurrentShaderProgram() != SHADER_PROGRAM_NONE)
    {
		// Unrealise the GLPrograms
		GLProgramFactory::instance().unrealise();
	}
}

std::size_t OpenGLRenderSystem::getTime() const
{
	return _time;
}

void OpenGLRenderSystem::setTime(std::size_t milliSeconds)
{
	_time = milliSeconds;
}

RenderSystem::ShaderProgram OpenGLRenderSystem::getCurrentShaderProgram() const
{
	return _currentShaderProgram;
}

bool OpenGLRenderSystem::shaderProgramsAvailable() const
{
	return _shadersAvailable;
}

void OpenGLRenderSystem::setShaderProgram(RenderSystem::ShaderProgram newProg)
{
    ShaderProgram oldProgram = _currentShaderProgram;

    if (oldProgram != newProg)
    {
		unrealise();
		GlobalMaterialManager().setLightingEnabled(
            newProg == SHADER_PROGRAM_INTERACTION
        );
    }

    _currentShaderProgram = newProg;

    if (oldProgram != newProg)
    {
        realise();
    }
}

void OpenGLRenderSystem::extensionsInitialised()
{
    // Determine if lighting is available based on GL extensions
	bool glslLightingAvailable = GLEW_VERSION_2_0 ? true : false;
    bool arbLightingAvailable  = GLEW_VERSION_1_3
                                 && GLEW_ARB_vertex_program
                                 && GLEW_ARB_fragment_program;

#if defined(DEBUG_NO_LIGHTING)
    glslLightingAvailable = arbLightingAvailable = false;
#endif

    std::cout << "[OpenGLRenderSystem] GLSL shading "
              << (glslLightingAvailable ? "IS" : "IS NOT" ) << " available."
              << std::endl;
    std::cout << "[OpenGLRenderSystem] ARB shading "
              << (arbLightingAvailable ? "IS" : "IS NOT" ) << " available."
              << std::endl;

    // Tell the GLProgramFactory which to use
    if (glslLightingAvailable)
    {
        GLProgramFactory::instance().setUsingGLSL(true);
    }
    else
    {
        GLProgramFactory::instance().setUsingGLSL(false);
    }

    // Set internal flags
    _shadersAvailable = glslLightingAvailable || arbLightingAvailable;

    // Inform the user of missing extensions
    if (!shaderProgramsAvailable())
    {
		rMessage() << "GL shading requires OpenGL features not"
                             << " supported by your graphics drivers:\n";

		if (!GLEW_VERSION_2_0) {
			rMessage() << "  GL version 2.0 or better\n";
		}

		if (!GLEW_ARB_shader_objects) {
			rMessage() << "  GL_ARB_shader_objects\n";
		}

		if (!GLEW_ARB_vertex_shader) {
			rMessage() << "  GL_ARB_vertex_shader\n";
		}

		if (!GLEW_ARB_fragment_shader) {
			rMessage() << "  GL_ARB_fragment_shader\n";
		}

		if (!GLEW_ARB_shading_language_100) {
			rMessage() << "  GL_ARB_shading_language_100\n";
		}

		if (!GLEW_ARB_vertex_program) {
			rMessage() << "  GL_ARB_vertex_program\n";
		}

		if (!GLEW_ARB_fragment_program) {
			rMessage() << "  GL_ARB_fragment_program\n";
		}

	}
}

LightList& OpenGLRenderSystem::attachLitObject(LitObject& object)
{
	return m_lightLists.insert(
		LightLists::value_type(
			&object,
			LinearLightList(
                object,
                m_lights,
                boost::bind(
                    &OpenGLRenderSystem::propagateLightChangedFlagToAllLights,
                    this
                )
            )
        )
    ).first->second;
}

void OpenGLRenderSystem::detachLitObject(LitObject& object) 
{
	m_lightLists.erase(&object);
}

void OpenGLRenderSystem::litObjectChanged(LitObject& object) 
{
	LightLists::iterator i = m_lightLists.find(&object);
    assert(i != m_lightLists.end());

	i->second.setDirty();
}

void OpenGLRenderSystem::attachLight(RendererLight& light)
{
    ASSERT_MESSAGE(m_lights.find(&light) == m_lights.end(), "light could not be attached");
    m_lights.insert(&light);
    lightChanged(light);
}

void OpenGLRenderSystem::detachLight(RendererLight& light)
{
    ASSERT_MESSAGE(m_lights.find(&light) != m_lights.end(), "light could not be detached");
    m_lights.erase(&light);
    lightChanged(light);
}

void OpenGLRenderSystem::lightChanged(RendererLight& light)
{
    m_lightsChanged = true;
}

void OpenGLRenderSystem::propagateLightChangedFlagToAllLights()
{
    if (m_lightsChanged)
    {
    	m_lightsChanged = false;
    	for (LightLists::iterator i = m_lightLists.begin();
             i != m_lightLists.end();
             ++i) 
        {
    		i->second.setDirty();
    	}
	}
}

void OpenGLRenderSystem::insertSortedState(const OpenGLStates::value_type& val) {
	_state_sorted.insert(val);
}

void OpenGLRenderSystem::eraseSortedState(const OpenGLStates::key_type& key) {
	_state_sorted.erase(key);
}

// renderables
void OpenGLRenderSystem::attachRenderable(const Renderable& renderable) {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "attaching renderable during traversal");
    ASSERT_MESSAGE(m_renderables.find(&renderable) == m_renderables.end(), "renderable could not be attached");
    m_renderables.insert(&renderable);
}

void OpenGLRenderSystem::detachRenderable(const Renderable& renderable) {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "detaching renderable during traversal");
    ASSERT_MESSAGE(m_renderables.find(&renderable) != m_renderables.end(), "renderable could not be detached");
    m_renderables.erase(&renderable);
}

void OpenGLRenderSystem::forEachRenderable(const RenderableCallback& callback) const {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "for-each during traversal");
    m_traverseRenderablesMutex = true;
    for (Renderables::const_iterator i = m_renderables.begin(); i != m_renderables.end(); ++i) {
    	callback(*(*i));
	}
	m_traverseRenderablesMutex = false;
}

// RegisterableModule implementation
const std::string& OpenGLRenderSystem::getName() const {
	static std::string _name(MODULE_RENDERSYSTEM);
	return _name;
}

const StringSet& OpenGLRenderSystem::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_SHADERSYSTEM);
		_dependencies.insert(MODULE_OPENGL);
	}

	return _dependencies;
}

void OpenGLRenderSystem::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "ShaderCache::initialiseModule called.\n";

	GlobalMaterialManager().attach(*this);

	// greebo: Don't realise the module yet, this must wait
	// until the shared GL context has been created (this
	// happens as soon as the first GL widget has been realised).
}

void OpenGLRenderSystem::shutdownModule()
{
}

// Define the static ShaderCache module
module::StaticModule<OpenGLRenderSystem> openGLRenderSystemModule;

} // namespace render
