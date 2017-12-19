#pragma once

#include "irender.h"
#include <sigc++/connection.h>
#include <map>
#include "imodule.h"
#include "backend/OpenGLStateManager.h"
#include "backend/OpenGLShader.h"
#include "LinearLightList.h"
#include "render/backend/OpenGLStateLess.h"

namespace render
{

class GLProgramFactory;
typedef std::shared_ptr<GLProgramFactory> GLProgramFactoryPtr;

/**
 * \brief
 * Implementation of RenderSystem.
 */
class OpenGLRenderSystem
: public RenderSystem,
  public OpenGLStateManager
{
private:
	// Map of named Shader objects
	typedef std::map<std::string, OpenGLShaderPtr> ShaderMap;
	ShaderMap _shaders;

	// whether this module has been realised
	bool _realised;

    // The GL program manager to acquire vfps
    GLProgramFactoryPtr _glProgramFactory;

    // Current shader program in use
    ShaderProgram _currentShaderProgram;

	// Map of OpenGLState references, with access functions.
	OpenGLStates _state_sorted;

	// Render time
	std::size_t _time;

	// Lights
	RendererLights m_lights;
	bool m_lightsChanged;
	typedef std::map<LitObject*, LinearLightList> LightLists;
	LightLists m_lightLists;

	sigc::signal<void> _sigExtensionsInitialised;

	sigc::connection _materialDefsLoaded;
	sigc::connection _materialDefsUnloaded;

private:
	void propagateLightChangedFlagToAllLights();

public:

	/**
	 * Main constructor.
	 */
	OpenGLRenderSystem();

	virtual ~OpenGLRenderSystem();

    /* RenderSystem implementation */

	ShaderPtr capture(const std::string& name) override;
	void render(RenderStateFlags globalstate,
				const Matrix4& modelview,
				const Matrix4& projection,
				const Vector3& viewer) override;
	void realise() override;
	void unrealise() override;

    GLProgramFactory& getGLProgramFactory();

	std::size_t getTime() const override;
	void setTime(std::size_t milliSeconds) override;

    ShaderProgram getCurrentShaderProgram() const override;
    void setShaderProgram(ShaderProgram prog) override;

	void extensionsInitialised() override;
	sigc::signal<void> signal_extensionsInitialised() override;

	LightList& attachLitObject(LitObject& cullable) override;
	void detachLitObject(LitObject& cullable) override;
	void litObjectChanged(LitObject& cullable) override;

    // Attach and detach light sources
	void attachLight(RendererLight& light) override;
	void detachLight(RendererLight& light) override;
	void lightChanged(RendererLight& light) override;

	typedef std::set<const Renderable*> Renderables;
	Renderables m_renderables;
	mutable bool m_traverseRenderablesMutex;

    /* OpenGLStateManager implementation */
	void insertSortedState(const OpenGLStates::value_type& val) override;
	void eraseSortedState(const OpenGLStates::key_type& key) override;

	// renderables
	void attachRenderable(const Renderable& renderable) override;
	void detachRenderable(const Renderable& renderable) override;
	void forEachRenderable(const RenderableCallback& callback) const override;

	// RegisterableModule implementation
    virtual const std::string& getName() const override;
    virtual const StringSet& getDependencies() const override;
    virtual void initialiseModule(const ApplicationContext& ctx) override;
    virtual void shutdownModule() override;
};
typedef std::shared_ptr<OpenGLRenderSystem> OpenGLRenderSystemPtr;

} // namespace render

