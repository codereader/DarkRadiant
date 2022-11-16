#include "MaterialManager.h"
#include "MaterialSourceGenerator.h"

#include "i18n.h"
#include "ideclmanager.h"

#include "iregistry.h"
#include "icommandsystem.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "igame.h"

#include "ShaderExpression.h"

#include "debugging/ScopedDebugTimer.h"
#include "module/StaticModule.h"

#include "decl/DeclarationCreator.h"
#include "stream/TemporaryOutputStream.h"
#include "materials/ParseLib.h"
#include <functional>

#include "decl/DeclLib.h"

namespace
{
    const char* TEXTURE_PREFIX = "textures/";

    // Default image maps for optional material stages
    const std::string IMAGE_FLAT = "_flat.bmp";
    const std::string IMAGE_BLACK = "_black.bmp";

    inline std::string getBitmapsPath()
    {
        return module::GlobalModuleRegistry().getApplicationContext().getBitmapsPath();
    }

}

namespace shaders
{

MaterialManager::MaterialManager() :
    _enableActiveUpdates(true)
{}

void MaterialManager::construct()
{
    _library = std::make_shared<ShaderLibrary>();
    _textureManager = std::make_shared<GLTextureManager>();
}

void MaterialManager::destroy()
{
    // Don't destroy the GLTextureManager, it's called from
    // the CShader destructors.
}

void MaterialManager::freeShaders() {
    _library->clear();
    _textureManager->checkBindings();
    activeShadersChangedNotify();
}

MaterialPtr MaterialManager::getMaterial(const std::string& name)
{
    return _library->findShader(name);
}

bool MaterialManager::materialExists(const std::string& name)
{
    return _library->definitionExists(name);
}

bool MaterialManager::materialCanBeModified(const std::string& name)
{
    if (!_library->definitionExists(name))
    {
        return false;
    }

    auto decl = _library->getTemplate(name);
    const auto& fileInfo = decl->getBlockSyntax().fileInfo;
    return fileInfo.name.empty() || fileInfo.getIsPhysicalFile();
}

void MaterialManager::foreachShaderName(const ShaderNameCallback& callback)
{
    // Pass the call to the Library
    _library->foreachShaderName(callback);
}

const char* MaterialManager::getTexturePrefix() const
{
    return TEXTURE_PREFIX;
}

GLTextureManager& MaterialManager::getTextureManager()
{
    return *_textureManager;
}

// Get default textures
TexturePtr MaterialManager::getDefaultInteractionTexture(IShaderLayer::Type type)
{
    TexturePtr defaultTex;

    // Look up based on layer type
    switch (type)
    {
    case IShaderLayer::DIFFUSE:
    case IShaderLayer::SPECULAR:
        defaultTex = _textureManager->getBinding(getBitmapsPath() + IMAGE_BLACK);
        break;

    case IShaderLayer::BUMP:
        defaultTex = _textureManager->getBinding(getBitmapsPath() + IMAGE_FLAT);
        break;
    default:
        break;
    }

    return defaultTex;
}

sigc::signal<void> MaterialManager::signal_activeShadersChanged() const
{
    return _signalActiveShadersChanged;
}

void MaterialManager::activeShadersChangedNotify()
{
    if (_enableActiveUpdates)
    {
        _signalActiveShadersChanged.emit();
    }
}

void MaterialManager::foreachMaterial(const std::function<void(const MaterialPtr&)>& func)
{
    _library->foreachShader(func);
}

TexturePtr MaterialManager::loadTextureFromFile(const std::string& filename)
{
    // Remove any unused Textures before allocating new ones.
    _textureManager->checkBindings();

    // Get the binding (i.e. load the texture)
    return _textureManager->getBinding(filename);
}

sigc::signal<void, const std::string&>& MaterialManager::signal_materialCreated()
{
    return _sigMaterialCreated;
}

sigc::signal<void, const std::string&, const std::string&>& MaterialManager::signal_materialRenamed()
{
    return _sigMaterialRenamed;
}

sigc::signal<void, const std::string&>& MaterialManager::signal_materialRemoved()
{
    return _sigMaterialRemoved;
}

IShaderExpression::Ptr MaterialManager::createShaderExpressionFromString(const std::string& exprStr)
{
    return ShaderExpression::createFromString(exprStr);
}

MaterialPtr MaterialManager::createEmptyMaterial(const std::string& name)
{
    // Find a non-conflicting name and create an empty declaration
    auto candidate = decl::generateNonConflictingName(decl::Type::Material, name);
    auto decl = GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Material, candidate);

    auto material = _library->findShader(candidate);
    material->setIsModified();

    _sigMaterialCreated.emit(candidate);

