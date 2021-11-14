#include "OpenGLShader.h"

#include "GLProgramFactory.h"
#include "../OpenGLRenderSystem.h"
#include "DepthFillPass.h"

#include "icolourscheme.h"
#include "ishaders.h"
#include "ifilter.h"
#include "irender.h"
#include "texturelib.h"
#include "string/predicate.h"

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
}

// Triplet of diffuse, bump and specular shaders
struct OpenGLShader::DBSTriplet
{
    // DBS layers
    IShaderLayer::Ptr diffuse;
    IShaderLayer::Ptr bump;
    IShaderLayer::Ptr specular;

    // Need-depth-fill flag
    bool needDepthFill;

    // Initialise
    DBSTriplet()
    : needDepthFill(true)
    { }

    // Clear pointers
    void reset()
    {
        diffuse.reset();
        bump.reset();
        specular.reset();
        needDepthFill = false;
    }
};

OpenGLShader::OpenGLShader(const std::string& name, OpenGLRenderSystem& renderSystem) :
    _name(name),
    _renderSystem(renderSystem),
    _isVisible(true),
    _useCount(0)
{
    _windingRenderer.reset(new WindingRenderer<WindingIndexer_Triangles>());
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
    _vertexBuffer.reset();
    _materialChanged.disconnect();
    _material.reset();
    _shaderPasses.clear();
}

void OpenGLShader::addRenderable(const OpenGLRenderable& renderable,
								 const Matrix4& modelview,
								 const LightSources* lights,
                                 const IRenderEntity* entity)
{
    if (!_isVisible) return;

    // Add the renderable to all of our shader passes
    for (const OpenGLShaderPassPtr& pass : _shaderPasses)
    {
        // If the shader pass cares about lighting, submit the renderable once
        // for each incident light
		if (pass->state().testRenderFlag(RENDER_BUMP))
		{
            if (lights)
			{
                lights->forEachLight([&](const RendererLight& light)
                {
                    pass->addRenderable(renderable, modelview, &light, entity);
                });
			}
		}
		else
		{
            // No lighting, submit the renderable once
			pass->addRenderable(renderable, modelview, nullptr, entity);
		}
    }
}

#if 0
void OpenGLShader::addSurface(const std::vector<ArbitraryMeshVertex>& vertices, const std::vector<unsigned int>& indices)
{
    if (!_vertexBuffer)
    {
        _vertexBuffer = std::make_unique<IndexedVertexBuffer<ArbitraryMeshVertex>>();
    }

    // Offset all incoming vertices with a given offset
    auto indexOffset = static_cast<unsigned int>(_vertexBuffer->getNumVertices());

    // Pump the data to the VBO
    _vertexBuffer->addVertices(vertices.begin(), vertices.end());
    _vertexBuffer->addIndicesToLastBatch(indices.begin(), indices.size(), indexOffset);
}
#endif
void OpenGLShader::drawSurfaces()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    
    // Surfaces are using CW culling
    glFrontFace(GL_CW);

    if (hasSurfaces())
    {
        //_vertexBuffer->renderAllBatches(GL_TRIANGLES, false);

        SurfaceRenderer::render();
#if 0
        // Render all triangles

        glBindBuffer(GL_VERTEX_ARRAY, _vbo);

        // Set the vertex pointer first
        glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().vertex);
        glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().normal);
        glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().texcoord);
        //glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().texcoord);
        //glVertexAttribPointer(ATTR_TANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().tangent);
        //glVertexAttribPointer(ATTR_BITANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().bitangent);

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, _indices.data());

        //glDisableClientState(GL_NORMAL_ARRAY);
        //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        //glDisableClientState(GL_VERTEX_ARRAY);

        glBindBuffer(GL_VERTEX_ARRAY, 0);
#endif
    }

    // Render all windings
    glFrontFace(GL_CCW);
    _windingRenderer->renderAllWindings();

