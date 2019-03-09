#include "Doom3ShaderSystem.h"
#include "ShaderFileLoader.h"

#include "i18n.h"
#include "iradiant.h"
#include "iregistry.h"
#include "ifilesystem.h"
#include "ipreferencesystem.h"
#include "imainframe.h"
#include "ieventmanager.h"
#include "iradiant.h"
#include "igame.h"
#include "iarchive.h"

#include "xmlutil/Node.h"
#include "xmlutil/MissingXMLNodeException.h"

#include "ShaderDefinition.h"
#include "ShaderExpression.h"

#include "debugging/ScopedDebugTimer.h"
#include "modulesystem/StaticModule.h"

#include "string/predicate.h"
#include "string/replace.h"
#include "parser/DefBlockTokeniser.h"
#include <functional>

namespace {
    const char* TEXTURE_PREFIX = "textures/";
    const char* MISSING_BASEPATH_NODE =
        "Failed to find \"/game/filesystem/shaders/basepath\" node \
in game descriptor";

    const char* MISSING_EXTENSION_NODE =
        "Failed to find \"/game/filesystem/shaders/extension\" node \
in game descriptor";

    // Default image maps for optional material stages
    const std::string IMAGE_FLAT = "_flat.bmp";
    const std::string IMAGE_BLACK = "_black.bmp";

}

