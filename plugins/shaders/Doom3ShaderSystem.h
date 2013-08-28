#pragma once

#include "ishaders.h"
#include "ifilesystem.h"
#include "imodule.h"
#include "icommandsystem.h"

#include <boost/function.hpp>
#include "moduleobservers.h"

#include "ShaderLibrary.h"
#include "TableDefinition.h"
#include "textures/GLTextureManager.h"

namespace shaders {

/**
 * \brief
 * Implementation of the MaterialManager for Doom 3 .
 */
class Doom3ShaderSystem
: public MaterialManager,
  public VirtualFileSystem::Observer
{
	// The shaderlibrary stores all the known shaderdefinitions
	// as well as the active shaders
	ShaderLibraryPtr _library;

	// The lookup tables used in shader expressions
	typedef std::map<std::string, TableDefinitionPtr, ShaderNameCompareFunctor> TableDefinitions;
	TableDefinitions _tables;

	// The manager that handles the texture caching.
	GLTextureManagerPtr _textureManager;

	// A list of observers with regards to the active shaders list
	typedef std::set<ActiveShadersObserverPtr> Observers;
	Observers _activeShadersObservers;

	// Flag to indicate whether the active shaders callback should be invoked
	bool _enableActiveUpdates;

	// TRUE if the material files have been parsed
	bool _realised;

	// The observers that are attached to this system. These get
	// notified upon realisation of this class.
	ModuleObservers _observers;

public:

	// Constructor, allocates the library
	Doom3ShaderSystem();

	// This attaches this class as ModuleObserver to the Filesystem
	void construct();
	void destroy();

	// Gets called on initialise
	virtual void onFileSystemInitialise();

	// Gets called on shutdown
	virtual void onFileSystemShutdown();

	// greebo: This parses the material files and calls realise() on any
	// attached moduleobservers
	void realise();

	// greebo: Unrealises the attached ModuleObservers and frees the shaders
	void unrealise();

	// Flushes the shaders from memory and reloads the material files
	void refresh();

	// Is the shader system realised
	bool isRealised();

	// Return a shader by name
	MaterialPtr getMaterialForName(const std::string& name);

	bool materialExists(const std::string& name);

	void foreachShaderName(const ShaderNameCallback& callback);

	void activeShadersChangedNotify();

	// Enable or disable the active shaders callback
	void setActiveShaderUpdates(bool v) {
		_enableActiveUpdates = v;
	}

	void attach(ModuleObserver& observer);
	void detach(ModuleObserver& observer);

	void setLightingEnabled(bool enabled);

	const char* getTexturePrefix() const;

	/**
	 * greebo: Traverse all shaders using the given visitor class.
	 */
	void foreachShader(ShaderVisitor& visitor);

	/* greebo: Loads an image from disk and creates a basic shader
	 * object out of it (i.e. only diffuse and editor image are non-empty).
	 */
	TexturePtr loadTextureFromFile(const std::string& filename,
								   const std::string& moduleNames = "GDK");

	ShaderLibrary& getLibrary();
	GLTextureManager& getTextureManager();

    // Get default textures for D,B,S layers
    TexturePtr getDefaultInteractionTexture(ShaderLayer::Type t);

	IShaderExpressionPtr createShaderExpressionFromString(const std::string& exprStr);

	// Look up a table def, return NULL if not found
	TableDefinitionPtr getTableForName(const std::string& name);

	// Method for adding tables, returns FALSE if a def with the same name already exists
	bool addTableDefinition(const TableDefinitionPtr& def);

	// The "Flush & Reload Shaders" command target
	void refreshShadersCmd(const cmd::ArgumentList& args);

public:

	/** Load the shader definitions from the MTR files
	 * (doesn't load any textures yet).	*/
	void loadMaterialFiles();

	// Unloads all the existing shaders and calls activeShadersChangedNotify()
	void freeShaders();

	void addActiveShadersObserver(const ActiveShadersObserverPtr& observer);
	void removeActiveShadersObserver(const ActiveShadersObserverPtr& observer);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

private:
	void testShaderExpressionParsing();
}; // class Doom3ShaderSystem

typedef boost::shared_ptr<Doom3ShaderSystem> Doom3ShaderSystemPtr;

} // namespace shaders

shaders::Doom3ShaderSystemPtr GetShaderSystem();

shaders::ShaderLibrary& GetShaderLibrary();

shaders::GLTextureManager& GetTextureManager();