#ifdef RENDERABLE_GEOMETRY
    glFrontFace(GL_CW);
    for (auto& geometry: _geometry)
    {
        GLenum mode = GL_TRIANGLES;

        switch (geometry.get().getType())
        {
        case RenderableGeometry::Type::Quads:
            mode = GL_QUADS;
            break;

        case RenderableGeometry::Type::Polygons:
            mode = GL_POLYGON;
            break;
        }

        glVertexPointer(3, GL_DOUBLE, static_cast<GLsizei>(geometry.get().getVertexStride()), &geometry.get().getFirstVertex());
        glDrawElements(mode, static_cast<GLsizei>(geometry.get().getNumIndices()), GL_UNSIGNED_INT, &geometry.get().getFirstIndex());
    }
#endif

    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

#ifdef RENDERABLE_GEOMETRY
void OpenGLShader::clearGeometry()
{
    _geometry.clear();
}
#endif

bool OpenGLShader::hasSurfaces() const
{
    return !SurfaceRenderer::empty() || _vertexBuffer && _vertexBuffer->getNumVertices() > 0;
}

ISurfaceRenderer::Slot OpenGLShader::addSurface(const std::vector<ArbitraryMeshVertex>& vertices,
    const std::vector<unsigned int>& indices)
{
    return SurfaceRenderer::addSurface(vertices, indices);
}

void OpenGLShader::removeSurface(ISurfaceRenderer::Slot slot)
{
    SurfaceRenderer::removeSurface(slot);
}

void OpenGLShader::updateSurface(ISurfaceRenderer::Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
    const std::vector<unsigned int>& indices)
{
    SurfaceRenderer::updateSurface(slot, vertices, indices);
}

IWindingRenderer::Slot OpenGLShader::addWinding(const std::vector<ArbitraryMeshVertex>& vertices)
{
    return _windingRenderer->addWinding(vertices);
}

void OpenGLShader::removeWinding(IWindingRenderer::Slot slot)
{
    _windingRenderer->removeWinding(slot);
}

void OpenGLShader::updateWinding(IWindingRenderer::Slot slot, const std::vector<ArbitraryMeshVertex>& vertices)
{
    _windingRenderer->updateWinding(slot, vertices);
}

bool OpenGLShader::hasWindings() const
{
    return !_windingRenderer->empty();
}

#ifdef RENDERABLE_GEOMETRY
void OpenGLShader::addGeometry(RenderableGeometry& geometry)
{
    _geometry.emplace_back(std::ref(geometry));
}

bool OpenGLShader::hasGeometry() const
{
    return !_geometry.empty();
}
#endif

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
    for (Passes::iterator i = _shaderPasses.begin();
         i != _shaderPasses.end();
          ++i)
    {
    	_renderSystem.insertSortedState(
            OpenGLStates::value_type((*i)->statePtr(), *i)
        );
    }
}

void OpenGLShader::removePasses()
{
    // Remove shader passes from the GL state manager
    for (Passes::iterator i = _shaderPasses.begin();
         i != _shaderPasses.end();
         ++i)
	{
        _renderSystem.eraseSortedState((*i)->statePtr());
    }
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
    auto& pass = _shaderPasses.emplace_back(std::make_shared<DepthFillPass>(*this, _renderSystem));
    return pass->state();
}

// Test if we can render in bump map mode
bool OpenGLShader::canUseLightingMode() const
{
    return _renderSystem.shaderProgramsAvailable() &&
        _renderSystem.getCurrentShaderProgram() == RenderSystem::SHADER_PROGRAM_INTERACTION;
}

void OpenGLShader::setGLTexturesFromTriplet(OpenGLState& pass,
                                            const DBSTriplet& triplet)
{
    // Get texture components. If any of the triplet is missing, look up the
    // default from the shader system.
    if (triplet.diffuse)
    {
        pass.texture0 = getTextureOrInteractionDefault(triplet.diffuse)->getGLTexNum();
		pass.stage0 = triplet.diffuse;
    }
    else
    {
        pass.texture0 = getDefaultInteractionTexture(IShaderLayer::DIFFUSE)->getGLTexNum();
    }

    if (triplet.bump)
    {
        pass.texture1 = getTextureOrInteractionDefault(triplet.bump)->getGLTexNum();
		pass.stage1 = triplet.bump;
    }
    else
    {
        pass.texture1 = getDefaultInteractionTexture(IShaderLayer::BUMP)->getGLTexNum();
    }

    if (triplet.specular)
    {
        pass.texture2 = getTextureOrInteractionDefault(triplet.specular)->getGLTexNum();
		pass.stage2 = triplet.specular;
    }
    else
    {
        pass.texture2 = getDefaultInteractionTexture(IShaderLayer::SPECULAR)->getGLTexNum();
    }
}

