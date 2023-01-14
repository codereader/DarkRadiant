#include "OpenGLRenderSystem.h"

#include "ishaders.h"
#include "igl.h"
#include "itextstream.h"
#include "iregistry.h"
#include "iradiant.h"
#include "icolourscheme.h"
#include "ideclmanager.h"

#include "math/Matrix4.h"
#include "module/StaticModule.h"
#include "backend/GLProgramFactory.h"
#include "backend/BuiltInShader.h"
#include "backend/ColourShader.h"
#include "backend/LightingModeRenderer.h"
#include "backend/FullBrightRenderer.h"
#include "backend/ObjectRenderer.h"
#include "debugging/debugging.h"

#include <functional>

namespace render
{

/**
 * Main constructor.
 */
OpenGLRenderSystem::OpenGLRenderSystem() :
    _realised(false),
    _shaderProgramsAvailable(false),
    _glProgramFactory(std::make_shared<GLProgramFactory>()),
    _currentShaderProgram(SHADER_PROGRAM_NONE),
    _time(0),
    _geometryStore(_syncObjectProvider, _bufferObjectProvider),
    _objectRenderer(_geometryStore),
    m_traverseRenderablesMutex(false)
{
    bool shouldRealise = false;

    // For the static default rendersystem, the DeclarationManager is not existent yet,
    // hence it will be attached in initialiseModule().
    if (module::GlobalModuleRegistry().moduleExists(MODULE_DECLMANAGER))
    {
        _materialDefsLoaded = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Material)
            .connect(sigc::mem_fun(*this, &OpenGLRenderSystem::realise));

        shouldRealise = true;
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

    // Destruct the shaders before the geometry store is destroyed
    _shaders.clear();
    _entities.clear();
    _lights.clear();
    _state_sorted.clear();
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

IRenderResult::Ptr OpenGLRenderSystem::renderFullBrightScene(RenderViewType renderViewType,
                                RenderStateFlags globalFlagsMask,
                                const IRenderView& view)
{
    // Pick the requested renderer
    auto& renderer = renderViewType == RenderViewType::Camera ? *_editorPreviewRenderer : *_orthoRenderer;

    return render(renderer, globalFlagsMask, view);
}

IRenderResult::Ptr OpenGLRenderSystem::renderLitScene(RenderStateFlags globalFlagsMask,
    const IRenderView& view)
{
    return render(*_lightingModeRenderer, globalFlagsMask, view);
}

IRenderResult::Ptr OpenGLRenderSystem::render(SceneRenderer& renderer, RenderStateFlags globalFlagsMask, const IRenderView& view)
{
    // Make sure all shaders are ready for rendering, submitting their data to the store
    for (const auto& [_, shader] : _shaders)
    {
        shader->prepareForRendering();
    }

    auto result = renderer.render(globalFlagsMask, view, _time);

    renderText();

    return result;
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

    _orthoRenderer = std::make_unique<FullBrightRenderer>(RenderViewType::OrthoView, _state_sorted, _geometryStore, _objectRenderer);
    _editorPreviewRenderer = std::make_unique<FullBrightRenderer>(RenderViewType::Camera, _state_sorted, _geometryStore, _objectRenderer);
    _lightingModeRenderer = std::make_unique<LightingModeRenderer>(*_glProgramFactory, _geometryStore, _objectRenderer, _lights, _entities);
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
        MODULE_COMMANDSYSTEM,
        MODULE_COLOURSCHEME_MANAGER,
        MODULE_SHADERSYSTEM,
        MODULE_XMLREGISTRY,
        MODULE_SHARED_GL_CONTEXT,
    };

    return _dependencies;
}

void OpenGLRenderSystem::initialiseModule(const IApplicationContext& ctx)
{
    _materialDefsLoaded = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Material)
        .connect(sigc::mem_fun(*this, &OpenGLRenderSystem::realise));

    // greebo: Don't realise the module yet, this must wait
    // until the shared GL context has been created (this
    // happens as soon as the first GL widget has been realised).
    _sharedContextCreated = GlobalOpenGLContext().signal_sharedContextCreated()
        .connect(sigc::mem_fun(this, &OpenGLRenderSystem::extensionsInitialised));

    _sharedContextDestroyed = GlobalOpenGLContext().signal_sharedContextDestroyed()
        .connect(sigc::mem_fun(this, &OpenGLRenderSystem::unrealise));

    GlobalCommandSystem().addCommand("ShowRenderMemoryStats",
        sigc::mem_fun(*this, &OpenGLRenderSystem::showMemoryStats));
}

void OpenGLRenderSystem::shutdownModule()
{
    _orthoRenderer.reset();
    _editorPreviewRenderer.reset();
    _lightingModeRenderer.reset();

    _entities.clear();
    _lights.clear();

    _textRenderers.clear();

    _sharedContextCreated.disconnect();
    _sharedContextDestroyed.disconnect();
	_materialDefsLoaded.disconnect();
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

IObjectRenderer& OpenGLRenderSystem::getObjectRenderer()
{
    return _objectRenderer;
}

void OpenGLRenderSystem::showMemoryStats(const cmd::ArgumentList& args)
{
    _geometryStore.printMemoryStats();
}

// Define the static OpenGLRenderSystem module
module::StaticModuleRegistration<OpenGLRenderSystem> openGLRenderSystemModule;

} // namespace render
