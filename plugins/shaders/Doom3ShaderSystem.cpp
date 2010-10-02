#include "Doom3ShaderSystem.h"

#include "iradiant.h"
#include "iregistry.h"
#include "ifilesystem.h"
#include "ipreferencesystem.h"

#include "xmlutil/Node.h"
#include "xmlutil/MissingXMLNodeException.h"

#include "ShaderDefinition.h"
#include "ShaderFileLoader.h"

#include "debugging/ScopedDebugTimer.h"

#include <boost/algorithm/string/predicate.hpp>

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

void Doom3ShaderSystem::construct() {
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
		GlobalRegistry().findXPath("game/filesystem/shaders/basepath");
	if (nlShaderPath.empty())
		throw xml::MissingXMLNodeException(MISSING_BASEPATH_NODE);

	xml::NodeList nlShaderExt = 
		GlobalRegistry().findXPath("game/filesystem/shaders/extension");
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

	globalOutputStream() << _library->getNumShaders() << " shaders found." << std::endl;
}

void Doom3ShaderSystem::realise() {
	if (!_realised) {
		loadMaterialFiles();
		_observers.realise();
		_realised = true;
	}
}

void Doom3ShaderSystem::unrealise() {
	if (_realised) {
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

void Doom3ShaderSystem::foreachShaderName(const ShaderNameCallback& callback) {
	// Pass the call to the Library
	_library->foreachShaderName(callback);
}

void Doom3ShaderSystem::beginActiveShadersIterator() {
	_library->getIterator() = _library->begin();
}
bool Doom3ShaderSystem::endActiveShadersIterator() {
	return _library->getIterator() == _library->end();
}
MaterialPtr Doom3ShaderSystem::dereferenceActiveShadersIterator() {
	return _library->getIterator()->second;
}
void Doom3ShaderSystem::incrementActiveShadersIterator() {
	_library->incrementIterator();
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

void Doom3ShaderSystem::setLightingEnabled(bool enabled) {
	if (CShader::m_lightingEnabled != enabled) {
		// First unrealise the lighting of all shaders
		for (ShaderLibrary::iterator i = _library->begin(); 
			 i != _library->end(); 
			 i++)
		{
			i->second->unrealiseLighting();
		}
		
		// Set the global (greebo: Does this really need to be done this way?) 
		CShader::m_lightingEnabled = enabled;
		
		// Now realise the lighting of all shaders
		for (ShaderLibrary::iterator i = _library->begin(); 
			 i != _library->end(); 
			 i++)
		{
			i->second->realiseLighting();
		}
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

const std::string& Doom3ShaderSystem::getName() const {
	static std::string _name(MODULE_SHADERSYSTEM);
	return _name;
}

const StringSet& Doom3ShaderSystem::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
	}

	return _dependencies;
}

void Doom3ShaderSystem::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "Doom3ShaderSystem::initialiseModule called\n";
	
	construct();
	realise();
}

void Doom3ShaderSystem::shutdownModule() {
	globalOutputStream() << "Doom3ShaderSystem::shutdownModule called\n";
	
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
