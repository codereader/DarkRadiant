#ifndef DOOM3SHADERSYSTEM_H_
#define DOOM3SHADERSYSTEM_H_

#include "ishaders.h"
#include "moduleobserver.h"

#include "generic/callback.h"
#include "moduleobservers.h"

#include "ShaderLibrary.h"
#include "textures/GLTextureManager.h"

namespace shaders {

class Doom3ShaderSystem : 
	public ShaderSystem, 
	public ModuleObserver
{
	// The shaderlibrary stores all the known shaderdefinitions 
	// as well as the active shaders
	ShaderLibraryPtr _library;
	
	// The manager that handles the texture caching. 
	GLTextureManagerPtr _textureManager;
	
	// greebo: Legacy callback (points to the TextureBrowser)
	Callback _activeShadersChangedNotify;
	
	// greebo: Legacy "unrealised" counter, gets 0 as soon as the
	// global filesystem calls the realise() method of this class,
	// (This ShaderSystem acts as ModuleObserver)
	std::size_t _shadersUnrealised;
	
	// The observers that are attached to this system. These get
	// notified upon realisation of this class.
	ModuleObservers _observers;
	
public:
	// Constructor, allocates the library
	Doom3ShaderSystem();
	
	// Destructor, removes the library from heap
	~Doom3ShaderSystem();

	// This attaches this class as ModuleObserver to the Filesystem
	void construct();
	void destroy();

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
	IShaderPtr getShaderForName(const std::string& name);

	void foreachShaderName(const ShaderNameCallback& callback);

	void beginActiveShadersIterator();
	bool endActiveShadersIterator();
	IShaderPtr dereferenceActiveShadersIterator();
	void incrementActiveShadersIterator();
	
	void setActiveShadersChangedNotify(const Callback& notify);
	
	void attach(ModuleObserver& observer);
	void detach(ModuleObserver& observer);

	void setLightingEnabled(bool enabled);

	const char* getTexturePrefix() const;
	
	/* greebo: Loads an image from disk and creates a basic shader
	 * object out of it (i.e. only diffuse and editor image are non-empty).
	 */
	TexturePtr loadTextureFromFile(const std::string& filename);

	ShaderLibrary& getLibrary();
	GLTextureManager& getTextureManager();

public:
	/** Load the shader definitions from the MTR files 
	 * (doesn't load any textures yet).	*/
	void loadMaterialFiles();

	// Unloads all the existing shaders and calls activeShadersChangedNotify()
	void freeShaders();
	
	// greebo: Legacy method, don't know what this is exactly used for
	void activeShadersChangedNotify();
	
}; // class Doom3ShaderSystem

} // namespace shaders

// Accessor function for the shader system
shaders::Doom3ShaderSystem& GetShaderSystem();

shaders::ShaderLibrary& GetShaderLibrary();

shaders::GLTextureManager& GetTextureManager();

#endif /*DOOM3SHADERSYSTEM_H_*/
