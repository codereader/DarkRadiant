#include "OpenGLShader.h"

#include "GLProgramFactory.h"
#include "../OpenGLRenderSystem.h"
#include "DepthFillPass.h"
#include "InteractionPass.h"

#include "icolourscheme.h"
#include "ishaders.h"
#include "ifilter.h"
#include "irender.h"
#include "texturelib.h"

#include <functional>

namespace render
{

namespace
{
    TexturePtr getDefaultInteractionTexture(IShaderLayer::Type type)
    {
        return GlobalMaterialManager().getDefaultInteractionTexture(type);
    }

    TexturePtr getTextureOrInteractionDefault(const IShaderLayer::Ptr& layer)
    {
        auto texture = layer->getTexture();
        return texture ? texture : getDefaultInteractionTexture(layer->getType());
    }

    IShaderLayer::Ptr findFirstLayerOfType(const MaterialPtr& material, IShaderLayer::Type type)
    {
        IShaderLayer::Ptr found;

        material->foreachLayer([&](const IShaderLayer::Ptr& layer)
        {
            if (layer->getType() == type)
            {
                found = layer;
                return false;
            }

            return true;
        });

        return found;
    }
}

OpenGLShader::OpenGLShader(const std::string& name, OpenGLRenderSystem& renderSystem) :
    _name(name),
    _renderSystem(renderSystem),
    _isVisible(true),
    _useCount(0),
    _geometryRenderer(renderSystem.getGeometryStore(), renderSystem.getObjectRenderer()),
    _surfaceRenderer(renderSystem.getGeometryStore(), renderSystem.getObjectRenderer()),
    _enabledViewTypes(0),
    _mergeModeActive(false)
{
    _windingRenderer.reset(new WindingRenderer<WindingIndexer_Triangles>(
        renderSystem.getGeometryStore(), renderSystem.getObjectRenderer(), this));
}

OpenGLShader::~OpenGLShader()
{
    destroy();
}

OpenGLRenderSystem& OpenGLShader::getRenderSystem()
{
    return _renderSystem;
}

void OpenGLShader::destroy()
{
    _enabledViewTypes = 0;
    _materialChanged.disconnect();
    _material.reset();
    clearPasses();
}

void OpenGLShader::addRenderable(const OpenGLRenderable& renderable,
								 const Matrix4& modelview)
{
    if (!_isVisible) return;

    // Add the renderable to all of our shader passes
    for (const OpenGLShaderPassPtr& pass : _shaderPasses)
    {
        // Submit the renderable to each pass
		pass->addRenderable(renderable, modelview);
    }
}

void OpenGLShader::drawSurfaces(const VolumeTest& view)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    // Always using CW culling by default
    glFrontFace(GL_CW);

    if (hasSurfaces())
    {
        if (supportsVertexColours())
        {
            glEnableClientState(GL_COLOR_ARRAY);
        }
        else
        {
            glDisableClientState(GL_COLOR_ARRAY);
        }

        _geometryRenderer.renderAllVisibleGeometry();

        // Surfaces are not allowed to render vertex colours (for now)
        // otherwise they don't show up in their parent entity's colour
        glDisableClientState(GL_COLOR_ARRAY);
        _surfaceRenderer.render(view);
    }

