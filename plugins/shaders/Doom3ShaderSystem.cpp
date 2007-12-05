#include "Doom3ShaderSystem.h"

#include "iradiant.h"
#include "iregistry.h"
#include "ifilesystem.h"
#include "ipreferencesystem.h"

#include "xmlutil/Node.h"

#include "ShaderDefinition.h"
#include "ShaderFileLoader.h"
#include "MissingXMLNodeException.h"

#include <boost/algorithm/string/predicate.hpp>

namespace {
	const char* TEXTURE_PREFIX = "textures/";
	const char* MISSING_BASEPATH_NODE =
		"Failed to find \"/game/filesystem/shaders/basepath\" node \
in game descriptor";
	 
	const char* MISSING_EXTENSION_NODE =
		"Failed to find \"/game/filesystem/shaders/extension\" node \
in game descriptor";
}

namespace shaders {

// Constructor
Doom3ShaderSystem::Doom3ShaderSystem() :
	_shadersUnrealised(1),
	_observers(getName())
{}

void Doom3ShaderSystem::construct() {
	_library = ShaderLibraryPtr(new ShaderLibrary());
	_textureManager = GLTextureManagerPtr(new GLTextureManager());
	
	// Register this class as moduleobserver 
	GlobalFileSystem().attach(*this);
}

void Doom3ShaderSystem::destroy() {
	// De-register this class
	GlobalFileSystem().detach(*this);
	
	// Free the shaders if we're in realised state
	if (_shadersUnrealised == 0) {
		freeShaders();
	}
	
	// Don't destroy the GLTextureManager, it's called from
	// the CShader destructors.
}

void Doom3ShaderSystem::loadMaterialFiles() {
	// Get the shaders path and extension from the XML game file
	xml::NodeList nlShaderPath = 
		GlobalRegistry().findXPath("game/filesystem/shaders/basepath");
	if (nlShaderPath.size() != 1)
		throw MissingXMLNodeException(MISSING_BASEPATH_NODE);

	xml::NodeList nlShaderExt = 
		GlobalRegistry().findXPath("game/filesystem/shaders/extension");
	if (nlShaderExt.size() != 1)
		throw MissingXMLNodeException(MISSING_EXTENSION_NODE);

	// Load the shader files from the VFS
	std::string sPath = nlShaderPath[0].getContent();
	if (!boost::algorithm::ends_with(sPath, "/"))
		sPath += "/";
		
	std::string extension = nlShaderExt[0].getContent();
	
	// Load each file from the global filesystem
	ShaderFileLoader ldr(sPath);
	GlobalFileSystem().forEachFile(sPath.c_str(), 
								   extension.c_str(), 
								   makeCallback1(ldr), 
								   0);
}

void Doom3ShaderSystem::realise() {
	if (--_shadersUnrealised == 0) {
		loadMaterialFiles();
		_observers.realise();
	}
}

void Doom3ShaderSystem::unrealise() {
	if (++_shadersUnrealised == 1) {
		_observers.unrealise();
		freeShaders();
	}
}

void Doom3ShaderSystem::freeShaders() {
	_library->clear();
	_textureManager->checkBindings();
}

void Doom3ShaderSystem::refresh() {
	unrealise();
	realise();
}

// Is the shader system realised
bool Doom3ShaderSystem::isRealised() {
	return _shadersUnrealised == 0;
}

// Return a shader by name
IShaderPtr Doom3ShaderSystem::getShaderForName(const std::string& name) {
	return _library->findShader(name);
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
IShaderPtr Doom3ShaderSystem::dereferenceActiveShadersIterator() {
	return _library->getIterator()->second;
}
void Doom3ShaderSystem::incrementActiveShadersIterator() {
	_library->incrementIterator();
}

void Doom3ShaderSystem::attach(ModuleObserver& observer) {
	_observers.attach(observer);
}
void Doom3ShaderSystem::detach(ModuleObserver& observer) {
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
		_dependencies.insert(MODULE_RADIANT);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
	}

	return _dependencies;
}

void Doom3ShaderSystem::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "Doom3ShaderSystem::initialiseModule called\n";
	
	construct();
}

void Doom3ShaderSystem::shutdownModule() {
	globalOutputStream() << "Doom3ShaderSystem::shutdownModule called\n";
	
	destroy();
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
