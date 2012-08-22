#include "OpenGLShader.h"

#include "GLProgramFactory.h"
#include "OpenGLShaderPassAdd.h"
#include "render/OpenGLRenderSystem.h"

#include "iuimanager.h"
#include "ishaders.h"
#include "ifilter.h"
#include "irender.h"
#include "texturelib.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

namespace render
{

// Triplet of diffuse, bump and specular shaders
struct OpenGLShader::DBSTriplet
{
    // DBS layers
    ShaderLayerPtr diffuse;
    ShaderLayerPtr bump;
    ShaderLayerPtr specular;

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

void OpenGLShader::destroy()
{
    _material = MaterialPtr();
    _shaderPasses.clear();
}

void OpenGLShader::addRenderable(const OpenGLRenderable& renderable,
								 const Matrix4& modelview,
								 const LightList* lights)
{
    if (!_isVisible) return;

    // Add the renderable to all of our shader passes
    BOOST_FOREACH(OpenGLShaderPassPtr pass, _shaderPasses)
    {
        g_assert(pass);

		if (pass->state().testRenderFlag(RENDER_BUMP))
		{
			if (lights != NULL)
			{
				OpenGLShaderPassAdd add(*pass, renderable, modelview);
				lights->forEachLight(boost::bind(&OpenGLShaderPassAdd::visit, &add, _1));
			}
		}
		else
		{
			pass->addRenderable(renderable, modelview);
		}
    }
}

void OpenGLShader::addRenderable(const OpenGLRenderable& renderable,
								 const Matrix4& modelview,
								 const IRenderEntity& entity,
								 const LightList* lights)
{
    if (!_isVisible) return;

    BOOST_FOREACH(OpenGLShaderPassPtr pass, _shaderPasses)
    {
        if (pass->state().testRenderFlag(RENDER_BUMP))
		{
			if (lights != NULL)
			{
				OpenGLShaderPassAdd add(*pass, renderable, modelview, &entity);
				lights->forEachLight(boost::bind(&OpenGLShaderPassAdd::visit, &add, _1));
			}
		}
		else
		{
			pass->addRenderable(renderable, modelview, entity);
		}
    }
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
    return _isVisible;
}

void OpenGLShader::incrementUsed()
{
    if (++m_used == 1 && _material)
    {
		_material->SetInUse(true);
    }
}

void OpenGLShader::decrementUsed()
{
    if (--m_used == 0 && _material)
    {
		_material->SetInUse(false);
    }
}

void OpenGLShader::realise(const std::string& name)
{
    // Construct the shader passes based on the name
    construct(name);

    if (_material != NULL)
	{
		// greebo: Check the filtersystem whether we're filtered
		_material->setVisible(GlobalFilterSystem().isVisible(FilterRule::TYPE_TEXTURE, name));

		if (m_used != 0)
		{
			_material->SetInUse(true);
		}
    }

    insertPasses();

    m_observers.realise();
}

void OpenGLShader::insertPasses()
{
    // Insert all shader passes into the GL state manager
    for (Passes::iterator i = _shaderPasses.begin();
         i != _shaderPasses.end();
          ++i)
    {
    	_glStateManager.insertSortedState(
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
    	_glStateManager.eraseSortedState((*i)->statePtr());
    }
}

void OpenGLShader::unrealise()
{
    m_observers.unrealise();

    removePasses();

    destroy();
}

unsigned int OpenGLShader::getFlags() const
{
    return _material->getMaterialFlags();
}

// Append a default shader pass onto the back of the state list
OpenGLState& OpenGLShader::appendDefaultPass()
{
    _shaderPasses.push_back(OpenGLShaderPassPtr(new OpenGLShaderPass(*this)));
    OpenGLState& state = _shaderPasses.back()->state();
    return state;
}

// Test if we can render in bump map mode
bool OpenGLShader::canUseLightingMode() const
{
    return (
        GlobalRenderSystem().shaderProgramsAvailable()
    	&& GlobalRenderSystem().getCurrentShaderProgram() 
           == RenderSystem::SHADER_PROGRAM_INTERACTION
    );
}

void OpenGLShader::setGLTexturesFromTriplet(OpenGLState& pass,
                                            const DBSTriplet& triplet)
{
    // Get texture components. If any of the triplet is missing, look up the
    // default from the shader system.
    if (triplet.diffuse)
    {
        pass.texture0 = triplet.diffuse->getTexture()->getGLTexNum();
		pass.stage0 = triplet.diffuse;
    }
    else
    {
        pass.texture0 = GlobalMaterialManager().getDefaultInteractionTexture(
            ShaderLayer::DIFFUSE
        )->getGLTexNum();
    }

    if (triplet.bump)
    {
        pass.texture1 = triplet.bump->getTexture()->getGLTexNum();
		pass.stage1 = triplet.bump;
    }
    else
    {
        pass.texture1 = GlobalMaterialManager().getDefaultInteractionTexture(
            ShaderLayer::BUMP
        )->getGLTexNum();
    }

    if (triplet.specular)
    {
        pass.texture2 = triplet.specular->getTexture()->getGLTexNum();
		pass.stage2 = triplet.specular;
    }
    else
    {
        pass.texture2 = GlobalMaterialManager().getDefaultInteractionTexture(
            ShaderLayer::SPECULAR
        )->getGLTexNum();
    }
}

// Add an interaction layer
void OpenGLShader::appendInteractionLayer(const DBSTriplet& triplet)
{
	// Set layer vertex colour mode and alphatest parameters
    ShaderLayer::VertexColourMode vcolMode = ShaderLayer::VERTEX_COLOUR_NONE;
    double alphaTest = -1;

    if (triplet.diffuse)
    {
        vcolMode = triplet.diffuse->getVertexColourMode();
        alphaTest = triplet.diffuse->getAlphaTest();
    }

    // Append a depthfill shader pass if requested (not applicable for
    // alpha-test materials)
    if (triplet.needDepthFill && alphaTest <= 0.0)
    {
        // Create depth-buffer fill pass
        OpenGLState& zPass = appendDefaultPass();
        zPass.setRenderFlag(RENDER_MASKCOLOUR);
        zPass.setRenderFlag(RENDER_FILL);
        zPass.setRenderFlag(RENDER_CULLFACE);
        zPass.setRenderFlag(RENDER_DEPTHTEST);
        zPass.setRenderFlag(RENDER_DEPTHWRITE);
        zPass.setRenderFlag(RENDER_PROGRAM);

        zPass.setSortPosition(OpenGLState::SORT_ZFILL);

        zPass.glProgram = GLProgramFactory::instance().getProgram("depthFill");
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

    dbsPass.glProgram = GLProgramFactory::instance().getProgram("bumpMap");

    if (vcolMode != ShaderLayer::VERTEX_COLOUR_NONE)
    {
        // Vertex colours allowed
        dbsPass.setRenderFlag(RENDER_VERTEX_COLOUR);

        if (vcolMode == ShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY)
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

void OpenGLShader::applyAlphaTestToPass(OpenGLState& pass, float alphaTest)
{
    if (alphaTest > 0)
    {
        pass.setRenderFlag(RENDER_ALPHATEST);
        pass.alphaFunc = GL_GEQUAL; // alpha >= threshold
        pass.alphaThreshold = alphaTest;
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
    const ShaderLayerVector& allLayers = _material->getAllLayers();

    for (ShaderLayerVector::const_iterator i = allLayers.begin();
         i != allLayers.end();
         ++i)
    {
		// Make sure we had at least one evaluation call to fill the material registers
		(*i)->evaluateExpressions(0);

        switch ((*i)->getType())
        {
        case ShaderLayer::DIFFUSE:
            if (triplet.diffuse)
            {
                appendInteractionLayer(triplet);
                triplet.reset();
            }
            triplet.diffuse = *i;
            break;

        case ShaderLayer::BUMP:
            if (triplet.bump)
            {
                appendInteractionLayer(triplet);
                triplet.reset();
            }
            triplet.bump = *i;
            break;

        case ShaderLayer::SPECULAR:
            if (triplet.specular)
            {
                appendInteractionLayer(triplet);
                triplet.reset();
            }
            triplet.specular = *i;
            break;

        case ShaderLayer::BLEND:
            if (triplet.specular || triplet.bump || triplet.diffuse)
            {
                appendInteractionLayer(triplet);
                triplet.reset();
            }

            appendBlendLayer(*i);
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
    const ShaderLayerVector& allLayers = _material->getAllLayers();

    for (ShaderLayerVector::const_iterator i = allLayers.begin();
         i != allLayers.end();
         ++i)
    {
        const ShaderLayerPtr& layer = *i;

        if (layer->getType() == ShaderLayer::DIFFUSE)
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
    previewPass.texture0 = _material->getEditorImage()->getGLTexNum();

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
void OpenGLShader::appendBlendLayer(const ShaderLayerPtr& layer)
{
    TexturePtr layerTex = layer->getTexture();

    OpenGLState& state = appendDefaultPass();
    state.setRenderFlag(RENDER_FILL);
    state.setRenderFlag(RENDER_BLEND);
    state.setRenderFlag(RENDER_DEPTHTEST);

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
    if (state.cubeMapMode == ShaderLayer::CUBE_MAP_CAMERA)
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
}

// Construct a normal shader
void OpenGLShader::constructNormalShader(const std::string& name)
{
    // Obtain the Material
    _material = GlobalMaterialManager().getMaterialForName(name);
    assert(_material);

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
void OpenGLShader::construct(const std::string& name)
{
	// Retrieve the highlight colour from the colourschemes (once)
	const static Colour4 highLightColour(
        ColourSchemes().getColour("selected_brush_camera"), 0.3f
    );

    // Check the first character of the name to see if this is a special built-in
    // shader
    switch (name[0])
    {
        case '(': // fill shader
        {
            OpenGLState& state = appendDefaultPass();

            Colour4 colour;
            sscanf(name.c_str(), "(%lf %lf %lf)", &colour[0], &colour[1], &colour[2]);
            colour[3] = 1.0f;
            state.setColour(colour);

            state.setRenderFlag(RENDER_FILL);
            state.setRenderFlag(RENDER_LIGHTING);
            state.setRenderFlag(RENDER_DEPTHTEST);
            state.setRenderFlag(RENDER_CULLFACE);
            state.setRenderFlag(RENDER_DEPTHWRITE);
            state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
            break;
        }

        case '[':
        {
            OpenGLState& state = appendDefaultPass();

            Colour4 colour;
            sscanf(name.c_str(), "[%lf %lf %lf]", &colour[0], &colour[1], &colour[2]);
            colour[3] = 0.5f;
            state.setColour(colour);

            state.setRenderFlag(RENDER_FILL);
            state.setRenderFlag(RENDER_LIGHTING);
            state.setRenderFlag(RENDER_DEPTHTEST);
            state.setRenderFlag(RENDER_CULLFACE);
            state.setRenderFlag(RENDER_DEPTHWRITE);
            state.setRenderFlag(RENDER_BLEND);
            state.setSortPosition(OpenGLState::SORT_TRANSLUCENT);
            break;
        }

        case '<': // wireframe shader
        {
            OpenGLState& state = appendDefaultPass();

            Colour4 colour;
            sscanf(name.c_str(), "<%lf %lf %lf>", &colour[0], &colour[1], &colour[2]);
            colour[3] = 1;
            state.setColour(colour);

            state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
            state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
            state.setDepthFunc(GL_LESS);
            state.m_linewidth = 1;
            state.m_pointsize = 1;
            break;
        }

        case '$':
        {
            OpenGLState& state = appendDefaultPass();

            if (name == "$POINT")
            {
              state.setRenderFlag(RENDER_POINT_COLOUR);
              state.setRenderFlag(RENDER_DEPTHWRITE);

              state.setSortPosition(OpenGLState::SORT_POINT_FIRST);
              state.m_pointsize = 4;
            }
            else if (name == "$SELPOINT")
            {
              state.setRenderFlag(RENDER_POINT_COLOUR);
              state.setRenderFlag(RENDER_DEPTHWRITE);

              state.setSortPosition(OpenGLState::SORT_POINT_LAST);
              state.m_pointsize = 4;
            }
            else if (name == "$BIGPOINT")
            {
              state.setRenderFlag(RENDER_POINT_COLOUR);
              state.setRenderFlag(RENDER_DEPTHWRITE);

              state.setSortPosition(OpenGLState::SORT_POINT_FIRST);
              state.m_pointsize = 6;
            }
            else if (name == "$PIVOT")
            {
              state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_GUI0);
              state.m_linewidth = 2;
              state.setDepthFunc(GL_LEQUAL);

              OpenGLState& hiddenLine = appendDefaultPass();
              hiddenLine.setRenderFlags(RENDER_DEPTHTEST | RENDER_LINESTIPPLE);
              hiddenLine.setSortPosition(OpenGLState::SORT_GUI0);
              hiddenLine.m_linewidth = 2;
              hiddenLine.setDepthFunc(GL_GREATER);
            }
            else if (name == "$LATTICE")
            {
              state.setColour(1, 0.5, 0, 1);
              state.setRenderFlag(RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_POINT_FIRST);
            }
            else if (name == "$WIREFRAME")
            {
              state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
            }
            else if (name == "$CAM_HIGHLIGHT")
            {
              state.setRenderFlag(RENDER_FILL);
              state.setRenderFlag(RENDER_DEPTHTEST);
              state.setRenderFlag(RENDER_CULLFACE);
              state.setRenderFlag(RENDER_BLEND);

              state.setColour(highLightColour);
              state.setSortPosition(OpenGLState::SORT_HIGHLIGHT);
              state.polygonOffset = 0.5f;
              state.setDepthFunc(GL_LEQUAL);
            }
            else if (name == "$CAM_OVERLAY")
            {
              state.setRenderFlags(RENDER_CULLFACE
                                 | RENDER_DEPTHTEST
                                 | RENDER_DEPTHWRITE
                                 | RENDER_OFFSETLINE);
              state.setSortPosition(OpenGLState::SORT_OVERLAY_LAST);
              state.setDepthFunc(GL_LEQUAL);

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
            }
            else if (name == "$XY_OVERLAY")
            {
              Vector3 colorSelBrushes = ColourSchemes().getColour("selected_brush");
              state.setColour(colorSelBrushes[0],
                              colorSelBrushes[1],
                              colorSelBrushes[2],
                              1);
              state.setRenderFlag(RENDER_LINESTIPPLE);
              state.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);
              state.m_linewidth = 2;
              state.m_linestipple_factor = 3;
            }
            else if (name == "$DEBUG_CLIPPED")
            {
              state.setRenderFlag(RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_LAST);
            }
            else if (name == "$POINTFILE")
            {
              state.setColour(1, 0, 0, 1);
              state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
              state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
              state.m_linewidth = 4;
            }
            else if (name == "$WIRE_OVERLAY")
            {
              state.setRenderFlags(RENDER_DEPTHWRITE
                                 | RENDER_DEPTHTEST
                                 | RENDER_OVERRIDE 
								 | RENDER_VERTEX_COLOUR);
              state.setSortPosition(OpenGLState::SORT_GUI1);
              state.setDepthFunc(GL_LEQUAL);

              OpenGLState& hiddenLine = appendDefaultPass();
              hiddenLine.setRenderFlags(RENDER_DEPTHWRITE
                                      | RENDER_DEPTHTEST
                                      | RENDER_OVERRIDE
                                      | RENDER_LINESTIPPLE
									  | RENDER_VERTEX_COLOUR);
              hiddenLine.setSortPosition(OpenGLState::SORT_GUI0);
              hiddenLine.setDepthFunc(GL_GREATER);
            }
            else if (name == "$FLATSHADE_OVERLAY")
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
            }
            else if (name == "$CLIPPER_OVERLAY")
            {
              state.setColour(ColourSchemes().getColour("clipper"));
              state.setRenderFlags(RENDER_CULLFACE
                                 | RENDER_DEPTHWRITE
                                 | RENDER_FILL
                                 | RENDER_POLYGONSTIPPLE);
              state.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);
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
            constructNormalShader(name);
        }

    } // switch (name[0])
}

}

