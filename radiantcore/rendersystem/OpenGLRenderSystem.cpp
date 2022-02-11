#include "OpenGLRenderSystem.h"

#include "ishaders.h"
#include "igl.h"
#include "itextstream.h"
#include "iradiant.h"

#include "math/Matrix4.h"
#include "module/StaticModule.h"
#include "backend/GLProgramFactory.h"
#include "backend/BuiltInShader.h"
#include "backend/ColourShader.h"
#include "backend/LightInteractions.h"
#include "debugging/debugging.h"
#include "LightingModeRenderResult.h"

#include <functional>

namespace render {

namespace {
    // Polygon stipple pattern
    const GLubyte POLYGON_STIPPLE_PATTERN[132] = {
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
          0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55
    };
}

/**
 * Main constructor.
 */
OpenGLRenderSystem::OpenGLRenderSystem() :
    _realised(false),
    _shaderProgramsAvailable(false),
    _glProgramFactory(std::make_shared<GLProgramFactory>()),
    _currentShaderProgram(SHADER_PROGRAM_NONE),
    _time(0),
    m_traverseRenderablesMutex(false)
{
    bool shouldRealise = false;

    // For the static default rendersystem, the MaterialManager is not existent yet,
    // hence it will be attached in initialiseModule().
    if (module::GlobalModuleRegistry().moduleExists(MODULE_SHADERSYSTEM))
    {
        _materialDefsLoaded = GlobalMaterialManager().signal_DefsLoaded().connect(
            sigc::mem_fun(*this, &OpenGLRenderSystem::realise));
        _materialDefsUnloaded = GlobalMaterialManager().signal_DefsUnloaded().connect(
            sigc::mem_fun(*this, &OpenGLRenderSystem::unrealise));

        if (GlobalMaterialManager().isRealised())
        {
            // Hold back with the realise() call until we know whether we can call 
            // extensionsInitialised() below - this should happen before realise()
            shouldRealise = true;
        }
    }

    // If the openGL module is already initialised and a shared context is created
    // trigger a call to extensionsInitialised().
    if (module::GlobalModuleRegistry().moduleExists(MODULE_SHARED_GL_CONTEXT) &&
        GlobalOpenGLContext().getSharedContext())
    {
        extensionsInitialised();
    }

    if (shouldRealise)
    {
        realise();
    }
}

OpenGLRenderSystem::~OpenGLRenderSystem()
{
    _materialDefsLoaded.disconnect();
    _materialDefsUnloaded.disconnect();
}

ITextRenderer::Ptr OpenGLRenderSystem::captureTextRenderer(IGLFont::Style style, std::size_t size)
{
    // Try to find an existing text renderer with this combination
    auto fontKey = std::make_pair(style, size);

    auto existing = _textRenderers.find(fontKey);

    if (existing == _textRenderers.end())
    {
        auto font = GlobalOpenGL().getFont(fontKey.first, fontKey.second);
        existing = _textRenderers.emplace(fontKey, std::make_shared<TextRenderer>(font)).first;
    }

    return existing->second;
}

ShaderPtr OpenGLRenderSystem::capture(const std::string& name, const std::function<OpenGLShaderPtr()>& createShader)
{
    // Usual ritual, check cache and return if found, otherwise create/
    // insert/return.
    auto existing = _shaders.find(name);

    if (existing != _shaders.end())
    {
        return existing->second;
    }

    // Either the shader was not found, or the weak pointer failed to lock
    // because the shader had been deleted. Either way, create a new shader
    // using the given factory functor and insert into the cache.
    auto shader = createShader();
    _shaders[name] = shader;

    // Realise the shader if the cache is realised
    if (_realised)
    {
        shader->realise();
    }

    return shader;
}

ShaderPtr OpenGLRenderSystem::capture(const std::string& name)
{
    // Forward to the method accepting our factory function
    return capture(name, [&]()
    { 
        return std::make_shared<OpenGLShader>(name, *this); 
    });
}

ShaderPtr OpenGLRenderSystem::capture(BuiltInShaderType type)
{
    // Forward to the method accepting our factory function
    auto name = BuiltInShader::GetNameForType(type);

    return capture(name, [&]()
    {
        return std::make_shared<BuiltInShader>(type, *this);
    });
}

ShaderPtr OpenGLRenderSystem::capture(ColourShaderType type, const Colour4& colour)
{
    // Forward to the method accepting our factory function
    auto name = ColourShader::ConstructName(type, colour);

    return capture(name, [&]()
    {
        return std::make_shared<ColourShader>(type, colour, *this);
    });
}

void OpenGLRenderSystem::beginRendering(OpenGLState& state)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // global settings that are not set in renderstates
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glPolygonOffset(-1, 1);