namespace shaders
{

// Constructor
Doom3ShaderSystem::Doom3ShaderSystem() :
    _defLoader(std::bind(&Doom3ShaderSystem::loadMaterialFiles, this)),
    _enableActiveUpdates(true),
    _realised(false)
{}

void Doom3ShaderSystem::construct()
{
    _library = std::make_shared<ShaderLibrary>();
    _textureManager = std::make_shared<GLTextureManager>();

    // Register this class as VFS observer
    GlobalFileSystem().addObserver(*this);
}

void Doom3ShaderSystem::destroy()
{
    // De-register this class as VFS Observer
    GlobalFileSystem().removeObserver(*this);

    // Free the shaders if we're in realised state
    if (_realised) 
    {
        freeShaders();
    }

    // Don't destroy the GLTextureManager, it's called from
    // the CShader destructors.
}

ShaderLibraryPtr Doom3ShaderSystem::loadMaterialFiles()
{
    // Get the shaders path and extension from the XML game file
    xml::NodeList nlShaderPath =
        GlobalGameManager().currentGame()->getLocalXPath("/filesystem/shaders/basepath");
    if (nlShaderPath.empty())
        throw xml::MissingXMLNodeException(MISSING_BASEPATH_NODE);

    xml::NodeList nlShaderExt =
        GlobalGameManager().currentGame()->getLocalXPath("/filesystem/shaders/extension");
    if (nlShaderExt.empty())
        throw xml::MissingXMLNodeException(MISSING_EXTENSION_NODE);

    // Load the shader files from the VFS
    std::string sPath = nlShaderPath[0].getContent();
    if (!string::ends_with(sPath, "/"))
        sPath += "/";

    std::string extension = nlShaderExt[0].getContent();

    ShaderLibraryPtr library = std::make_shared<ShaderLibrary>();

    // Load each file from the global filesystem
    ShaderFileLoader<ShaderLibrary> loader(sPath, *library);
    {
        ScopedDebugTimer timer("ShaderFiles parsed: ");
        GlobalFileSystem().forEachFile(
            sPath, extension,
            [&](const vfs::FileInfo& fileInfo) { loader.addFile(fileInfo); },
            0
        );
        loader.parseFiles();
    }

    rMessage() << library->getNumDefinitions() << " shader definitions found." << std::endl;

    return library;
}

void Doom3ShaderSystem::realise()
{
    if (!_realised) 
    {
        // Start loading defs
        _defLoader.start();

        _signalDefsLoaded.emit();
        _realised = true;
    }
}

void Doom3ShaderSystem::unrealise()
{
    if (_realised)
    {
        _signalDefsUnloaded.emit();
        freeShaders();
        _realised = false;
    }
}

void Doom3ShaderSystem::ensureDefsLoaded()
{
    // To avoid assigning the pointer everytime, check if the library is empty
    if (_library->getNumDefinitions() == 0)
    {
        _library = _defLoader.get();
    }
}

void Doom3ShaderSystem::onFileSystemInitialise()
{
    realise();
}

void Doom3ShaderSystem::onFileSystemShutdown()
{
    unrealise();
}

void Doom3ShaderSystem::freeShaders() {
    _library->clear();
    _defLoader.reset();
    _textureManager->checkBindings();
    activeShadersChangedNotify();
}

void Doom3ShaderSystem::refresh() {
    unrealise();
    realise();
}

// Is the shader system realised
bool Doom3ShaderSystem::isRealised()
{
    // Don't report true until we have at least some definitions loaded
    return _realised && _library->getNumDefinitions() > 0;
}

sigc::signal<void>& Doom3ShaderSystem::signal_DefsLoaded()
{
    return _signalDefsLoaded;
}

sigc::signal<void>& Doom3ShaderSystem::signal_DefsUnloaded()
{
    return _signalDefsUnloaded;
}

// Return a shader by name
MaterialPtr Doom3ShaderSystem::getMaterialForName(const std::string& name)
{
    ensureDefsLoaded();

    CShaderPtr shader = _library->findShader(name);
    return shader;
}

bool Doom3ShaderSystem::materialExists(const std::string& name)
{
    ensureDefsLoaded();

    return _library->definitionExists(name);
}

void Doom3ShaderSystem::foreachShaderName(const ShaderNameCallback& callback)
{
    ensureDefsLoaded();

    // Pass the call to the Library
    _library->foreachShaderName(callback);
}

void Doom3ShaderSystem::setLightingEnabled(bool enabled)
{
    ensureDefsLoaded();

    if (CShader::m_lightingEnabled != enabled)
    {
        // First unrealise the lighting of all shaders
        _library->foreachShader([](const CShaderPtr& shader)
        {
            shader->unrealiseLighting();
        });

        // Set the global (greebo: Does this really need to be done this way?)
        CShader::m_lightingEnabled = enabled;

        // Now realise the lighting of all shaders
        _library->foreachShader([](const CShaderPtr& shader)
        {
            shader->realiseLighting();
        });
    }
}

const char* Doom3ShaderSystem::getTexturePrefix() const
{
    return TEXTURE_PREFIX;
}

GLTextureManager& Doom3ShaderSystem::getTextureManager()
{
    return *_textureManager;
}

// Get default textures
TexturePtr Doom3ShaderSystem::getDefaultInteractionTexture(ShaderLayer::Type type)
{
    TexturePtr defaultTex;

    // Look up based on layer type
    switch (type)
    {
    case ShaderLayer::DIFFUSE:
    case ShaderLayer::SPECULAR:
        defaultTex = _textureManager->getBinding(
            GlobalRegistry().get(RKEY_BITMAPS_PATH) + IMAGE_BLACK
        );
        break;

    case ShaderLayer::BUMP:
        defaultTex = _textureManager->getBinding(
            GlobalRegistry().get(RKEY_BITMAPS_PATH) + IMAGE_FLAT
        );
        break;
    default:
        break;
    }

    return defaultTex;
}

sigc::signal<void> Doom3ShaderSystem::signal_activeShadersChanged() const
{
    return _signalActiveShadersChanged;
}

void Doom3ShaderSystem::activeShadersChangedNotify()
{
    if (_enableActiveUpdates)
    {
        _signalActiveShadersChanged.emit();
    }
}

void Doom3ShaderSystem::foreachMaterial(const std::function<void(const MaterialPtr&)>& func)
{
    ensureDefsLoaded();

    _library->foreachShader(func);
}

TexturePtr Doom3ShaderSystem::loadTextureFromFile(const std::string& filename)
{
    // Remove any unused Textures before allocating new ones.
    _textureManager->checkBindings();

    // Get the binding (i.e. load the texture)
    return _textureManager->getBinding(filename);
}

IShaderExpressionPtr Doom3ShaderSystem::createShaderExpressionFromString(const std::string& exprStr)
{
    return ShaderExpression::createFromString(exprStr);
}

TableDefinitionPtr Doom3ShaderSystem::getTableForName(const std::string& name)
{
    ensureDefsLoaded();

    return _library->getTableForName(name);
}

void Doom3ShaderSystem::refreshShadersCmd(const cmd::ArgumentList& args)
{
    // Disable screen updates for the scope of this function
    IScopedScreenUpdateBlockerPtr blocker = GlobalMainFrame().getScopedScreenUpdateBlocker(_("Processing..."), _("Loading Shaders"));

    // Reload the Shadersystem, this will also trigger an 
    // OpenGLRenderSystem unrealise/realise sequence as the rendersystem
    // is attached to this class as Observer
    // We can't do this refresh() operation in a thread it seems due to context binding
    refresh();

    GlobalMainFrame().updateAllWindows();
}

const std::string& Doom3ShaderSystem::getName() const
{
    static std::string _name(MODULE_SHADERSYSTEM);
    return _name;
}

const StringSet& Doom3ShaderSystem::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
    {
        _dependencies.insert(MODULE_VIRTUALFILESYSTEM);
        _dependencies.insert(MODULE_XMLREGISTRY);
        _dependencies.insert(MODULE_GAMEMANAGER);
        _dependencies.insert(MODULE_PREFERENCESYSTEM);
    }

