#pragma once

#include "OpenGLShaderPass.h"
#include "OpenGLStateManager.h"

#include "irender.h"
#include "ishaders.h"
#include "moduleobservers.h"
#include "string/string.h"

#include <list>

namespace render
{

/**
 * Implementation of the Shader class.
 */
class OpenGLShader
: public Shader
{
    // The state manager we will be inserting/removing OpenGL states from (this
    // will be the OpenGLRenderSystem).
	OpenGLStateManager& _glStateManager;

    // List of shader passes for this shader
	typedef std::list<OpenGLShaderPassPtr> Passes;
	Passes _shaderPasses;

    // The Material corresponding to this OpenGLShader
	MaterialPtr _material;

    // Visibility flag
    bool _isVisible;

	std::size_t _useCount;
	ModuleObservers m_observers;

private:

    // Start point for constructing shader passes from the shader name
	void construct(const std::string& name);

    // Construct shader passes from a regular shader (as opposed to a special
    // built-in shader)
    void constructNormalShader(const std::string& name);

    // Shader pass construction helpers
    void appendBlendLayer(const ShaderLayerPtr& layer);

    struct DBSTriplet;
    void appendInteractionLayer(const DBSTriplet& triplet);

    void constructLightingPassesFromMaterial();
    void determineBlendModeForEditorPass(OpenGLState& pass);
    void constructEditorPreviewPassFromMaterial();
    void applyAlphaTestToPass(OpenGLState& pass, float alphaTest);
    void setGLTexturesFromTriplet(OpenGLState&, const DBSTriplet&);

    // Destroy internal data
	void destroy();

    // Add a shader pass to the end of the list, and return its state object
	OpenGLState& appendDefaultPass();

    // Test if we can render using lighting mode
    bool canUseLightingMode() const;

    void insertPasses();
    void removePasses();
    
    bool realised() const;

public:

    /// Construct and initialise
    OpenGLShader(OpenGLStateManager& glStateManager);

    // Shader implementation
	void addRenderable(const OpenGLRenderable& renderable,
					   const Matrix4& modelview,
					   const LightList* lights) override;

	void addRenderable(const OpenGLRenderable& renderable,
					   const Matrix4& modelview,
					   const IRenderEntity& entity,
					   const LightList* lights) override;

    void setVisible(bool visible) override;
    bool isVisible() const override;
    void incrementUsed() override;
    void decrementUsed() override;

    void attach(ModuleObserver& observer) override;
    void detach(ModuleObserver& observer) override;

	/**
	 * Realise this shader, setting the name in the process.
	 */
	void realise(const std::string& name);

	void unrealise();

	// Return the Material used by this shader
    const MaterialPtr& getMaterial() const override;

    unsigned int getFlags() const override;
};

typedef std::shared_ptr<OpenGLShader> OpenGLShaderPtr;

} // namespace