    return material;
}

bool MaterialManager::renameMaterial(const std::string& oldName, const std::string& newName)
{
    auto result = _library->renameDefinition(oldName, newName);

    if (result)
    {
        // Mark the material as modified
        _library->findShader(newName)->setIsModified();
        _sigMaterialRenamed(oldName, newName);
    }

    return result;
}

void MaterialManager::removeMaterial(const std::string& name)
{
    if (!_library->definitionExists(name))
    {
        rWarning() << "Cannot remove non-existent material " << name << std::endl;
        return;
    }

    _library->removeDefinition(name);

    _sigMaterialRemoved.emit(name);
}

MaterialPtr MaterialManager::copyMaterial(const std::string& nameOfOriginal, const std::string& nameOfCopy)
{
    if (nameOfCopy.empty())
    {
        rWarning() << "Cannot copy, the new name must not be empty" << std::endl;
        return MaterialPtr();
    }

    auto candidate = decl::generateNonConflictingName(decl::Type::Material, nameOfCopy);

    if (!_library->definitionExists(nameOfOriginal))
    {
        rWarning() << "Cannot copy non-existent material " << nameOfOriginal << std::endl;
        return MaterialPtr();
    }

    _library->copyDefinition(nameOfOriginal, candidate);

    _sigMaterialCreated.emit(candidate);

    auto material = _library->findShader(candidate);
    material->setIsModified();

    return material;
}

void MaterialManager::saveMaterial(const std::string& name)
{
    auto material = _library->findShader(name);

    if (!material->isModified())
    {
        rMessage() << "Material is not modified, nothing to save." << std::endl;
        return;
    }

    if (!materialCanBeModified(material->getName()))
    {
        throw std::runtime_error("Cannot save this material, it's read-only.");
    }

    // Store the modifications in our actual template and un-mark the file
    material->commitModifications();

    // Write the declaration to disk
    GlobalDeclarationManager().saveDeclaration(material->getTemplate());
}

ITableDefinition::Ptr MaterialManager::getTable(const std::string& name)
{
    return std::static_pointer_cast<ITableDefinition>(
        GlobalDeclarationManager().findDeclaration(decl::Type::Table, name)
    );
}

void MaterialManager::reloadImages()
{
    _library->foreachShader([](const CShaderPtr& shader)
    {
        shader->refreshImageMaps();
    });
}

const std::string& MaterialManager::getName() const
{
    static std::string _name(MODULE_SHADERSYSTEM);
    return _name;
}

const StringSet& MaterialManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_DECLMANAGER,
        MODULE_VIRTUALFILESYSTEM,
        MODULE_COMMANDSYSTEM,
        MODULE_XMLREGISTRY,
        MODULE_GAMEMANAGER,
        MODULE_FILETYPES,
    };

    return _dependencies;
}

void MaterialManager::initialiseModule(const IApplicationContext& ctx)
{
    GlobalDeclarationManager().registerDeclType("table", std::make_shared<decl::DeclarationCreator<TableDefinition>>(decl::Type::Table));
    GlobalDeclarationManager().registerDeclType("material", std::make_shared<decl::DeclarationCreator<ShaderTemplate>>(decl::Type::Material));
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "materials/", ".mtr");

    _materialsReloadedSignal = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Material)
        .connect(sigc::mem_fun(this, &MaterialManager::onMaterialDefsReloaded));

    construct();

    // Register the mtr file extension
    GlobalFiletypes().registerPattern("material", FileTypePattern(_("Material File"), "mtr", "*.mtr"));

    GlobalCommandSystem().addCommand("ReloadImages", [this](const cmd::ArgumentList&) { reloadImages(); });
}

void MaterialManager::onMaterialDefsReloaded()
{
    _library->foreachShader([](const CShaderPtr& shader)
    {
        shader->unrealise();
        shader->realise();
        shader->refreshImageMaps();
    });
}

void MaterialManager::shutdownModule()
{
    rMessage() << "MaterialManager::shutdownModule called" << std::endl;

    destroy();
    _library->clear();
    _library.reset();
}

// Accessor function encapsulating the static shadersystem instance
MaterialManagerPtr GetShaderSystem()
{
    // Acquire the moduleptr from the module registry
    RegisterableModulePtr modulePtr(module::GlobalModuleRegistry().getModule(MODULE_SHADERSYSTEM));

    // static_cast it onto our shadersystem type
    return std::static_pointer_cast<MaterialManager>(modulePtr);
}

GLTextureManager& GetTextureManager()
{
    return GetShaderSystem()->getTextureManager();
}

// Static module instance
module::StaticModuleRegistration<MaterialManager> materialManagerModule;

} // namespace shaders
