#ifndef DOOM3SHADERSYSTEM_H_
#define DOOM3SHADERSYSTEM_H_

#include "ishaders.h"
#include "moduleobserver.h"

#include "generic/callback.h"

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
	
	Callback _activeShadersChangedNotify;
	
public:
	// Constructor, allocates the library
	Doom3ShaderSystem();
	
	// Destructor, removes the library from heap
	~Doom3ShaderSystem();

	void realise();
	void unrealise();
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

	ShaderLibrary& getLibrary();
	GLTextureManager& getTextureManager();

	// greebo: Legacy method, don't know what this is exactly used for
	void activeShadersChangedNotify();
	
}; // class Doom3ShaderSystem

} // namespace shaders

// Accessor function for the shader system
shaders::Doom3ShaderSystem& GetShaderSystem();

shaders::ShaderLibrary& GetShaderLibrary();

shaders::GLTextureManager& GetTextureManager();

#endif /*DOOM3SHADERSYSTEM_H_*/
