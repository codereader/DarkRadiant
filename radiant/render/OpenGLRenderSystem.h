#pragma once

#include "irender.h"
#include <map>
#include "imodule.h"
#include "backend/OpenGLStateManager.h"
#include "backend/OpenGLShader.h"
#include "LinearLightList.h"
#include "render/backend/OpenGLStateLess.h"

#include <boost/weak_ptr.hpp>

namespace render
{

/**
 * \brief
 * Implementation of RenderSystem.
 */
class OpenGLRenderSystem
: public RenderSystem,
  public OpenGLStateManager,
  public ModuleObserver
{
private:
	// Map of named Shader objects
	typedef std::map<std::string, OpenGLShaderPtr> ShaderMap;
	ShaderMap _shaders;

	// whether this module has been realised
	bool _realised;

    // Current shader program in use
    ShaderProgram _currentShaderProgram;

    // OpenGL shader programs available/unavailable
	bool _shadersAvailable;

	// Map of OpenGLState references, with access functions.
	OpenGLStates _state_sorted;

	// Render time
	std::size_t _time;

	// Lights
	RendererLights m_lights;
	bool m_lightsChanged;
	typedef std::map<LitObject*, LinearLightList> LightLists;
	LightLists m_lightLists;

private:
	void propagateLightChangedFlagToAllLights();

public:

	/**
	 * Main constructor.
	 */
	OpenGLRenderSystem();

	virtual ~OpenGLRenderSystem();

    /* RenderSystem implementation */

	ShaderPtr capture(const std::string& name);
	void render(RenderStateFlags globalstate,
				const Matrix4& modelview,
				const Matrix4& projection,
				const Vector3& viewer);
	void realise();
	void unrealise();

	std::size_t getTime() const;
	void setTime(std::size_t milliSeconds);

    bool shaderProgramsAvailable() const;
    ShaderProgram getCurrentShaderProgram() const;
    void setShaderProgram(ShaderProgram prog);

	void extensionsInitialised();

	LightList& attachLitObject(LitObject& cullable);
	void detachLitObject(LitObject& cullable);
	void litObjectChanged(LitObject& cullable);

    // Attach and detach light sources
	void attachLight(RendererLight& light);
	void detachLight(RendererLight& light);
	void lightChanged(RendererLight& light);

	typedef std::set<const Renderable*> Renderables;
	Renderables m_renderables;
	mutable bool m_traverseRenderablesMutex;

    /* OpenGLStateManager implementation */
	void insertSortedState(const OpenGLStates::value_type& val);
	void eraseSortedState(const OpenGLStates::key_type& key);

	// renderables
	void attachRenderable(const Renderable& renderable);
	void detachRenderable(const Renderable& renderable);
	void forEachRenderable(const RenderableCallback& callback) const;

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();
};
typedef boost::shared_ptr<OpenGLRenderSystem> OpenGLRenderSystemPtr;

} // namespace render

