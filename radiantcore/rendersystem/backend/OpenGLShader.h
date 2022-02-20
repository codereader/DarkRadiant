#pragma once

#include "OpenGLShaderPass.h"

#include "igeometrystore.h"
#include "irender.h"
#include "ishaders.h"
#include "string/string.h"
#include "render/WindingRenderer.h"
#include "GeometryRenderer.h"
#include "SurfaceRenderer.h"

#include <list>
#include <sigc++/connection.h>

namespace render
{

class OpenGLRenderSystem;

/**
 * Implementation of the Shader class.
 */
class OpenGLShader :
	public Shader
{
private:
    // Name used to construct the shader
    std::string _name;

    // The state manager we will be inserting/removing OpenGL states from
    OpenGLRenderSystem& _renderSystem;

    // List of shader passes for this shader
    std::list<OpenGLShaderPassPtr> _shaderPasses;

    // The depth fill pass of this shader (can be empty).
    // Lighting mode needs to have quick access to this pass
    OpenGLShaderPassPtr _depthFillPass;

    // Interaction pass used by lighting mode
    OpenGLShaderPassPtr _interactionPass;

    // The Material corresponding to this OpenGLShader
	MaterialPtr _material;
    sigc::connection _materialChanged;

    // Visibility flag
    bool _isVisible;

	std::size_t _useCount;

	// Observers attached to this Shader
	typedef std::set<Observer*> Observers;
	Observers _observers;

    std::unique_ptr<IBackendWindingRenderer> _windingRenderer;
    GeometryRenderer _geometryRenderer;
    SurfaceRenderer _surfaceRenderer;

    // Each shader can be used by either camera or orthoview, or both 
    std::size_t _enabledViewTypes;

    bool _mergeModeActive;

private:

    // Construct shader passes from a regular shader (as opposed to a special
    // built-in shader)
    void constructNormalShader();

    void constructFromMaterial(const MaterialPtr& material);

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

    void onMaterialChanged();
    
public:
    /// Construct and initialise
    OpenGLShader(const std::string& name, OpenGLRenderSystem& renderSystem);

    virtual ~OpenGLShader();

    // Returns the owning render system
    OpenGLRenderSystem& getRenderSystem();

    // Shader implementation
    std::string getName() const override { return _name; }
	void addRenderable(const OpenGLRenderable& renderable,
					   const Matrix4& modelview) override;

    bool hasSurfaces() const;
    void drawSurfaces(const VolumeTest& view, const RenderInfo& info);

    IGeometryRenderer::Slot addGeometry(GeometryType indexType,
        const std::vector<ArbitraryMeshVertex>& vertices, const std::vector<unsigned int>& indices) override;
    void removeGeometry(IGeometryRenderer::Slot slot) override;
    void updateGeometry(IGeometryRenderer::Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override;
    void renderGeometry(IGeometryRenderer::Slot slot) override;
    AABB getGeometryBounds(IGeometryRenderer::Slot slot) override;
    IGeometryStore::Slot getGeometryStorageLocation(IGeometryRenderer::Slot slot) override;

    ISurfaceRenderer::Slot addSurface(IRenderableSurface& surface) override;
    void removeSurface(ISurfaceRenderer::Slot slot) override;
    void updateSurface(ISurfaceRenderer::Slot slot) override;
    void renderSurface(ISurfaceRenderer::Slot slot) override;
    IGeometryStore::Slot getSurfaceStorageLocation(ISurfaceRenderer::Slot slot) override;

    IWindingRenderer::Slot addWinding(const std::vector<ArbitraryMeshVertex>& vertices, IRenderEntity* entity) override;
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

	void realise();
	void unrealise();

	// Return the Material used by this shader
    const MaterialPtr& getMaterial() const override;

    unsigned int getFlags() const override;

    bool isApplicableTo(RenderViewType renderViewType) const;
    void enableViewType(RenderViewType renderViewType);

    bool isMergeModeEnabled() const;
    void setMergeModeEnabled(bool enabled);

    void foreachPass(const std::function<void(OpenGLShaderPass&)>& functor);

    // All non-interaction, non-depth fill passes are forwarded to the functor
    void foreachNonInteractionPass(const std::function<void(OpenGLShaderPass&)>& functor);

    // Returns the depth fill pass of this shader, or null if this shader doesn't have one
    OpenGLShaderPass* getDepthFillPass() const;

    // Returns the interaction pass of this shader, or null if this shader doesn't have one
    OpenGLShaderPass* getInteractionPass() const;

protected:
    // Start point for constructing shader passes from the shader name
    virtual void construct();

    // Test if we can render using lighting mode
    bool canUseLightingMode() const;

    // Add a shader pass to the end of the list, and return its state object
    OpenGLState& appendDefaultPass();

    const IBackendWindingRenderer& getWindingRenderer() const;

    // Assign a new winding renderer to this shader (renderer will be move-assigned)
    void setWindingRenderer(std::unique_ptr<IBackendWindingRenderer> renderer);

    // Called when setMergeModeEnabled() switches the flag
    virtual void onMergeModeChanged()
    {}

    // Attaches all passes to the rendersystem
    void insertPasses();

    // Detaches all passes from the rendersystem
    void removePasses();

    // Deletes all passes from this shader
    void clearPasses();

    OpenGLState& appendDepthFillPass();
    OpenGLState& appendInteractionPass();
};

typedef std::shared_ptr<OpenGLShader> OpenGLShaderPtr;

} // namespace

