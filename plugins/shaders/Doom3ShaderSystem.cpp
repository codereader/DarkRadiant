#include "Doom3ShaderSystem.h"

#include "i18n.h"
#include "iradiant.h"
#include "iregistry.h"
#include "ifilesystem.h"
#include "ipreferencesystem.h"
#include "imainframe.h"
#include "ieventmanager.h"
#include "igame.h"

#include "xmlutil/Node.h"
#include "xmlutil/MissingXMLNodeException.h"

#include "ShaderDefinition.h"
#include "ShaderFileLoader.h"
#include "ShaderExpression.h"

#include "debugging/ScopedDebugTimer.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/bind.hpp>

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

namespace shaders {

// Constructor
Doom3ShaderSystem::Doom3ShaderSystem() :
	_enableActiveUpdates(true),
	_realised(false),
	_observers(getName())
{}

void Doom3ShaderSystem::construct()
{
	_library = ShaderLibraryPtr(new ShaderLibrary());
	_textureManager = GLTextureManagerPtr(new GLTextureManager());

	// Register this class as VFS observer
	GlobalFileSystem().addObserver(*this);
}

void Doom3ShaderSystem::destroy() {
	// De-register this class as VFS Observer
	GlobalFileSystem().removeObserver(*this);

	// Free the shaders if we're in realised state
	if (_realised) {
		freeShaders();
	}

	// Don't destroy the GLTextureManager, it's called from
	// the CShader destructors.
}

void Doom3ShaderSystem::loadMaterialFiles()
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
	if (!boost::algorithm::ends_with(sPath, "/"))
		sPath += "/";

	std::string extension = nlShaderExt[0].getContent();

	// Load each file from the global filesystem
	ShaderFileLoader loader(sPath);
	{
		ScopedDebugTimer timer("ShaderFiles parsed: ");
		GlobalFileSystem().forEachFile(sPath, extension, loader, 0);
	}

	rMessage() << _library->getNumShaders() << " shaders found." << std::endl;
}

void Doom3ShaderSystem::realise() {
	if (!_realised) {
		loadMaterialFiles();
		_observers.realise();
		_realised = true;
	}
}

void Doom3ShaderSystem::unrealise()
{
	if (_realised)
	{
		_tables.clear();
		_observers.unrealise();
		freeShaders();
		_realised = false;
	}
}

void Doom3ShaderSystem::onFileSystemInitialise() {
	realise();
}

void Doom3ShaderSystem::onFileSystemShutdown() {
	unrealise();
}

void Doom3ShaderSystem::freeShaders() {
	_library->clear();
	_textureManager->checkBindings();
	activeShadersChangedNotify();
}

void Doom3ShaderSystem::refresh() {
	unrealise();
	realise();
}

// Is the shader system realised
bool Doom3ShaderSystem::isRealised() {
	return _realised;
}

// Return a shader by name
MaterialPtr Doom3ShaderSystem::getMaterialForName(const std::string& name)
{
	CShaderPtr shader = _library->findShader(name);
	return shader;
}

bool Doom3ShaderSystem::materialExists(const std::string& name)
{
	return _library->definitionExists(name);
}

void Doom3ShaderSystem::foreachShaderName(const ShaderNameCallback& callback) {
	// Pass the call to the Library
	_library->foreachShaderName(callback);
}

void Doom3ShaderSystem::attach(ModuleObserver& observer)
{
	_observers.attach(observer);

	if (_realised)
	{
		observer.realise();
	}
}

void Doom3ShaderSystem::detach(ModuleObserver& observer)
{
	if (_realised)
	{
		observer.unrealise();
	}

	_observers.detach(observer);
}

void Doom3ShaderSystem::setLightingEnabled(bool enabled)
{
	if (CShader::m_lightingEnabled != enabled)
	{
		// First unrealise the lighting of all shaders
		_library->unrealiseLighting();

		// Set the global (greebo: Does this really need to be done this way?)
		CShader::m_lightingEnabled = enabled;

		// Now realise the lighting of all shaders
		_library->realiseLighting();
	}
}

const char* Doom3ShaderSystem::getTexturePrefix() const {
	return TEXTURE_PREFIX;
}

ShaderLibrary& Doom3ShaderSystem::getLibrary() {
	return *_library;
}

GLTextureManager& Doom3ShaderSystem::getTextureManager() {
	return *_textureManager;
}