// Add an interaction layer
void OpenGLShader::appendInteractionLayer(const DBSTriplet& triplet)
{
	// Set layer vertex colour mode and alphatest parameters
    IShaderLayer::VertexColourMode vcolMode = IShaderLayer::VERTEX_COLOUR_NONE;
    double alphaTest = -1;

    if (triplet.diffuse)
    {
        vcolMode = triplet.diffuse->getVertexColourMode();
        alphaTest = triplet.diffuse->getAlphaTest();
    }

    // Append a depthfill shader pass if requested
    if (triplet.needDepthFill && triplet.diffuse)
    {
        // Create depth-buffer fill pass with alpha test
        OpenGLState& zPass = appendDepthFillPass();

        // Store the alpha test value
        zPass.alphaThreshold = static_cast<GLfloat>(alphaTest);

        // We need a diffuse stage to be able to performthe alpha test
        zPass.stage0 = triplet.diffuse;
        zPass.texture0 = getTextureOrInteractionDefault(triplet.diffuse)->getGLTexNum();
    }

    // Add the DBS pass
    OpenGLState& dbsPass = appendDefaultPass();

    // Populate the textures and remember the stage reference
    setGLTexturesFromTriplet(dbsPass, triplet);

    // Set render flags
    dbsPass.setRenderFlag(RENDER_BLEND);
    dbsPass.setRenderFlag(RENDER_FILL);
    dbsPass.setRenderFlag(RENDER_TEXTURE_2D);
    dbsPass.setRenderFlag(RENDER_CULLFACE);
    dbsPass.setRenderFlag(RENDER_DEPTHTEST);
    dbsPass.setRenderFlag(RENDER_SMOOTH);
    dbsPass.setRenderFlag(RENDER_BUMP);
    dbsPass.setRenderFlag(RENDER_PROGRAM);

    dbsPass.glProgram = _renderSystem.getGLProgramFactory().getBuiltInProgram("bumpMap");

    if (vcolMode != IShaderLayer::VERTEX_COLOUR_NONE)
    {
        // Vertex colours allowed
        dbsPass.setRenderFlag(RENDER_VERTEX_COLOUR);

        if (vcolMode == IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY)
        {
            // Vertex colours are inverted
            dbsPass.setColourInverted(true);
        }
    }

    applyAlphaTestToPass(dbsPass, alphaTest);

	// Apply the diffuse colour modulation
	if (triplet.diffuse)
	{
		dbsPass.setColour(triplet.diffuse->getColour());
	}

    dbsPass.setDepthFunc(GL_LEQUAL);
    dbsPass.polygonOffset = 0.5f;
    dbsPass.setSortPosition(OpenGLState::SORT_INTERACTION);
    dbsPass.m_blend_src = GL_ONE;
    dbsPass.m_blend_dst = GL_ONE;
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
    // Build up and add shader passes for DBS triplets as they are found. A
    // new triplet is found when (1) the same DBS layer type is seen twice, (2)
    // we have at least one DBS layer then see a blend layer, or (3) we have at
    // least one DBS layer then reach the end of the layers.

    DBSTriplet triplet;
    const IShaderLayerVector allLayers = _material->getAllLayers();

    for (const auto& layer : allLayers)
    {
        // Skip programmatically disabled layers
        if (!layer->isEnabled()) continue;

		// Make sure we had at least one evaluation call to fill the material registers
		layer->evaluateExpressions(0);

        switch (layer->getType())
        {
        case IShaderLayer::DIFFUSE:
            if (triplet.diffuse)
            {
                appendInteractionLayer(triplet);
                triplet.reset();
            }
            triplet.diffuse = layer;
            break;

        case IShaderLayer::BUMP:
            if (triplet.bump)
            {
                appendInteractionLayer(triplet);
                triplet.reset();
            }
            triplet.bump = layer;
            break;

        case IShaderLayer::SPECULAR:
            if (triplet.specular)
            {
                appendInteractionLayer(triplet);
                triplet.reset();
            }
            triplet.specular = layer;
            break;

        case IShaderLayer::BLEND:
            if (triplet.specular || triplet.bump || triplet.diffuse)
            {
                appendInteractionLayer(triplet);
                triplet.reset();
            }

            appendBlendLayer(layer);
        }
    }

    // Submit final pass if we reach the end
    if (triplet.specular || triplet.bump || triplet.diffuse)
	{
		appendInteractionLayer(triplet);
	}
}