    // Render all windings (without vertex colours)
    glDisableClientState(GL_COLOR_ARRAY);
    _windingRenderer->renderAllWindings();

    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

bool OpenGLShader::hasSurfaces() const
{
    return !_geometryRenderer.empty() || !_surfaceRenderer.empty();
}

void OpenGLShader::prepareForRendering()
{
    _surfaceRenderer.prepareForRendering();
    _windingRenderer->prepareForRendering();
    // _geometryRenderer doesn't need to prepare at this point
}

IGeometryRenderer::Slot OpenGLShader::addGeometry(GeometryType indexType,
    const std::vector<RenderVertex>& vertices, const std::vector<unsigned int>& indices)
{
    return _geometryRenderer.addGeometry(indexType, vertices, indices);
}

void OpenGLShader::activateGeometry(IGeometryRenderer::Slot slot)
{
    _geometryRenderer.activateGeometry(slot);
}

void OpenGLShader::deactivateGeometry(IGeometryRenderer::Slot slot)
{
    _geometryRenderer.deactivateGeometry(slot);
}

void OpenGLShader::removeGeometry(IGeometryRenderer::Slot slot)
{
    _geometryRenderer.removeGeometry(slot);
}

void OpenGLShader::updateGeometry(IGeometryRenderer::Slot slot, const std::vector<RenderVertex>& vertices,
    const std::vector<unsigned int>& indices)
{
    _geometryRenderer.updateGeometry(slot, vertices, indices);
}

void OpenGLShader::renderAllVisibleGeometry()
{
    _geometryRenderer.renderAllVisibleGeometry();
}

void OpenGLShader::renderGeometry(IGeometryRenderer::Slot slot)
{
    _geometryRenderer.renderGeometry(slot);
}

AABB OpenGLShader::getGeometryBounds(IGeometryRenderer::Slot slot) const
{
    return _geometryRenderer.getGeometryBounds(slot);
}

IGeometryStore::Slot OpenGLShader::getGeometryStorageLocation(IGeometryRenderer::Slot slot)
{
    return _geometryRenderer.getGeometryStorageLocation(slot);
}

ISurfaceRenderer::Slot OpenGLShader::addSurface(IRenderableSurface& surface)
{
    return _surfaceRenderer.addSurface(surface);
}

void OpenGLShader::removeSurface(ISurfaceRenderer::Slot slot)
{
    _surfaceRenderer.removeSurface(slot);
}

void OpenGLShader::updateSurface(ISurfaceRenderer::Slot slot)
{
    _surfaceRenderer.updateSurface(slot);
}

void OpenGLShader::renderSurface(ISurfaceRenderer::Slot slot)
{
    _surfaceRenderer.renderSurface(slot);
}

IGeometryStore::Slot OpenGLShader::getSurfaceStorageLocation(ISurfaceRenderer::Slot slot)
{
    return _surfaceRenderer.getSurfaceStorageLocation(slot);
}

IWindingRenderer::Slot OpenGLShader::addWinding(const std::vector<RenderVertex>& vertices, IRenderEntity* entity)
{
    return _windingRenderer->addWinding(vertices, entity);
}

void OpenGLShader::removeWinding(IWindingRenderer::Slot slot)
{
    _windingRenderer->removeWinding(slot);
}

void OpenGLShader::updateWinding(IWindingRenderer::Slot slot, const std::vector<RenderVertex>& vertices)
{
    _windingRenderer->updateWinding(slot, vertices);
}

bool OpenGLShader::hasWindings() const
{
    return !_windingRenderer->empty();
}

void OpenGLShader::renderWinding(IWindingRenderer::RenderMode mode, IWindingRenderer::Slot slot)
{
    _windingRenderer->renderWinding(mode, slot);
}

void OpenGLShader::setVisible(bool visible)
{
    // Control visibility by inserting or removing our shader passes from the GL
    // state manager
    if (!_isVisible && visible)
    {
        insertPasses();
    }
    else if (_isVisible && !visible)
    {
        removePasses();
    }

    _isVisible = visible;
}

bool OpenGLShader::isVisible() const
{
    return _isVisible && (!_material || _material->isVisible());
}

void OpenGLShader::incrementUsed()
{
    if (++_useCount == 1 && _material)
    {
		_material->SetInUse(true);
    }
}

void OpenGLShader::decrementUsed()
{
    if (--_useCount == 0 && _material)
    {
		_material->SetInUse(false);
    }
}

void OpenGLShader::attachObserver(Observer& observer)
{
	std::pair<Observers::iterator, bool> result = _observers.insert(&observer);

	// Prevent double-attach operations in debug mode
	assert(result.second);

	// Emit the signal immediately if we're in realised state
	if (isRealised())
	{
		observer.onShaderRealised();
	}
}

void OpenGLShader::detachObserver(Observer& observer)
{
	// Emit the signal immediately if we're in realised state
	if (isRealised())
	{
		observer.onShaderUnrealised();
	}

	// Prevent invalid detach operations in debug mode
	assert(_observers.find(&observer) != _observers.end());

	_observers.erase(&observer);
}

bool OpenGLShader::isRealised()
{
    return _material != 0;
}

void OpenGLShader::realise()
{
    // Construct the shader passes based on the name
    construct();

    if (_material)
	{
		// greebo: Check the filtersystem whether we're filtered
		_material->setVisible(GlobalFilterSystem().isVisible(FilterRule::TYPE_TEXTURE, _name));

		if (_useCount != 0)
		{
			_material->SetInUse(true);
		}
    }

    insertPasses();

	for (Observer* observer : _observers)
	{
		observer->onShaderRealised();
	}
}

void OpenGLShader::insertPasses()
{
    // Insert all shader passes into the GL state manager
    for (auto& shaderPass : _shaderPasses)
    {
        if (shaderPass == _depthFillPass) continue; // don't insert the depth fill pass

    	_renderSystem.insertSortedState(std::make_pair(shaderPass->statePtr(), shaderPass));
    }
}

void OpenGLShader::removePasses()
{
    // Remove shader passes from the GL state manager
    for (auto& shaderPass : _shaderPasses)
	{
        if (shaderPass == _depthFillPass) continue; // don't handle the depth fill pass

        _renderSystem.eraseSortedState(shaderPass->statePtr());
    }
}

void OpenGLShader::clearPasses()
{
    _interactionPass.reset();
    _depthFillPass.reset();
    _shaderPasses.clear();
}

void OpenGLShader::unrealise()
{
	for (Observer* observer : _observers)
	{
		observer->onShaderUnrealised();
	}

    removePasses();

    destroy();
}

const MaterialPtr& OpenGLShader::getMaterial() const
{
    return _material;
}

unsigned int OpenGLShader::getFlags() const
{
    return _material->getMaterialFlags();
}

// Append a default shader pass onto the back of the state list
OpenGLState& OpenGLShader::appendDefaultPass()
{
    _shaderPasses.push_back(std::make_shared<OpenGLShaderPass>(*this));
    OpenGLState& state = _shaderPasses.back()->state();
    return state;
}

OpenGLState& OpenGLShader::appendDepthFillPass()
{
    _depthFillPass = std::make_shared<DepthFillPass>(*this, _renderSystem);
    _shaderPasses.push_back(_depthFillPass);

    return _depthFillPass->state();
}

// Test if we can render in bump map mode
bool OpenGLShader::canUseLightingMode() const
{
    return _renderSystem.shaderProgramsAvailable() &&
        _renderSystem.getCurrentShaderProgram() == RenderSystem::SHADER_PROGRAM_INTERACTION;
}

OpenGLState& OpenGLShader::appendInteractionPass(std::vector<IShaderLayer::Ptr>& stages)
{
    // Store all stages in a single interaction pass
    _interactionPass = std::make_shared<InteractionPass>(*this, _renderSystem, stages);
    _shaderPasses.push_back(_interactionPass);

    return _interactionPass->state();
}

void OpenGLShader::applyAlphaTestToPass(OpenGLState& pass, double alphaTest)
{
    if (alphaTest > 0)
    {
        pass.setRenderFlag(RENDER_ALPHATEST);
        pass.alphaFunc = GL_GEQUAL; // alpha >= threshold
        pass.alphaThreshold = static_cast<GLfloat>(alphaTest);
    }
}

// Construct lighting mode render passes
void OpenGLShader::constructLightingPassesFromMaterial()
{
    // Build up and add shader passes for DBS stages similar to the game code.
    // DBS stages are first sorted and stored in the interaction pass where
    // they will be evaluated and grouped to DBS triplets in every frame.
    // All other layers are treated as independent blend layers.

    std::vector<IShaderLayer::Ptr> interactionLayers;
    IShaderLayer::Ptr diffuseForDepthFillPass;

    _material->foreachLayer([&] (const IShaderLayer::Ptr& layer)
    {
        // Skip programmatically disabled layers
        if (!layer->isEnabled()) return true;

		// Make sure we had at least one evaluation call to fill the material registers
		layer->evaluateExpressions(0);

        switch (layer->getType())
        {
        case IShaderLayer::DIFFUSE:
            // Use the diffusemap with the highest opacity for the z-fill pass
            if (!diffuseForDepthFillPass ||
                (diffuseForDepthFillPass->getAlphaTest() != -1 && layer->getAlphaTest() == -1))
            {
                diffuseForDepthFillPass = layer;
            }
            interactionLayers.push_back(layer);
            break;

        case IShaderLayer::BUMP:
        case IShaderLayer::SPECULAR:
            interactionLayers.push_back(layer);
            break;

        case IShaderLayer::BLEND:
            appendBlendLayer(layer);
        }

        return true;
    });

    // Sort interaction stages: bumps go first, then diffuses, speculars last
    std::sort(interactionLayers.begin(), interactionLayers.end(), [](const IShaderLayer::Ptr& a, const IShaderLayer::Ptr& b)
    {
        // Use the enum value to sort stages
        return static_cast<int>(a->getType()) < static_cast<int>(b->getType());
    });

    if (!interactionLayers.empty())
    {
        // Translucent materials don't contribute to the depth buffer
        if (_material->getCoverage() != Material::MC_TRANSLUCENT)
        {
            // Create depth-buffer fill pass, possibly with alpha test
            auto& zPass = appendDepthFillPass();

            zPass.stage0 = diffuseForDepthFillPass;
            zPass.texture0 = diffuseForDepthFillPass ?
                getTextureOrInteractionDefault(diffuseForDepthFillPass)->getGLTexNum() :
                getDefaultInteractionTexture(IShaderLayer::DIFFUSE)->getGLTexNum();
            zPass.alphaThreshold = diffuseForDepthFillPass ? diffuseForDepthFillPass->getAlphaTest() : -1.0f;
        }

        appendInteractionPass(interactionLayers);
    }
}

void OpenGLShader::determineBlendModeForEditorPass(OpenGLState& pass, const IShaderLayer::Ptr& diffuseLayer)
{
    // Determine alphatest from first diffuse layer
    if (diffuseLayer && diffuseLayer->getAlphaTest() > 0)
    {
        applyAlphaTestToPass(pass, diffuseLayer->getAlphaTest());
    }

    // If this is a purely blend material (no DBS layers), set the editor blend
    // mode from the first layer.
	// greebo: Hack to let "shader not found" textures be handled as diffusemaps
    if (!diffuseLayer && _material->getNumLayers() > 0 && _material->getName() != "_default")
    {
		pass.setRenderFlag(RENDER_BLEND);
		pass.setSortPosition(OpenGLState::SORT_TRANSLUCENT);

		BlendFunc bf = _material->getLayer(0)->getBlendFunc();
		pass.m_blend_src = bf.src;
		pass.m_blend_dst = bf.dest;
    }
}

// Construct editor-image-only render passes
void OpenGLShader::constructEditorPreviewPassFromMaterial()
{
    OpenGLState& previewPass = appendDefaultPass();

    // Render the editor texture in legacy mode
    auto editorTex = _material->getEditorImage();
    previewPass.texture0 = editorTex ? editorTex->getGLTexNum() : 0;

    // If there's a diffuse stage's, link it to this shader pass to inherit
    // settings like scale and translate
    previewPass.stage0 = findFirstLayerOfType(_material, IShaderLayer::DIFFUSE);

    // Evaluate the expressions of the diffuse stage once to be able to get a meaningful alphatest value
    if (previewPass.stage0)
    {
        previewPass.stage0->evaluateExpressions(0);
    }

    previewPass.setRenderFlag(RENDER_FILL);
    previewPass.setRenderFlag(RENDER_TEXTURE_2D);
    previewPass.setRenderFlag(RENDER_DEPTHTEST);
    previewPass.setRenderFlag(RENDER_LIGHTING);
    previewPass.setRenderFlag(RENDER_SMOOTH);

	// Don't let translucent materials write to the depth buffer
	if (!(_material->getMaterialFlags() & Material::FLAG_TRANSLUCENT))
	{
		previewPass.setRenderFlag(RENDER_DEPTHWRITE);
	}

    // Handle certain shader flags
	if (_material->getCullType() != Material::CULL_NONE)
    {
        previewPass.setRenderFlag(RENDER_CULLFACE);
    }

    // Set up blend properties
    determineBlendModeForEditorPass(previewPass, previewPass.stage0);

    // Set the GL color to white
    previewPass.setColour(Colour4::WHITE());

    // For the editor preview pass we always ignore the evaluated colour of the material stage
    previewPass.ignoreStageColour = true;

    // Sort position
    if (_material->getSortRequest() >= Material::SORT_DECAL)
    {
        previewPass.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);
    }
    else if (previewPass.getSortPosition() != OpenGLState::SORT_TRANSLUCENT)
    {
        previewPass.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
    }

