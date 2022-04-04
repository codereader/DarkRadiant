#include "Doom3ShaderSystem.h"
#include "ShaderFileLoader.h"
#include "MaterialSourceGenerator.h"

#include "i18n.h"
#include "iradiant.h"
#include "iregistry.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "ipreferencesystem.h"
#include "iradiant.h"
#include "igame.h"
#include "iarchive.h"

#include "xmlutil/Node.h"
#include "xmlutil/MissingXMLNodeException.h"

#include "ShaderDefinition.h"
#include "ShaderExpression.h"

#include "debugging/ScopedDebugTimer.h"
#include "module/StaticModule.h"

#include "os/file.h"
#include "os/path.h"
#include "decl/SpliceHelper.h"
#include "stream/TemporaryOutputStream.h"
#include "string/predicate.h"
#include "string/replace.h"
#include "materials/ParseLib.h"
#include "parser/DefBlockTokeniser.h"
#include <functional>

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

Doom3ShaderSystem::Doom3ShaderSystem() :
    _enableActiveUpdates(true),
    _realised(false)
{}

void Doom3ShaderSystem::construct()
{
    _defLoader = std::make_unique<ShaderFileLoader>();
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

void Doom3ShaderSystem::realise()
{
    if (!_realised)
    {
        // Start loading defs
        _defLoader->start();

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
        _library = _defLoader->get();
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
    _defLoader->reset();
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
MaterialPtr Doom3ShaderSystem::getMaterial(const std::string& name)
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

bool Doom3ShaderSystem::materialCanBeModified(const std::string& name)
{
    ensureDefsLoaded();

    if (!_library->definitionExists(name))
    {
        return false;
    }

    const auto& def = _library->getDefinition(name);
    return def.file.name.empty() || def.file.getIsPhysicalFile();
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
TexturePtr Doom3ShaderSystem::getDefaultInteractionTexture(IShaderLayer::Type type)
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

sigc::signal<void, const std::string&>& Doom3ShaderSystem::signal_materialCreated()
{
    return _sigMaterialCreated;
}

sigc::signal<void, const std::string&, const std::string&>& Doom3ShaderSystem::signal_materialRenamed()
{
    return _sigMaterialRenamed;
}

sigc::signal<void, const std::string&>& Doom3ShaderSystem::signal_materialRemoved()
{
    return _sigMaterialRemoved;
}

IShaderExpression::Ptr Doom3ShaderSystem::createShaderExpressionFromString(const std::string& exprStr)
{
    return ShaderExpression::createFromString(exprStr);
}

std::string Doom3ShaderSystem::ensureNonConflictingName(const std::string& name)
{
    auto candidate = name;
    auto i = 0;

    while (_library->definitionExists(candidate))
    {
        candidate += fmt::format("{0:02d}", ++i);
    }

    return candidate;
}

MaterialPtr Doom3ShaderSystem::createEmptyMaterial(const std::string& name)
{
    auto candidate = ensureNonConflictingName(name);

    // Create a new template/definition
    auto shaderTemplate = std::make_shared<ShaderTemplate>(candidate, "");

    ShaderDefinition def{ shaderTemplate, vfs::FileInfo("", "", vfs::Visibility::HIDDEN) };

    _library->addDefinition(candidate, def);

    auto material = _library->findShader(candidate);
    material->setIsModified();

    _sigMaterialCreated.emit(candidate);

    return material;
}

bool Doom3ShaderSystem::renameMaterial(const std::string& oldName, const std::string& newName)
{
    if (oldName == newName)
    {
        rWarning() << "Cannot rename, the new name is no different" << std::endl;
        return false;
    }

    if (!_library->definitionExists(oldName))
    {
        rWarning() << "Cannot rename non-existent material " << oldName << std::endl;
        return false;
    }

    if (_library->definitionExists(newName))
    {
        rWarning() << "Cannot rename material to " << newName << " since this name is already in use" << std::endl;
        return false;
    }

    _library->renameDefinition(oldName, newName);

    _sigMaterialRenamed(oldName, newName);

    return true;
}

void Doom3ShaderSystem::removeMaterial(const std::string& name)
{
    if (!_library->definitionExists(name))
    {
        rWarning() << "Cannot remove non-existent material " << name << std::endl;
        return;
    }

    _library->removeDefinition(name);

    _sigMaterialRemoved.emit(name);
}

MaterialPtr Doom3ShaderSystem::createDefaultMaterial(const std::string& name)
{
    return std::make_shared<CShader>(name, _library->getEmptyDefinition(), true);
}

MaterialPtr Doom3ShaderSystem::copyMaterial(const std::string& nameOfOriginal, const std::string& nameOfCopy)
{
    if (nameOfCopy.empty())
    {
        rWarning() << "Cannot copy, the new name must not be empty" << std::endl;
        return MaterialPtr();
    }

    auto candidate = ensureNonConflictingName(nameOfCopy);

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

void Doom3ShaderSystem::saveMaterial(const std::string& name)
{
    ensureDefsLoaded();

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

    if (material->getShaderFileInfo().fullPath().empty())
    {
        throw std::runtime_error("No file path set on this material, cannot save.");
    }

    // Construct the output path for this material
    fs::path outputPath = GlobalGameManager().getModPath();

    if (outputPath.empty())
    {
        outputPath = GlobalGameManager().getUserEnginePath();
    }

    outputPath /= material->getShaderFileInfo().fullPath();

    auto outputDir = os::getDirectory(outputPath.string());
    if (!os::fileOrDirExists(outputDir))
    {
        rMessage() << "Creating mod materials path: " << outputDir << std::endl;
        fs::create_directories(outputDir);
    }

    rMessage() << "Saving material " << material->getName() << " to " << outputPath << std::endl;

    stream::TemporaryOutputStream tempStream(outputPath);

    std::string tempString;
    auto& stream = tempStream.getStream();

    // If a previous file exists, open it for reading and filter out the material def we'll be writing
    if (fs::exists(outputPath))
    {
        std::ifstream inheritStream(outputPath);

        if (!inheritStream.is_open())
        {
            throw std::runtime_error(
                fmt::format(_("Cannot open file for reading: {0}"), outputPath.string()));
        }

        // Write the file to the output stream, up to the point the material def should be written to
        std::regex pattern(getDeclNamePatternForMaterialName(material->getName()));

        decl::SpliceHelper::PipeStreamUntilInsertionPoint(inheritStream, stream, pattern);

        if (inheritStream.eof())
        {
            // Material declaration was not found in the inherited stream, write our comment
            stream << std::endl << std::endl;

            MaterialSourceGenerator::WriteMaterialGenerationComment(stream);
        }

        // We're at the insertion point (which might as well be EOF of the inheritStream)

        // Write the name, curly braces and block contents
        MaterialSourceGenerator::WriteFullMaterialToStream(stream, material);

        stream << inheritStream.rdbuf();

        inheritStream.close();
    }
    else
    {
        // File is empty, just write the comment and the declaration
        MaterialSourceGenerator::WriteMaterialGenerationComment(stream);
        MaterialSourceGenerator::WriteFullMaterialToStream(stream, material);
    }

    tempStream.closeAndReplaceTargetFile();

    // Store the modifications in our actual template and un-mark the file
    material->commitModifications();

    // Update the template in our library
    // Re-acquire the vfs::FileInfo structure which might still be empty for a newly created material
    _library->replaceDefinition(material->getName(), ShaderDefinition
    {
        material->getTemplate(),
        GlobalFileSystem().getFileInfo(material->getShaderFileInfo().fullPath())
    });
}

ITableDefinition::Ptr Doom3ShaderSystem::getTable(const std::string& name)
{
    ensureDefsLoaded();

    return _library->getTableForName(name);
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
        _dependencies.insert(MODULE_FILETYPES);
    }

    return _dependencies;
}

void Doom3ShaderSystem::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called" << std::endl;

    construct();
    realise();

#if 0
    testShaderExpressionParsing();
#endif

    // Register the mtr file extension
    GlobalFiletypes().registerPattern("material", FileTypePattern(_("Material File"), "mtr", "*.mtr"));
}

// Horrible evil macro to avoid assertion failures if expr is NULL
#define GET_EXPR_OR_RETURN expr = createShaderExpressionFromString(exprStr);\
                                  if (!expr) return;

void Doom3ShaderSystem::testShaderExpressionParsing()
{
    // Test a few things
    std::string exprStr = "3";
    IShaderExpression::Ptr expr;
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
module::StaticModuleRegistration<Doom3ShaderSystem> d3ShaderSystemModule;

} // namespace shaders