void OpenGLShader::determineBlendModeForEditorPass(OpenGLState& pass)
{
    bool hasDiffuseLayer = false;

    // Determine alphatest from first diffuse layer
    const IShaderLayerVector allLayers = _material->getAllLayers();

    for (IShaderLayerVector::const_iterator i = allLayers.begin();
         i != allLayers.end();
         ++i)
    {
        const IShaderLayer::Ptr& layer = *i;

        if (layer->getType() == IShaderLayer::DIFFUSE)
        {
            hasDiffuseLayer = true;

            if (layer->getAlphaTest() > 0)
            {
                applyAlphaTestToPass(pass, layer->getAlphaTest());
                break;
            }
        }
    }

    // If this is a purely blend material (no DBS layers), set the editor blend
    // mode from the first blend layer.
	// greebo: Hack to let "shader not found" textures be handled as diffusemaps
    if (!hasDiffuseLayer && !allLayers.empty() && _material->getName() != "_default")
    {
		pass.setRenderFlag(RENDER_BLEND);
		pass.setSortPosition(OpenGLState::SORT_TRANSLUCENT);

		BlendFunc bf = allLayers[0]->getBlendFunc();
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
    determineBlendModeForEditorPass(previewPass);

    // Set the GL color to white
    previewPass.setColour(Colour4::WHITE());

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

    // Get the blend function
    BlendFunc blendFunc = layer->getBlendFunc();
    state.m_blend_src = blendFunc.src;
    state.m_blend_dst = blendFunc.dest;

	// Alpha-tested stages or one-over-zero blends should use the depth buffer
    if (state.m_blend_src == GL_SRC_ALPHA || state.m_blend_dst == GL_SRC_ALPHA ||
		(state.m_blend_src == GL_ONE && state.m_blend_dst == GL_ZERO))
    {
		state.setRenderFlag(RENDER_DEPTHWRITE);
    }

    // Set texture dimensionality (cube map or 2D)
    state.cubeMapMode = layer->getCubeMapMode();
    if (state.cubeMapMode == IShaderLayer::CUBE_MAP_CAMERA)
    {
        state.setRenderFlag(RENDER_TEXTURE_CUBEMAP);
    }
    else
    {
        state.setRenderFlag(RENDER_TEXTURE_2D);
    }

    // Colour modulation
    state.setColour(layer->getColour());

	// Sort position
    if (_material->getSortRequest() >= Material::SORT_DECAL)
    {
        state.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);
    }
    else
    {
        state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
	}

    // Polygon offset
    state.polygonOffset = _material->getPolygonOffset();

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

// Construct a normal shader
void OpenGLShader::constructNormalShader()
{
    // Obtain the Material
    _material = GlobalMaterialManager().getMaterial(_name);
    assert(_material);

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

// Main shader construction entry point
void OpenGLShader::construct()
{
	// Retrieve the highlight colour from the colourschemes (once)
	const static Colour4 highLightColour(
        GlobalColourSchemeManager().getColour("selected_brush_camera"), 0.3f
    );

    // Check the first character of the name to see if this is a special built-in
    // shader
    switch (_name[0])
    {
        case '(': // fill shader
        {
            OpenGLState& state = appendDefaultPass();
			state.setName(_name);

            Colour4 colour;
            sscanf(_name.c_str(), "(%f %f %f)", &colour[0], &colour[1], &colour[2]);
            colour[3] = 1.0f;
            state.setColour(colour);

            state.setRenderFlag(RENDER_FILL);
            state.setRenderFlag(RENDER_LIGHTING);
            state.setRenderFlag(RENDER_DEPTHTEST);
            state.setRenderFlag(RENDER_CULLFACE);
            state.setRenderFlag(RENDER_DEPTHWRITE);
            state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);

            enableViewType(RenderViewType::Camera);
            break;
        }

        case '[':
        {
            OpenGLState& state = appendDefaultPass();
			state.setName(_name);

            Colour4 colour;
            sscanf(_name.c_str(), "[%f %f %f]", &colour[0], &colour[1], &colour[2]);
            colour[3] = 0.5f;
            state.setColour(colour);

            state.setRenderFlag(RENDER_FILL);
            state.setRenderFlag(RENDER_LIGHTING);
            state.setRenderFlag(RENDER_DEPTHTEST);
            state.setRenderFlag(RENDER_CULLFACE);
            state.setRenderFlag(RENDER_DEPTHWRITE);
            state.setRenderFlag(RENDER_BLEND);
            state.setSortPosition(OpenGLState::SORT_TRANSLUCENT);

            enableViewType(RenderViewType::Camera);
            break;
        }

        case '<': // wireframe shader
        {
            // Wireframe renderer is using GL_LINES to display each winding
            _windingRenderer.reset(new WindingRenderer<WindingIndexer_Lines>());

            OpenGLState& state = appendDefaultPass();
			state.setName(_name);

            Colour4 colour;
            sscanf(_name.c_str(), "<%f %f %f>", &colour[0], &colour[1], &colour[2]);
            colour[3] = 1;
            state.setColour(colour);

            state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
            state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
            state.setDepthFunc(GL_LESS);
            state.m_linewidth = 1;
            state.m_pointsize = 1;

            enableViewType(RenderViewType::OrthoView);
            break;
        }

        case '$':
        {
            OpenGLState& state = appendDefaultPass();
			state.setName(_name);

            if (_name == "$POINT")
            {
              state.setRenderFlag(RENDER_POINT_COLOUR);
              state.setRenderFlag(RENDER_DEPTHWRITE);

              state.setSortPosition(OpenGLState::SORT_POINT_FIRST);
              state.m_pointsize = 4;

              enableViewType(RenderViewType::Camera);
              enableViewType(RenderViewType::OrthoView);
            }
            else if (_name == "$SELPOINT")
            {
              state.setRenderFlag(RENDER_POINT_COLOUR);
              state.setRenderFlag(RENDER_DEPTHWRITE);

              state.setSortPosition(OpenGLState::SORT_POINT_LAST);
              state.m_pointsize = 4;

              enableViewType(RenderViewType::Camera);
              enableViewType(RenderViewType::OrthoView);
            }
            else if (_name == "$BIGPOINT")
            {
              state.setRenderFlag(RENDER_POINT_COLOUR);
              state.setRenderFlag(RENDER_DEPTHWRITE);

              state.setSortPosition(OpenGLState::SORT_POINT_FIRST);
              state.m_pointsize = 6;

              enableViewType(RenderViewType::Camera);
              enableViewType(RenderViewType::OrthoView);
            }
            else if (_name == "$PIVOT")
            {
              state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_GUI0);
              state.m_linewidth = 2;
              state.setDepthFunc(GL_LEQUAL);

              OpenGLState& hiddenLine = appendDefaultPass();
			  hiddenLine.setName(_name + "_Hidden");
              hiddenLine.setRenderFlags(RENDER_DEPTHTEST | RENDER_LINESTIPPLE);
              hiddenLine.setSortPosition(OpenGLState::SORT_GUI0);
              hiddenLine.m_linewidth = 2;
              hiddenLine.setDepthFunc(GL_GREATER);

              enableViewType(RenderViewType::Camera);
              enableViewType(RenderViewType::OrthoView);
            }
            else if (_name == "$LATTICE")
            {
              state.setColour(1, 0.5, 0, 1);
              state.setRenderFlag(RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_POINT_FIRST);

              enableViewType(RenderViewType::Camera);
              enableViewType(RenderViewType::OrthoView);
            }
#if 0
            else if (_name == "$WIREFRAME")
            {
              state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
            }
#endif
            else if (_name == "$CAM_HIGHLIGHT")
            {
				// This is the shader drawing a coloured overlay
				// over faces/polys. Its colour is configurable,
				// and it has depth test activated.
				state.setRenderFlag(RENDER_FILL);
				state.setRenderFlag(RENDER_DEPTHTEST);
				state.setRenderFlag(RENDER_CULLFACE);
				state.setRenderFlag(RENDER_BLEND);

				state.setColour(highLightColour);
				state.setSortPosition(OpenGLState::SORT_HIGHLIGHT);
				state.polygonOffset = 0.5f;
				state.setDepthFunc(GL_LEQUAL);

                enableViewType(RenderViewType::Camera);
            }
            else if (_name == "$CAM_OVERLAY")
            {
				// This is the shader drawing a solid line to outline
				// a selected item. The first pass has its depth test
				// activated using GL_LESS, whereas the second pass
				// draws the hidden lines in stippled appearance
				// with its depth test using GL_GREATER.
				state.setRenderFlags(RENDER_OFFSETLINE | RENDER_DEPTHTEST);
				state.setSortPosition(OpenGLState::SORT_OVERLAY_LAST);

				// Second pass for hidden lines
				OpenGLState& hiddenLine = appendDefaultPass();
				hiddenLine.setColour(0.75, 0.75, 0.75, 1);
				hiddenLine.setRenderFlags(RENDER_CULLFACE
					| RENDER_DEPTHTEST
					| RENDER_OFFSETLINE
					| RENDER_LINESTIPPLE);
				hiddenLine.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);
				hiddenLine.setDepthFunc(GL_GREATER);
				hiddenLine.m_linestipple_factor = 2;

                enableViewType(RenderViewType::Camera);
            }
            else if (string::starts_with(_name, "$MERGE_ACTION_"))
            {
                Colour4 colour;
                auto sortPosition = OpenGLState::SORT_OVERLAY_FIRST;
                auto lineSortPosition = OpenGLState::SORT_OVERLAY_LAST;

                if (string::ends_with(_name, "_ADD"))
                {
                    colour = Colour4(0, 0.9f, 0, 0.5f);
                    sortPosition = OpenGLState::SORT_OVERLAY_THIRD; // render additions over removals
                }
                else if (string::ends_with(_name, "_REMOVE"))
                {
                    colour = Colour4(0.6f, 0.1f, 0, 0.5f);
                    lineSortPosition = OpenGLState::SORT_OVERLAY_ONE_BEFORE_LAST;
                }
                else if (string::ends_with(_name, "_CHANGE"))
                {
                    colour = Colour4(0, 0.4f, 0.9f, 0.5f);
                    sortPosition = OpenGLState::SORT_OVERLAY_SECOND;
                }
                else if (string::ends_with(_name, "_CONFLICT"))
                {
                    colour = Colour4(0.9f, 0.5f, 0.0f, 0.5f);
                    sortPosition = OpenGLState::SORT_OVERLAY_ONE_BEFORE_LAST;
                }

                // This is the shader drawing a coloured overlay
                // over faces/polys. Its colour is configurable,
                // and it has depth test activated.
                state.setRenderFlag(RENDER_FILL);
                state.setRenderFlag(RENDER_DEPTHTEST);
                state.setRenderFlag(RENDER_CULLFACE);
                state.setRenderFlag(RENDER_BLEND);

                state.setColour(colour);
                state.setSortPosition(sortPosition);
                state.polygonOffset = 0.5f;
                state.setDepthFunc(GL_LEQUAL);

                // This is the outline pass
                auto& linesOverlay = appendDefaultPass();
                colour[3] = 0.78f;
                linesOverlay.setColour(colour);
                linesOverlay.setRenderFlags(RENDER_OFFSETLINE | RENDER_DEPTHTEST | RENDER_BLEND);
                linesOverlay.setSortPosition(lineSortPosition);

                enableViewType(RenderViewType::Camera);
            }
            else if (_name == "$XY_OVERLAY")
            {
              Vector3 colorSelBrushes = GlobalColourSchemeManager().getColour("selected_brush");
              state.setColour(static_cast<float>(colorSelBrushes[0]),
                              static_cast<float>(colorSelBrushes[1]),
                              static_cast<float>(colorSelBrushes[2]),
                              static_cast<float>(1));
              state.setRenderFlag(RENDER_LINESTIPPLE);
              state.setSortPosition(OpenGLState::SORT_HIGHLIGHT);
              state.m_linewidth = 2;
              state.m_linestipple_factor = 3;

              enableViewType(RenderViewType::OrthoView);
            }
			else if (_name == "$XY_OVERLAY_GROUP")
			{
				Vector3 colorSelBrushes = GlobalColourSchemeManager().getColour("selected_group_items");
                state.setColour(static_cast<float>(colorSelBrushes[0]),
                    static_cast<float>(colorSelBrushes[1]),
                    static_cast<float>(colorSelBrushes[2]),
                    static_cast<float>(1));
				state.setRenderFlag(RENDER_LINESTIPPLE);
				state.setSortPosition(OpenGLState::SORT_HIGHLIGHT);
				state.m_linewidth = 2;
				state.m_linestipple_factor = 3;

                enableViewType(RenderViewType::OrthoView);
			}
            else if (string::starts_with(_name, "$XY_MERGE_ACTION_"))
            {
                Colour4 colour;
                auto sortPosition = OpenGLState::SORT_OVERLAY_FIRST;
                auto lineSortPosition = OpenGLState::SORT_OVERLAY_LAST;

                if (string::ends_with(_name, "_ADD"))
                {
                    colour = Colour4(0, 0.5f, 0, 0.5f);
                    sortPosition = OpenGLState::SORT_OVERLAY_THIRD; // render additions over removals
                }
                else if (string::ends_with(_name, "_REMOVE"))
                {
                    colour = Colour4(0.6f, 0.1f, 0, 0.5f);
                    lineSortPosition = OpenGLState::SORT_OVERLAY_ONE_BEFORE_LAST;
                }
                else if (string::ends_with(_name, "_CHANGE"))
                {
                    colour = Colour4(0, 0.4f, 0.9f, 0.5f);
                    sortPosition = OpenGLState::SORT_OVERLAY_SECOND;
                }
                else if (string::ends_with(_name, "_CONFLICT"))
                {
                    colour = Colour4(0.9f, 0.5f, 0.0f, 0.5f);
                    sortPosition = OpenGLState::SORT_OVERLAY_ONE_BEFORE_LAST;
                }

                state.setColour(colour);
                state.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);
                state.m_linewidth = 2;

                enableViewType(RenderViewType::OrthoView);
            }
            else if (_name == "$XY_INACTIVE_NODE")
            {
                Colour4 colour(0, 0, 0, 0.05f);
                state.setColour(static_cast<float>(colour[0]),
                    static_cast<float>(colour[1]),
                    static_cast<float>(colour[2]),
                    static_cast<float>(colour[3]));

                state.m_blend_src = GL_SRC_ALPHA;
                state.m_blend_dst = GL_ONE_MINUS_SRC_ALPHA;

                state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE | RENDER_BLEND);
                state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
                state.setDepthFunc(GL_LESS);

                state.m_linewidth = 1;
                state.m_pointsize = 1;

                enableViewType(RenderViewType::OrthoView);
            }