    // Polygon offset
    previewPass.polygonOffset = _material->getPolygonOffset();
}

// Append a blend (non-interaction) layer
void OpenGLShader::appendBlendLayer(const IShaderLayer::Ptr& layer)
{
    TexturePtr layerTex = layer->getTexture();

    if (!layerTex) return;

    OpenGLState& state = appendDefaultPass();
    state.setRenderFlag(RENDER_FILL);
    state.setRenderFlag(RENDER_BLEND);
    state.setRenderFlag(RENDER_DEPTHTEST);
    state.setDepthFunc(GL_LEQUAL);

	// Remember the stage for later evaluation of shader expressions
	state.stage0 = layer;

    // Set the texture
    state.texture0 = layerTex->getGLTexNum();

    // BlendLights need to load the fall off image into texture unit 1
    if (_material->isBlendLight())
    {
        state.texture1 = _material->lightFalloffImage()->getGLTexNum();
        state.setRenderFlag(RENDER_CULLFACE);
    }

    // Get the blend function
    BlendFunc blendFunc = layer->getBlendFunc();
    state.m_blend_src = blendFunc.src;
    state.m_blend_dst = blendFunc.dest;

    if (_material->getCoverage() == Material::MC_TRANSLUCENT)
    {
        // Material is blending with the background, don't write to the depth buffer
        state.clearRenderFlag(RENDER_DEPTHWRITE);
    }
	// Alpha-tested stages or one-over-zero blends should use the depth buffer
    else if (state.m_blend_src == GL_SRC_ALPHA || state.m_blend_dst == GL_SRC_ALPHA ||
		     (state.m_blend_src == GL_ONE && state.m_blend_dst == GL_ZERO))
    {
		state.setRenderFlag(RENDER_DEPTHWRITE);
    }

    // Set texture dimensionality (cube map or 2D)
    state.cubeMapMode = layer->getCubeMapMode();
    if (state.cubeMapMode == IShaderLayer::CUBE_MAP_CAMERA)
    {
        state.glProgram = _renderSystem.getGLProgramFactory().getBuiltInProgram(ShaderProgram::CubeMap);
        state.setRenderFlag(RENDER_PROGRAM);
        state.setRenderFlag(RENDER_TEXTURE_CUBEMAP);
        state.clearRenderFlag(RENDER_TEXTURE_2D);
    }
    else if (_material && _material->isBlendLight())
    {
        state.glProgram = _renderSystem.getGLProgramFactory().getBuiltInProgram(ShaderProgram::BlendLight);
        state.setRenderFlag(RENDER_TEXTURE_2D);
        state.setRenderFlag(RENDER_PROGRAM);
    }
    else
    {
        state.glProgram = _renderSystem.getGLProgramFactory().getBuiltInProgram(ShaderProgram::RegularStage);
        state.setRenderFlag(RENDER_TEXTURE_2D);
        state.setRenderFlag(RENDER_PROGRAM);
    }

    // Colour modulation
    state.setColour(layer->getColour());
    state.setVertexColourMode(layer->getVertexColourMode());

	// Sort position
    if (_material->getSortRequest() >= Material::SORT_DECAL)
    {
        state.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);
    }
    else
    {
        state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
	}

    // Polygon offset: use the one defined on the material if it has one,
    // otherwise use a sensible default to avoid z-fighting with the depth layer
    if (_material->getMaterialFlags() & Material::FLAG_POLYGONOFFSET)
    {
        state.polygonOffset = _material->getPolygonOffset();
    }
    else if (!(state.getRenderFlags() & RENDER_DEPTHWRITE))
    {
        // #5938: Blending stages seem to z-fight with the result of the depth-buffer
        // apply a slight polygon offset to stop that
        state.polygonOffset = 0.1f;
    }