    return _dependencies;
}

void Doom3ShaderSystem::initialiseModule(const ApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called" << std::endl;

    GlobalCommandSystem().addCommand("RefreshShaders", 
        std::bind(&Doom3ShaderSystem::refreshShadersCmd, this, std::placeholders::_1));
    GlobalEventManager().addCommand("RefreshShaders", "RefreshShaders");

    construct();
    realise();

#if 0
    testShaderExpressionParsing();
#endif
}

// Horrible evil macro to avoid assertion failures if expr is NULL
#define GET_EXPR_OR_RETURN expr = createShaderExpressionFromString(exprStr);\
                                  if (!expr) return;

void Doom3ShaderSystem::testShaderExpressionParsing()
{
    // Test a few things
    std::string exprStr = "3";
    IShaderExpressionPtr expr;
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3+4";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "(3+4)";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "(4.2)";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3+5+6";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3+(5+6)";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3 * 3+5";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3+3*5";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "(3+3)*5";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "(3+3*7)-5";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3-3*5";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "blinktable[0]";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "blinktable[1]";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "blinktable[0.3]";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "blinksnaptable[0.3]";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "xianjittertable[0]";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "xianjittertable[time]";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3-3*xianjittertable[2]";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3+xianjittertable[3]*7";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "(3+xianjittertable[3])*7";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "2.3 % 2";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "2.0 % 0.5";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "2 == 2";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "1 == 2";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "1 != 2";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "1.2 != 1.2";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "1.2 == 1.2*3";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "1.2*3 == 1.2*3";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3 == 3 && 1 != 0";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "1 != 1 || 3 == 3";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "4 == 3 || 1 != 0";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "time";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(2) << std::endl;

    exprStr = "-3 + 5";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3 * -5";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3 * -5 + 4";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3 + -5 * 4";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "3 * 5 * -6";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;

    exprStr = "decalFade[(time - Parm3)/(parm4 - parm3)]";
    GET_EXPR_OR_RETURN;
    rMessage() << "Expression " << exprStr << ": " << expr->getValue(0) << std::endl;
}

void Doom3ShaderSystem::shutdownModule()
{
    rMessage() << "Doom3ShaderSystem::shutdownModule called" << std::endl;

    destroy();
    unrealise();
}

// Accessor function encapsulating the static shadersystem instance
Doom3ShaderSystemPtr GetShaderSystem()
{
    // Acquire the moduleptr from the module registry
    RegisterableModulePtr modulePtr(module::GlobalModuleRegistry().getModule(MODULE_SHADERSYSTEM));

    // static_cast it onto our shadersystem type
    return std::static_pointer_cast<Doom3ShaderSystem>(modulePtr);
}

GLTextureManager& GetTextureManager()
{
    return GetShaderSystem()->getTextureManager();
}

// Static module instance
module::StaticModule<Doom3ShaderSystem> d3ShaderSystemModule;

} // namespace shaders