#if 0
            else if (_name == "$DEBUG_CLIPPED")
            {
              state.setRenderFlag(RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_LAST);
            }
#endif
            else if (_name == "$POINTFILE")
            {
              state.setColour(1, 0, 0, 1);
              state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
              state.m_linewidth = 4;

              enableViewType(RenderViewType::Camera);
              enableViewType(RenderViewType::OrthoView);
            }
            else if (_name == "$WIRE_OVERLAY")
            {
              state.setRenderFlags(RENDER_DEPTHWRITE
                                 | RENDER_DEPTHTEST
                                 | RENDER_OVERRIDE
								 | RENDER_VERTEX_COLOUR);
              state.setSortPosition(OpenGLState::SORT_GUI1);
              state.setDepthFunc(GL_LEQUAL);

              OpenGLState& hiddenLine = appendDefaultPass();
			  hiddenLine.setName(_name + "_Hidden");
              hiddenLine.setRenderFlags(RENDER_DEPTHWRITE
                                      | RENDER_DEPTHTEST
                                      | RENDER_OVERRIDE
                                      | RENDER_LINESTIPPLE
									  | RENDER_VERTEX_COLOUR);
              hiddenLine.setSortPosition(OpenGLState::SORT_GUI0);
              hiddenLine.setDepthFunc(GL_GREATER);

              enableViewType(RenderViewType::Camera);
              enableViewType(RenderViewType::OrthoView);
            }
            else if (_name == "$FLATSHADE_OVERLAY")
            {
              state.setRenderFlags(RENDER_CULLFACE
                                 | RENDER_LIGHTING
                                 | RENDER_SMOOTH
                                 | RENDER_SCALED
                                 | RENDER_FILL
                                 | RENDER_DEPTHWRITE
                                 | RENDER_DEPTHTEST
                                 | RENDER_OVERRIDE);
              state.setSortPosition(OpenGLState::SORT_GUI1);
              state.setDepthFunc(GL_LEQUAL);

              OpenGLState& hiddenLine = appendDefaultPass();
			  hiddenLine.setName(_name + "_Hidden");
              hiddenLine.setRenderFlags(RENDER_CULLFACE
                                      | RENDER_LIGHTING
                                      | RENDER_SMOOTH
                                      | RENDER_SCALED
                                      | RENDER_FILL
                                      | RENDER_DEPTHWRITE
                                      | RENDER_DEPTHTEST
                                      | RENDER_OVERRIDE
                                      | RENDER_POLYGONSTIPPLE);
              hiddenLine.setSortPosition(OpenGLState::SORT_GUI0);
              hiddenLine.setDepthFunc(GL_GREATER);

              enableViewType(RenderViewType::Camera);
              enableViewType(RenderViewType::OrthoView);
            }
            else if (_name == "$CLIPPER_OVERLAY")
            {
              state.setColour(GlobalColourSchemeManager().getColour("clipper"));
              state.setRenderFlags(RENDER_CULLFACE
                                 | RENDER_DEPTHWRITE
                                 | RENDER_FILL
                                 | RENDER_POLYGONSTIPPLE);
              state.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);

              enableViewType(RenderViewType::Camera);
              enableViewType(RenderViewType::OrthoView);
            }
            else if (_name == "$AAS_AREA")
            {
				state.setColour(1, 1, 1, 1);
				state.setRenderFlags(RENDER_DEPTHWRITE
					| RENDER_DEPTHTEST
					| RENDER_OVERRIDE);
				state.setSortPosition(OpenGLState::SORT_OVERLAY_LAST);
				state.setDepthFunc(GL_LEQUAL);

				OpenGLState& hiddenLine = appendDefaultPass();
				hiddenLine.setColour(1, 1, 1, 1);
				hiddenLine.setRenderFlags(RENDER_DEPTHWRITE
					| RENDER_DEPTHTEST
					| RENDER_OVERRIDE
					| RENDER_LINESTIPPLE);
				hiddenLine.setSortPosition(OpenGLState::SORT_OVERLAY_LAST);
				hiddenLine.setDepthFunc(GL_GREATER);

                enableViewType(RenderViewType::Camera);
            }
            else
            {
                assert(false);
            }
            break;
        } // case '$'

        default:
        {
            // This is not a hard-coded shader, construct from the shader system
            constructNormalShader();

            enableViewType(RenderViewType::Camera);
        }
    } // switch (name[0])
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

}