#if 0
    if (!layer->getVertexProgram().empty() || !layer->getFragmentProgram().empty())
    {
        try
        {
            state.glProgram = _renderSystem.getGLProgramFactory().getProgram(
                layer->getVertexProgram(),
                layer->getFragmentProgram()
            );
        }
        catch (std::runtime_error& ex)
        {
            rError() << "Failed to create GL program for material " <<
                _material->getName() << ": " << ex.what() << std::endl;

            state.glProgram = nullptr;
        }
    }
#endif
}

void OpenGLShader::constructFromMaterial(const MaterialPtr& material)
{
    assert(material);

    _material = material;

    _materialChanged = _material->sig_materialChanged().connect(
        sigc::mem_fun(this, &OpenGLShader::onMaterialChanged));

    // Determine whether we can render this shader in lighting/bump-map mode,
    // and construct the appropriate shader passes
    if (canUseLightingMode())
    {
        // Full lighting, DBS and blend modes
        constructLightingPassesFromMaterial();
    }
    else
    {
        // Editor image rendering only
        constructEditorPreviewPassFromMaterial();
    }
}

void OpenGLShader::construct()
{
    // Construct the shader from the material definition
    constructFromMaterial(GlobalMaterialManager().getMaterial(_name));
    enableViewType(RenderViewType::Camera);
}

