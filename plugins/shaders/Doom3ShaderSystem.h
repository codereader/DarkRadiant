#pragma once

#include "ishaders.h"
#include "ifilesystem.h"
#include "imodule.h"
#include "iradiant.h"
#include "icommandsystem.h"

#include <future>
#include <functional>
#include "moduleobservers.h"

#include "ShaderLibrary.h"
#include "TableDefinition.h"
#include "textures/GLTextureManager.h"

namespace shaders 
{

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

    // The ShaderFileLoader will provide a new ShaderLibrary once complete
    std::future<ShaderLibraryPtr> _loadResult;
    bool _defsLoaded;

	// The manager that handles the texture caching.
	GLTextureManagerPtr _textureManager;

	// Active shaders list changed signal
    sigc::signal<void> _signalActiveShadersChanged;

	// Flag to indicate whether the active shaders callback should be invoked
	bool _enableActiveUpdates;

	// TRUE if the material files have been parsed
	bool _realised;

	// The observers that are attached to this system. These get
	// notified upon realisation of this class.
	ModuleObservers _observers;

	// Used to provide feedback to the user during long operations
	ILongRunningOperation* _currentOperation;

public:

	// Constructor, allocates the library
	Doom3ShaderSystem();

	// Gets called on initialise
	void onFileSystemInitialise() override;

	// Gets called on shutdown
    void onFileSystemShutdown() override;

	// greebo: This parses the material files and calls realise() on any
	// attached moduleobservers
    void realise() override;

	// greebo: Unrealises the attached ModuleObservers and frees the shaders
    void unrealise() override;

	// Flushes the shaders from memory and reloads the material files
    void refresh() override;

	// Is the shader system realised
    bool isRealised() override;

	// Return a shader by name
    MaterialPtr getMaterialForName(const std::string& name) override;

    bool materialExists(const std::string& name) override;

    void foreachShaderName(const ShaderNameCallback& callback) override;

	void activeShadersChangedNotify();

	// Enable or disable the active shaders callback
	void setActiveShaderUpdates(bool v) override {
		_enableActiveUpdates = v;
	}

	void attach(ModuleObserver& observer) override;
	void detach(ModuleObserver& observer) override;

    void setLightingEnabled(bool enabled) override;

    const char* getTexturePrefix() const override;

	/**
	 * greebo: Traverse all shaders using the given visitor class.
	 */
    void foreachShader(ShaderVisitor& visitor) override;

	/* greebo: Loads an image from disk and creates a basic shader
	 * object out of it (i.e. only diffuse and editor image are non-empty).
	 */
    TexturePtr loadTextureFromFile(const std::string& filename) override;

	GLTextureManager& getTextureManager();

    // Get default textures for D,B,S layers
    TexturePtr getDefaultInteractionTexture(ShaderLayer::Type t) override;

    IShaderExpressionPtr createShaderExpressionFromString(const std::string& exprStr) override;

	// Look up a table def, return NULL if not found
	TableDefinitionPtr getTableForName(const std::string& name);

public:
    sigc::signal<void> signal_activeShadersChanged() const override;

	// RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const ApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    // This attaches this class as ModuleObserver to the Filesystem
    void construct();
    void destroy();

    // For methods accessing the ShaderLibrary the parser thread must be done
    void ensureDefsLoaded();

    // The "Flush & Reload Shaders" command target
    void refreshShadersCmd(const cmd::ArgumentList& args);

    // Unloads all the existing shaders and calls activeShadersChangedNotify()
    void freeShaders();

    /** Load the shader definitions from the MTR files
    * (doesn't load any textures yet).	*/
    ShaderLibraryPtr loadMaterialFiles();

	void testShaderExpressionParsing();
}; // class Doom3ShaderSystem

typedef std::shared_ptr<Doom3ShaderSystem> Doom3ShaderSystemPtr;

Doom3ShaderSystemPtr GetShaderSystem();

GLTextureManager& GetTextureManager();

} // namespace shaders