    // Set polygon stipple pattern from constant
    glPolygonStipple(POLYGON_STIPPLE_PATTERN);

    glEnableClientState(GL_VERTEX_ARRAY);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    if (GLEW_VERSION_1_3) {
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }

    if (GLEW_ARB_shader_objects) {
        glUseProgramObjectARB(0);
        glDisableVertexAttribArrayARB(c_attr_TexCoord0);
        glDisableVertexAttribArrayARB(c_attr_Tangent);
        glDisableVertexAttribArrayARB(c_attr_Binormal);
    }

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set up initial GL state. This MUST MATCH the defaults in the OpenGLState
    // object, otherwise required state changes may not occur.
    glLineStipple(state.m_linestipple_factor,
        state.m_linestipple_pattern);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisable(GL_BLEND);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);
    glDisable(GL_DEPTH_TEST);

    // RENDER_DEPTHWRITE defaults to 0
    glDepthMask(GL_FALSE);

    // RENDER_MASKCOLOUR defaults to 0
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDisable(GL_ALPHA_TEST);

    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_POLYGON_STIPPLE);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glDisable(GL_POLYGON_OFFSET_FILL); // greebo: otherwise tiny gap lines between brushes are visible

    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(1, 1, 1, 1);
    glDepthFunc(state.getDepthFunc());
    glAlphaFunc(GL_ALWAYS, 0);
    glLineWidth(1);
    glPointSize(1);

    glHint(GL_FOG_HINT, GL_NICEST);
    glDisable(GL_FOG);
}

void OpenGLRenderSystem::setupViewMatrices(const Matrix4& modelview, const Matrix4& projection)
{
    // Set the projection and modelview matrices
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projection);

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(modelview);
}

