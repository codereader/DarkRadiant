#pragma once

#include "OpenGLShaderPass.h"

#include "igeometrystore.h"
#include "irender.h"
#include "ishaders.h"
#include "string/string.h"
#include "render/WindingRenderer.h"
#include "GeometryRenderer.h"
#include "SurfaceRenderer.h"
#include "DepthFillPass.h"
#include "InteractionPass.h"

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
    std::shared_ptr<DepthFillPass> _depthFillPass;

    // Interaction pass used by lighting mode
    std::shared_ptr<InteractionPass> _interactionPass;

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

    void constructFromMaterial(const MaterialPtr& material);

    // Shader pass construction helpers
    void appendBlendLayer(const IShaderLayer::Ptr& layer);

    OpenGLState& appendInteractionPass(std::vector<IShaderLayer::Ptr>& stages);

    void constructLightingPassesFromMaterial();
    void determineBlendModeForEditorPass(OpenGLState& pass, const IShaderLayer::Ptr& diffuseLayer);
    void constructEditorPreviewPassFromMaterial();
    void applyAlphaTestToPass(OpenGLState& pass, double alphaTest);

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
    void drawSurfaces(const VolumeTest& view);
    void prepareForRendering();

    IGeometryRenderer::Slot addGeometry(GeometryType indexType,
        const std::vector<RenderVertex>& vertices, const std::vector<unsigned int>& indices) override;
    void activateGeometry(IGeometryRenderer::Slot slot) override;
    void deactivateGeometry(IGeometryRenderer::Slot slot) override;
    void removeGeometry(IGeometryRenderer::Slot slot) override;
    void updateGeometry(IGeometryRenderer::Slot slot, const std::vector<RenderVertex>& vertices,
        const std::vector<unsigned int>& indices) override;
    void renderAllVisibleGeometry() override;
    void renderGeometry(IGeometryRenderer::Slot slot) override;
    AABB getGeometryBounds(IGeometryRenderer::Slot slot) override;
    IGeometryStore::Slot getGeometryStorageLocation(IGeometryRenderer::Slot slot) override;

    ISurfaceRenderer::Slot addSurface(IRenderableSurface& surface) override;
    void removeSurface(ISurfaceRenderer::Slot slot) override;
    void updateSurface(ISurfaceRenderer::Slot slot) override;
    void renderSurface(ISurfaceRenderer::Slot slot) override;
    IGeometryStore::Slot getSurfaceStorageLocation(ISurfaceRenderer::Slot slot) override;

    IWindingRenderer::Slot addWinding(const std::vector<RenderVertex>& vertices, IRenderEntity* entity) override;
    void removeWinding(IWindingRenderer::Slot slot) override;
    void updateWinding(IWindingRenderer::Slot slot, const std::vector<RenderVertex>& vertices) override;
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
    DepthFillPass* getDepthFillPass() const;

    // Returns the interaction pass of this shader, or null if this shader doesn't have one
    InteractionPass* getInteractionPass() const;

protected:
    // Start point for constructing shader passes from the shader name
    virtual void construct();

    // Test if we can render using lighting mode
    bool canUseLightingMode() const;

    // Whether any surfaces or geometries should submit colours
    virtual bool supportsVertexColours() const
    {
        return true;
    }

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
};

typedef std::shared_ptr<OpenGLShader> OpenGLShaderPtr;

} // namespace

