#pragma once

#include "OpenGLShaderPass.h"

#include "irender.h"
#include "ishaders.h"
#include "string/string.h"

#include <list>
#include <sigc++/connection.h>

namespace render
{

class OpenGLRenderSystem;

/**
 * Implementation of the Shader class.
 */
class OpenGLShader final : 
	public Shader
{
private:
    std::string _name;

    // The state manager we will be inserting/removing OpenGL states from
    OpenGLRenderSystem& _renderSystem;

    // List of shader passes for this shader
	typedef std::list<OpenGLShaderPassPtr> Passes;
	Passes _shaderPasses;

    // Name used to construct the shader. Currently only used for
    // tests/debugging.
    std::string _name;

    // The Material corresponding to this OpenGLShader
	MaterialPtr _material;
    sigc::connection _materialChanged;

    // Visibility flag
    bool _isVisible;

	std::size_t _useCount;

	// Observers attached to this Shader
	typedef std::set<Observer*> Observers;
	Observers _observers;

private:

    // Start point for constructing shader passes from the shader name
	void construct();

    // Construct shader passes from a regular shader (as opposed to a special
    // built-in shader)
    void constructNormalShader();

    // Shader pass construction helpers
    void appendBlendLayer(const IShaderLayer::Ptr& layer);

    struct DBSTriplet;
    void appendInteractionLayer(const DBSTriplet& triplet);

    void constructLightingPassesFromMaterial();
    void determineBlendModeForEditorPass(OpenGLState& pass);
    void constructEditorPreviewPassFromMaterial();
    void applyAlphaTestToPass(OpenGLState& pass, double alphaTest);
    void setGLTexturesFromTriplet(OpenGLState&, const DBSTriplet&);

    // Destroy internal data
	void destroy();

    // Add a shader pass to the end of the list, and return its state object
	OpenGLState& appendDefaultPass();

    // Test if we can render using lighting mode
    bool canUseLightingMode() const;

    void insertPasses();
    void removePasses();

    void onMaterialChanged();
    
public:
    /// Construct and initialise
    OpenGLShader(const std::string& name, OpenGLRenderSystem& renderSystem);

    ~OpenGLShader();

    // Returns the owning render system
    OpenGLRenderSystem& getRenderSystem();

    // Shader implementation
    std::string getName() const override { return _name; }
	void addRenderable(const OpenGLRenderable& renderable,
					   const Matrix4& modelview,
					   const LightSources* lights,
                       const IRenderEntity* entity) override;
    void setVisible(bool visible) override;
    bool isVisible() const override;
    void incrementUsed() override;
    void decrementUsed() override;

	void attachObserver(Observer& observer) override;
	void detachObserver(Observer& observer) override;

	bool isRealised() override;

	/**
	 * Realise this shader
	 */
	void realise();

	void unrealise();

	// Return the Material used by this shader
    const MaterialPtr& getMaterial() const override;

    unsigned int getFlags() const override;
};

typedef std::shared_ptr<OpenGLShader> OpenGLShaderPtr;

} // namespace