void OpenGLRenderSystem::finishRendering()
{
    if (GLEW_ARB_shader_objects)
    {
        glUseProgramObjectARB(0);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glPopAttrib();
}

/*
 * Render all states in the ShaderCache along with their renderables. This
 * is where the actual OpenGL rendering starts.
 */
void OpenGLRenderSystem::render(RenderViewType renderViewType, 
                                RenderStateFlags globalstate,
                                const Matrix4& modelview,
                                const Matrix4& projection,
                                const Vector3& viewer,
                                const VolumeTest& view)
{
    // Construct default OpenGL state
    OpenGLState current;
    beginRendering(current);

    setupViewMatrices(modelview, projection);

    // Iterate over the sorted mapping between OpenGLStates and their
    // OpenGLShaderPasses (containing the renderable geometry), and render the
    // contents of each bucket. Each pass is passed a reference to the "current"
    // state, which it can change.
    for (const auto& pair : _state_sorted)
    {
        // Render the OpenGLShaderPass
        if (pair.second->empty()) continue;

        if (pair.second->isApplicableTo(renderViewType))
        {
            pair.second->render(current, globalstate, viewer, view, _time);
        }

        pair.second->clearRenderables();
    }

    renderText();

    finishRendering();
}

void OpenGLRenderSystem::startFrame()
{
    // Prepare the storage objects
    _geometryStore.onFrameStart();
}

void OpenGLRenderSystem::endFrame()
{
    _geometryStore.onFrameFinished();
}

IRenderResult::Ptr OpenGLRenderSystem::renderLitScene(RenderStateFlags globalFlagsMask,
    const IRenderView& view)
{
    auto result = std::make_shared<LightingModeRenderResult>();

    // Construct default OpenGL state
    OpenGLState current;
    beginRendering(current);
    setupViewMatrices(view.GetModelview(), view.GetProjection());

    std::size_t visibleLights = 0;
    std::vector<LightInteractions> interactionLists;
    interactionLists.reserve(_lights.size());

    // Gather all visible lights and render the surfaces touched by them
    for (const auto& light : _lights)
    {
        LightInteractions interaction(*light, _geometryStore);

        if (!interaction.isInView(view))
        {
            result->skippedLights++;
            continue;
        }

        result->visibleLights++;

        // Check all the surfaces that are touching this light
        interaction.collectSurfaces(_entities);

        result->objects += interaction.getObjectCount();
        result->entities += interaction.getEntityCount();
        
        interactionLists.emplace_back(std::move(interaction));
    }

    // Run the depth fill pass
    for (auto& interactionList : interactionLists)
    {
        interactionList.fillDepthBuffer(current, globalFlagsMask, view, _time);
    }

    // Draw the surfaces per light and material
    for (auto& interactionList : interactionLists)
    {
        interactionList.render(current, globalFlagsMask, view, _time);
        result->drawCalls += interactionList.getDrawCalls();
    }

    renderText();

    finishRendering();

    return result;
}

void OpenGLRenderSystem::renderText()
{
    // Render all text
    glDisable(GL_DEPTH_TEST);

    for (const auto& [_, textRenderer] : _textRenderers)
    {
        textRenderer->render();
    }
}

void OpenGLRenderSystem::realise()
{
    if (_realised) {
        return; // already realised
    }

    _realised = true;

    if (shaderProgramsAvailable() && getCurrentShaderProgram() != SHADER_PROGRAM_NONE)
    {
        // Realise the GLPrograms
        _glProgramFactory->realise();
    }

    // Realise all shaders
    for (auto& [_, shader] : _shaders)
    {
        shader->realise();
    }
}

void OpenGLRenderSystem::unrealise()
{
    if (!_realised) {
        return;
    }

    _realised = false;

    // Unrealise all OpenGLShader objects
    for (auto& [_, shader] : _shaders)
    {
        shader->unrealise();
    }

	if (GlobalOpenGLContext().getSharedContext() && 
        shaderProgramsAvailable() && 
        getCurrentShaderProgram() != SHADER_PROGRAM_NONE)
    {
        // Unrealise the GLPrograms
        _glProgramFactory->unrealise();
    }
}

GLProgramFactory& OpenGLRenderSystem::getGLProgramFactory()
{
    return *_glProgramFactory;
}

std::size_t OpenGLRenderSystem::getTime() const
{
    return _time;
}

void OpenGLRenderSystem::setTime(std::size_t milliSeconds)
{
    _time = milliSeconds;
}

RenderSystem::ShaderProgram OpenGLRenderSystem::getCurrentShaderProgram() const
{
    return _currentShaderProgram;
}

void OpenGLRenderSystem::setShaderProgram(RenderSystem::ShaderProgram newProg)
{
    ShaderProgram oldProgram = _currentShaderProgram;

    if (oldProgram != newProg)
    {
        unrealise();
        GlobalMaterialManager().setLightingEnabled(
            newProg == SHADER_PROGRAM_INTERACTION
        );
    }

    _currentShaderProgram = newProg;

    if (oldProgram != newProg)
    {
        realise();
    }
}

void OpenGLRenderSystem::extensionsInitialised()
{
    // Determine if lighting is available based on GL extensions
    bool haveGLSL = GLEW_VERSION_2_0 ? true : false;

#if defined(DEBUG_NO_LIGHTING)
    haveGLSL = false;
#endif

    rMessage() << "[OpenGLRenderSystem] GLSL shading "
               << (haveGLSL ? "IS" : "IS NOT" ) << " available.\n";

    // Set the flag in the openGL module
    setShaderProgramsAvailable(haveGLSL);

    // Inform the user of missing extensions
    if (!haveGLSL)
    {
        rWarning() << "Light rendering requires OpenGL 2.0 or newer.\n";
    }

    // Now that GL extensions are done, we can realise our shaders
    // This was previously done explicitly by the OpenGLModule after the
    // shared context was created. But we need realised shaders before
    // we can fire off the "extensions initialised" signal, map loading
    // code might rely on materials being constructed.
    realise();

    // Notify all our observers
    _sigExtensionsInitialised();
}

sigc::signal<void> OpenGLRenderSystem::signal_extensionsInitialised()
{
    return _sigExtensionsInitialised;
}

bool OpenGLRenderSystem::shaderProgramsAvailable() const
{
    return _shaderProgramsAvailable;
}

void OpenGLRenderSystem::setShaderProgramsAvailable(bool available)
{
    _shaderProgramsAvailable = available;
}

void OpenGLRenderSystem::insertSortedState(const OpenGLStates::value_type& val) {
    _state_sorted.insert(val);
}

void OpenGLRenderSystem::eraseSortedState(const OpenGLStates::key_type& key) {
    _state_sorted.erase(key);
}

// renderables
void OpenGLRenderSystem::attachRenderable(Renderable& renderable) {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "attaching renderable during traversal");
    ASSERT_MESSAGE(m_renderables.find(&renderable) == m_renderables.end(), "renderable could not be attached");
    m_renderables.insert(&renderable);
}

