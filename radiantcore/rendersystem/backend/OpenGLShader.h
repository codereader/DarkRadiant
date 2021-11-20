#pragma once

#include "OpenGLShaderPass.h"

#include "irender.h"
#include "ishaders.h"
#include "string/string.h"
#include "render/IndexedVertexBuffer.h"
#include "render/WindingRenderer.h"
#include "SurfaceRenderer.h"

#include <list>
#include <sigc++/connection.h>

namespace render
{

class OpenGLRenderSystem;

/**
 * Implementation of the Shader class.
 */
class OpenGLShader final : 
	public Shader,
    protected SurfaceRenderer
{
private:
    // Name used to construct the shader
    std::string _name;

    // The state manager we will be inserting/removing OpenGL states from
    OpenGLRenderSystem& _renderSystem;

    // List of shader passes for this shader
	typedef std::list<OpenGLShaderPassPtr> Passes;
	Passes _shaderPasses;

    // The Material corresponding to this OpenGLShader
	MaterialPtr _material;
    sigc::connection _materialChanged;

    // Visibility flag
    bool _isVisible;

	std::size_t _useCount;

	// Observers attached to this Shader
	typedef std::set<Observer*> Observers;
	Observers _observers;

    std::unique_ptr<render::IndexedVertexBuffer<ArbitraryMeshVertex>> _vertexBuffer;

#ifdef RENDERABLE_GEOMETRY
    std::vector<std::reference_wrapper<RenderableGeometry>> _geometry;
#endif

    std::unique_ptr<IBackendWindingRenderer> _windingRenderer;

    // Each shader can be used by either camera or orthoview, or both 
    std::size_t _enabledViewTypes;

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
	OpenGLState& appendDepthFillPass();

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

    //void addSurface(const std::vector<ArbitraryMeshVertex>& vertices, const std::vector<unsigned int>& indices) override;
    bool hasSurfaces() const;
    void drawSurfaces();

    ISurfaceRenderer::Slot addSurface(SurfaceIndexingType indexType, 
        const std::vector<ArbitraryMeshVertex>& vertices, const std::vector<unsigned int>& indices) override;
    void removeSurface(ISurfaceRenderer::Slot slot) override;
    void updateSurface(ISurfaceRenderer::Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override;
    void renderSurface(ISurfaceRenderer::Slot slot) override;
#ifdef RENDERABLE_GEOMETRY
    void addGeometry(RenderableGeometry& geometry) override;
    bool hasGeometry() const;
    void clearGeometry();
#endif

    IWindingRenderer::Slot addWinding(const std::vector<ArbitraryMeshVertex>& vertices) override;
    void removeWinding(IWindingRenderer::Slot slot) override;
    void updateWinding(IWindingRenderer::Slot slot, const std::vector<ArbitraryMeshVertex>& vertices) override;
    bool hasWindings() const;
    void renderWinding(IWindingRenderer::RenderMode mode, IWindingRenderer::Slot slot) override;

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

    bool isApplicableTo(RenderViewType renderViewType) const;
    void enableViewType(RenderViewType renderViewType);
};

typedef std::shared_ptr<OpenGLShader> OpenGLShaderPtr;

} // namespace

