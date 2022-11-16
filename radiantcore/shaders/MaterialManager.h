#pragma once

#include "ishaders.h"
#include "imodule.h"

#include <functional>

#include "ShaderLibrary.h"
#include "textures/GLTextureManager.h"

namespace shaders
{

/**
 * \brief
 * Implementation of the IMaterialManager for Doom 3 .
 */
class MaterialManager :
	public IMaterialManager
{
	// The shaderlibrary stores all the known templates
	// as well as the active shaders
	ShaderLibraryPtr _library;

	// The manager that handles the texture caching.
	GLTextureManagerPtr _textureManager;

	// Active shaders list changed signal
    sigc::signal<void> _signalActiveShadersChanged;

	// Flag to indicate whether the active shaders callback should be invoked
	bool _enableActiveUpdates;

    sigc::signal<void, const std::string&> _sigMaterialCreated;
    sigc::signal<void, const std::string&, const std::string&> _sigMaterialRenamed;
    sigc::signal<void, const std::string&> _sigMaterialRemoved;

    sigc::connection _materialsReloadedSignal;

public:
    MaterialManager();

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

    MaterialPtr copyMaterial(const std::string& nameOfOriginal, const std::string& nameOfCopy) override;
    bool renameMaterial(const std::string& oldName, const std::string& newName) override;
    void removeMaterial(const std::string& name) override;
    void saveMaterial(const std::string& name) override;

	// Look up a table def, return NULL if not found
	ITableDefinition::Ptr getTable(const std::string& name) override;

    void reloadImages() override;

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

    // Unloads all the existing shaders and calls activeShadersChangedNotify()
    void freeShaders();

    void onMaterialDefsReloaded();
};

typedef std::shared_ptr<MaterialManager> MaterialManagerPtr;

MaterialManagerPtr GetShaderSystem();

GLTextureManager& GetTextureManager();

} // namespace shaders