void OpenGLRenderSystem::detachRenderable(Renderable& renderable) {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "detaching renderable during traversal");
    ASSERT_MESSAGE(m_renderables.find(&renderable) != m_renderables.end(), "renderable could not be detached");
    m_renderables.erase(&renderable);
}

void OpenGLRenderSystem::forEachRenderable(const RenderableCallback& callback) const {
    ASSERT_MESSAGE(!m_traverseRenderablesMutex, "for-each during traversal");
    m_traverseRenderablesMutex = true;
    for (Renderables::iterator i = m_renderables.begin(); i != m_renderables.end(); ++i) {
        callback(*(*i));
    }
    m_traverseRenderablesMutex = false;
}

void OpenGLRenderSystem::setMergeModeEnabled(bool enabled)
{
    for (auto& [_, shader] : _shaders)
    {
        shader->setMergeModeEnabled(enabled);
    }
}

// RegisterableModule implementation
const std::string& OpenGLRenderSystem::getName() const
{
    static std::string _name(MODULE_RENDERSYSTEM);
    return _name;
}

const StringSet& OpenGLRenderSystem::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_SHADERSYSTEM,
        MODULE_SHARED_GL_CONTEXT,
    };

    return _dependencies;
}

void OpenGLRenderSystem::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    _materialDefsLoaded = GlobalMaterialManager().signal_DefsLoaded().connect(
        sigc::mem_fun(*this, &OpenGLRenderSystem::realise));
    _materialDefsUnloaded = GlobalMaterialManager().signal_DefsUnloaded().connect(
        sigc::mem_fun(*this, &OpenGLRenderSystem::unrealise));

    if (GlobalMaterialManager().isRealised())
    {
        realise();
    }

    // greebo: Don't realise the module yet, this must wait
    // until the shared GL context has been created (this
    // happens as soon as the first GL widget has been realised).
    _sharedContextCreated = GlobalOpenGLContext().signal_sharedContextCreated()
        .connect(sigc::mem_fun(this, &OpenGLRenderSystem::extensionsInitialised));

    _sharedContextDestroyed = GlobalOpenGLContext().signal_sharedContextDestroyed()
        .connect(sigc::mem_fun(this, &OpenGLRenderSystem::unrealise));
}

void OpenGLRenderSystem::shutdownModule()
{
    _entities.clear();
    _lights.clear();

    _textRenderers.clear();

    _sharedContextCreated.disconnect();
    _sharedContextDestroyed.disconnect();
	_materialDefsLoaded.disconnect();
	_materialDefsUnloaded.disconnect();
}

void OpenGLRenderSystem::addEntity(const IRenderEntityPtr& renderEntity)
{
    assert(renderEntity);

    if (!_entities.insert(renderEntity).second)
    {
        throw std::logic_error("Duplicate entity registration.");
    }

    auto light = std::dynamic_pointer_cast<RendererLight>(renderEntity);

    if (!light) return;
     
    if (!_lights.insert(light).second)
    {
        throw std::logic_error("Duplicate light registration.");
    }
}

void OpenGLRenderSystem::removeEntity(const IRenderEntityPtr& renderEntity)
{
    if (_entities.erase(renderEntity) == 0)
    {
        throw std::logic_error("Entity has not been registered.");
    }

    auto light = std::dynamic_pointer_cast<RendererLight>(renderEntity);

    if (!light) return;

    if (_lights.erase(light) == 0)
    {
        throw std::logic_error("Light has not been registered.");
    }
}

void OpenGLRenderSystem::foreachEntity(const std::function<void(const IRenderEntityPtr&)>& functor)
{
    std::for_each(_entities.begin(), _entities.end(), functor);
}

void OpenGLRenderSystem::foreachLight(const std::function<void(const RendererLightPtr&)>& functor)
{
    std::for_each(_lights.begin(), _lights.end(), functor);
}

IGeometryStore& OpenGLRenderSystem::getGeometryStore()
{
    return _geometryStore;
}

// Define the static OpenGLRenderSystem module
module::StaticModule<OpenGLRenderSystem> openGLRenderSystemModule;

} // namespace render
