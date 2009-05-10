#ifndef OPENGLSHADERCACHE_H_
#define OPENGLSHADERCACHE_H_

#include "irender.h"
#include <map>
#include "moduleobserver.h"
#include "backend/OpenGLShader.h"
#include "LinearLightList.h"
#include "render/backend/OpenGLStateLess.h"
#include "generic/reference.h"

class OpenGLState;
class OpenGLShaderPass;

/* Sorted state map */

typedef ConstReference<OpenGLState> OpenGLStateReference;
typedef std::map<OpenGLStateReference, 
				 OpenGLShaderPass*, 
				 OpenGLStateLess> OpenGLStates;

namespace render {

/**
 * \brief
 * Implementation of RenderSystem.
 */
class OpenGLRenderSystem 
: public RenderSystem, 
  public ModuleObserver
{
	// Map of named Shader objects
	typedef boost::shared_ptr<OpenGLShader> OpenGLShaderPtr;
	typedef boost::weak_ptr<OpenGLShader> OpenGLShaderWeakPtr;
	typedef std::map<std::string, OpenGLShaderWeakPtr> ShaderMap;
	ShaderMap _shaders;
	
	// whether this module has been realised
	bool _realised;

	bool m_lightingEnabled;
	bool m_lightingSupported;
	
	// Map of OpenGLState references, with access functions.
	OpenGLStates _state_sorted;

private:

    // Set internal lighting-supported and lighting-enabled flags
	void setLighting(bool supported, bool enabled);
	
public:

	/**
	 * Main constructor.
	 */
	OpenGLRenderSystem();
	
	/* Capture the given shader.
	 */
	ShaderPtr capture(const std::string& name);

  	/*
  	 * Render all states in the ShaderCache along with their renderables. This
  	 * is where the actual OpenGL rendering starts.
  	 */
	void render(RenderStateFlags globalstate, 
				const Matrix4& modelview, 
				const Matrix4& projection, 
				const Vector3& viewer);
	
	void realise();
	void unrealise();

	bool lightingEnabled() const;
	bool lightingSupported() const;
	
	void extensionsInitialised();
	void setLightingEnabled(bool enabled);

	// light culling

	RendererLights m_lights;
	bool m_lightsChanged;
	typedef std::map<LightCullable*, LinearLightList> LightLists;
	LightLists m_lightLists;

	const LightList& attach(LightCullable& cullable);
	void detach(LightCullable& cullable);
	void changed(LightCullable& cullable);
	
    // Attach and detach light sources
	void attachLight(RendererLight& light);
	void detachLight(RendererLight& light);
	void lightChanged(RendererLight& light);

	void evaluateChanged();
	typedef MemberCaller<OpenGLRenderSystem, &OpenGLRenderSystem::evaluateChanged> EvaluateChangedCaller;

	typedef std::set<const Renderable*> Renderables; 
	Renderables m_renderables;
	mutable bool m_traverseRenderablesMutex;

	// Called by OpenGLShader
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

// Accessor method to the Singleton module
OpenGLRenderSystem& getOpenGLRenderSystem();

} // namespace render

#endif /*OPENGLSHADERCACHE_H_*/
