#include "Doom3ShaderSystem.h"

#include "shaders.h"
#include "ShaderDefinition.h"

// TODO: remove this as soon as shaderlibrary is fully functional
extern ShaderDefinitionMap g_shaderDefinitions;

#include "generic/referencecounted.h"

typedef SmartPointer<CShader> ShaderPointer;
typedef std::map<std::string, ShaderPointer> shaders_t;
extern shaders_t g_ActiveShaders;

#include "moduleobservers.h"
extern ModuleObservers g_observers;

extern std::size_t g_shaders_unrealised;
// end TODO

namespace {
	const char* TEXTURE_PREFIX = "textures/";
}

namespace shaders {

// Constructor
Doom3ShaderSystem::Doom3ShaderSystem() :
	_library(new ShaderLibrary()),
	_textureManager(new GLTextureManager())
{}

// Destructor
Doom3ShaderSystem::~Doom3ShaderSystem()
{}

void Doom3ShaderSystem::realise() {
	Shaders_Realise();
}
void Doom3ShaderSystem::unrealise() {
	Shaders_Unrealise();
}
void Doom3ShaderSystem::refresh() {
	Shaders_Refresh();
}

// Is the shader system realised
bool Doom3ShaderSystem::isRealised() {
	return g_shaders_unrealised == 0;
}

// Return a shader by name
IShader* Doom3ShaderSystem::getShaderForName(const std::string& name) {
	ShaderPtr shader = _library->findShader(name);
	IShader *pShader = Try_Shader_ForName(name.c_str());
	pShader->IncRef();
	return pShader;
}

void Doom3ShaderSystem::foreachShaderName(const ShaderNameCallback& callback) {
	for (ShaderDefinitionMap::const_iterator i = g_shaderDefinitions.begin(); i != g_shaderDefinitions.end(); ++i) {
		callback((*i).first.c_str());
	}
}

void Doom3ShaderSystem::beginActiveShadersIterator() {
	ActiveShaders_IteratorBegin();
}
bool Doom3ShaderSystem::endActiveShadersIterator() {
	return ActiveShaders_IteratorAtEnd();
}
IShader* Doom3ShaderSystem::dereferenceActiveShadersIterator() {
	return ActiveShaders_IteratorCurrent();
}
void Doom3ShaderSystem::incrementActiveShadersIterator() {
	ActiveShaders_IteratorIncrement();
}
void Doom3ShaderSystem::setActiveShadersChangedNotify(const Callback& notify) {
	g_ActiveShadersChangedNotify = notify;
}

void Doom3ShaderSystem::attach(ModuleObserver& observer) {
	g_observers.attach(observer);
}
void Doom3ShaderSystem::detach(ModuleObserver& observer) {
	g_observers.detach(observer);
}

void Doom3ShaderSystem::setLightingEnabled(bool enabled) {
	if (CShader::m_lightingEnabled != enabled) {
		for (shaders_t::const_iterator i = g_ActiveShaders.begin(); i != g_ActiveShaders.end(); ++i) {
			(*i).second->unrealiseLighting();
		}
		CShader::m_lightingEnabled = enabled;
		for (shaders_t::const_iterator i = g_ActiveShaders.begin(); i != g_ActiveShaders.end(); ++i) {
			(*i).second->realiseLighting();
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

} // namespace shaders

// Accessor function encapsulating the static shadersystem instance
shaders::Doom3ShaderSystem& GetShaderSystem() {
	static shaders::Doom3ShaderSystem _shaderSystem;
	return _shaderSystem;
}

shaders::ShaderLibrary& GetShaderLibrary() {
	return GetShaderSystem().getLibrary();
}

shaders::GLTextureManager& GetTextureManager() {
	return GetShaderSystem().getTextureManager();
}
