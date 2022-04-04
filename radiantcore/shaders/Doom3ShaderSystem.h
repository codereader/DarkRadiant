#pragma once

#include "ishaders.h"
#include "ifilesystem.h"
#include "imodule.h"
#include "iradiant.h"
#include "icommandsystem.h"

#include <functional>

#include "ShaderLibrary.h"
#include "ShaderFileLoader.h"
#include "TableDefinition.h"
#include "textures/GLTextureManager.h"

namespace shaders
{

/**
 * \brief
 * Implementation of the MaterialManager for Doom 3 .
 */
class Doom3ShaderSystem :
	public MaterialManager,
	public vfs::VirtualFileSystem::Observer
{
	// The shaderlibrary stores all the known shaderdefinitions
	// as well as the active shaders
	ShaderLibraryPtr _library;

    // The ShaderFileLoader will provide a new ShaderLibrary once complete
    std::unique_ptr<ShaderFileLoader> _defLoader;

	// The manager that handles the texture caching.
	GLTextureManagerPtr _textureManager;

	// Active shaders list changed signal
    sigc::signal<void> _signalActiveShadersChanged;

	// Flag to indicate whether the active shaders callback should be invoked
	bool _enableActiveUpdates;

	// TRUE if the material files have been parsed
	bool _realised;

	// Signals for module subscribers
	sigc::signal<void> _signalDefsLoaded;
	sigc::signal<void> _signalDefsUnloaded;

    sigc::signal<void, const std::string&> _sigMaterialCreated;
    sigc::signal<void, const std::string&, const std::string&> _sigMaterialRenamed;
    sigc::signal<void, const std::string&> _sigMaterialRemoved;

public:

	// Constructor, allocates the library
	Doom3ShaderSystem();

	// Gets called on initialise
	void onFileSystemInitialise() override;

	// Gets called on shutdown
    void onFileSystemShutdown() override;

	// greebo: This parses the material files and emits the defs loaded signal
    void realise() override;

	// greebo: Emits the defs unloaded signal and frees the shaders
    void unrealise() override;

	// Flushes the shaders from memory and reloads the material files
    void refresh() override;

	// Is the shader system realised
    bool isRealised() override;

	sigc::signal<void>& signal_DefsLoaded() override;
	sigc::signal<void>& signal_DefsUnloaded() override;

    sigc::signal<void, const std::string&>& signal_materialCreated() override;
    sigc::signal<void, const std::string&, const std::string&>& signal_materialRenamed() override;
    sigc::signal<void, const std::string&>& signal_materialRemoved() override;

	// Return a shader by name
    MaterialPtr getMaterial(const std::string& name) override;

    bool materialExists(const std::string& name) override;
    bool materialCanBeModified(const std::string& name) override;

    void foreachShaderName(const ShaderNameCallback& callback) override;

	void activeShadersChangedNotify();

	// Enable or disable the active shaders callback
	void setActiveShaderUpdates(bool v) override {
		_enableActiveUpdates = v;
	}

    void setLightingEnabled(bool enabled) override;

    const char* getTexturePrefix() const override;

	/**
	 * greebo: Traverse all shaders using the given visitor class.
	 */
    void foreachMaterial(const std::function<void(const MaterialPtr&)>& func) override;

	/* greebo: Loads an image from disk and creates a basic shader
	 * object out of it (i.e. only diffuse and editor image are non-empty).
	 */
    TexturePtr loadTextureFromFile(const std::string& filename) override;

	GLTextureManager& getTextureManager();

    // Get default textures for D,B,S layers
    TexturePtr getDefaultInteractionTexture(IShaderLayer::Type t) override;

    IShaderExpression::Ptr createShaderExpressionFromString(const std::string& exprStr) override;

    MaterialPtr createEmptyMaterial(const std::string& name) override;

    MaterialPtr createDefaultMaterial(const std::string& name) override;
    MaterialPtr copyMaterial(const std::string& nameOfOriginal, const std::string& nameOfCopy) override;
    bool renameMaterial(const std::string& oldName, const std::string& newName) override;
    void removeMaterial(const std::string& name) override;
    void saveMaterial(const std::string& name) override;

	// Look up a table def, return NULL if not found
	ITableDefinition::Ptr getTable(const std::string& name);

public:
    sigc::signal<void> signal_activeShadersChanged() const override;

	// RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    // This attaches this class as Observer to the Filesystem
    void construct();
    void destroy();

    // For methods accessing the ShaderLibrary the parser thread must be done
    void ensureDefsLoaded();

    // Unloads all the existing shaders and calls activeShadersChangedNotify()
    void freeShaders();

	void testShaderExpressionParsing();

    std::string ensureNonConflictingName(const std::string& name);
};

typedef std::shared_ptr<Doom3ShaderSystem> Doom3ShaderSystemPtr;

Doom3ShaderSystemPtr GetShaderSystem();

GLTextureManager& GetTextureManager();

} // namespace shaders