void OpenGLShader::onMaterialChanged()
{
    // It's possible that the name of the material got changed, update it
    if (_material && _material->getName() != _name)
    {
        _name = _material->getName();
    }

    unrealise();
    realise();
}

bool OpenGLShader::isApplicableTo(RenderViewType renderViewType) const
{
    return (_enabledViewTypes & static_cast<std::size_t>(renderViewType)) != 0;
}

void OpenGLShader::enableViewType(RenderViewType renderViewType)
{
    _enabledViewTypes |= static_cast<std::size_t>(renderViewType);
}

const IBackendWindingRenderer& OpenGLShader::getWindingRenderer() const
{
    return *_windingRenderer;
}

void OpenGLShader::setWindingRenderer(std::unique_ptr<IBackendWindingRenderer> renderer)
{
    _windingRenderer = std::move(renderer);
}

bool OpenGLShader::isMergeModeEnabled() const
{
    return _mergeModeActive;
}

void OpenGLShader::setMergeModeEnabled(bool enabled)
{
    if (_mergeModeActive == enabled) return;

    _mergeModeActive = enabled;

    onMergeModeChanged();
}

void OpenGLShader::foreachPass(const std::function<void(OpenGLShaderPass&)>& functor)
{
    for (auto& pass : _shaderPasses)
    {
        functor(*pass);
    }
}

void OpenGLShader::foreachNonInteractionPass(const std::function<void(OpenGLShaderPass&)>& functor)
{
    for (auto& pass : _shaderPasses)
    {
        if (pass != _depthFillPass && pass != _interactionPass)
        {
            functor(*pass);
        }
    }
}

DepthFillPass* OpenGLShader::getDepthFillPass() const
{
    return _depthFillPass.get();
}

InteractionPass* OpenGLShader::getInteractionPass() const
{
    return _interactionPass.get();
}

}