// Get default textures
TexturePtr Doom3ShaderSystem::getDefaultInteractionTexture(ShaderLayer::Type t)
{
    TexturePtr defaultTex;

    // Look up based on layer type
    switch (t)
    {
    case ShaderLayer::DIFFUSE:
    case ShaderLayer::SPECULAR:
        defaultTex = GetTextureManager().getBinding(
            GlobalRegistry().get(RKEY_BITMAPS_PATH) + IMAGE_BLACK
        );
        break;

    case ShaderLayer::BUMP:
        defaultTex = GetTextureManager().getBinding(
            GlobalRegistry().get(RKEY_BITMAPS_PATH) + IMAGE_FLAT
        );
        break;
	default:
		break;
    }

    return defaultTex;
}

void Doom3ShaderSystem::addActiveShadersObserver(const ActiveShadersObserverPtr& observer)
{
	_activeShadersObservers.insert(observer);
}

void Doom3ShaderSystem::removeActiveShadersObserver(const ActiveShadersObserverPtr& observer)
{
	_activeShadersObservers.erase(observer);
}

void Doom3ShaderSystem::activeShadersChangedNotify()
{
	if (_enableActiveUpdates)
	{
		for (Observers::const_iterator i = _activeShadersObservers.begin();
			 i != _activeShadersObservers.end(); )
		{
			(*i++)->onActiveShadersChanged();
		}
	}
}

void Doom3ShaderSystem::foreachShader(ShaderVisitor& visitor) {
	_library->foreachShader(visitor);
}

TexturePtr Doom3ShaderSystem::loadTextureFromFile(const std::string& filename,
												  const std::string& moduleNames)
{
	// Remove any unused Textures before allocating new ones.
	_textureManager->checkBindings();

	// Pass the call to the library
	return _library->loadTextureFromFile(filename, moduleNames);
}

IShaderExpressionPtr Doom3ShaderSystem::createShaderExpressionFromString(const std::string& exprStr)
{
	return ShaderExpression::createFromString(exprStr);
}

TableDefinitionPtr Doom3ShaderSystem::getTableForName(const std::string& name)
{
	TableDefinitions::const_iterator i = _tables.find(name);

	return i != _tables.end() ? i->second : TableDefinitionPtr();
}

bool Doom3ShaderSystem::addTableDefinition(const TableDefinitionPtr& def)
{
	std::pair<TableDefinitions::iterator, bool> result = _tables.insert(
		TableDefinitions::value_type(def->getName(), def));

	return result.second;
}

void Doom3ShaderSystem::refreshShadersCmd(const cmd::ArgumentList& args)
{
	// Disable screen updates for the scope of this function
	IScopedScreenUpdateBlockerPtr blocker = GlobalMainFrame().getScopedScreenUpdateBlocker(_("Processing..."), _("Loading Shaders"));

	// Reload the Shadersystem, this will also trigger an 
	// OpenGLRenderSystem unrealise/realise sequence as the rendersystem
	// is attached to this class as Observer
	refresh();

	GlobalMainFrame().updateAllWindows();
}

const std::string& Doom3ShaderSystem::getName() const {
	static std::string _name(MODULE_SHADERSYSTEM);
	return _name;
}

const StringSet& Doom3ShaderSystem::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
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

	GlobalCommandSystem().addCommand("RefreshShaders", boost::bind(&Doom3ShaderSystem::refreshShadersCmd, this, _1));
	GlobalEventManager().addCommand("RefreshShaders", "RefreshShaders");

	construct();
	realise();

#ifdef _DEBUG
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
	rMessage() << "Doom3ShaderSystem::shutdownModule called\n";

	destroy();
	unrealise();
}

} // namespace shaders

// Accessor function encapsulating the static shadersystem instance
shaders::Doom3ShaderSystemPtr GetShaderSystem() {
	// Acquire the moduleptr from the module registry
	RegisterableModulePtr modulePtr(module::GlobalModuleRegistry().getModule(MODULE_SHADERSYSTEM));

	// static_cast it onto our shadersystem type
	return boost::static_pointer_cast<shaders::Doom3ShaderSystem>(modulePtr);
}

shaders::ShaderLibrary& GetShaderLibrary() {
	return GetShaderSystem()->getLibrary();
}

shaders::GLTextureManager& GetTextureManager() {
	return GetShaderSystem()->getTextureManager();
}
