#include "CShader.h"
#include "Doom3ShaderSystem.h"

#include "iregistry.h"
#include "ishaders.h"
#include "texturelib.h"
#include "parser/DefTokeniser.h"

#include <boost/lexical_cast.hpp>

/* CONSTANTS */
namespace {
	
	// Registry path for default light shader
	const std::string DEFAULT_LIGHT_PATH = "game/defaults/lightShader";	
	
}

namespace shaders 
{

/* Constructor. Sets the name and the ShaderDefinition to use.
 */
CShader::CShader(const std::string& name, const ShaderDefinition& definition) : 
	_template(definition.shaderTemplate),
	_fileName(definition.filename),
	_name(name),
	m_bInUse(false),
	_visible(true)
{
	// Realise the shader
	realise();
}

CShader::~CShader() {
	unrealise();
	GetTextureManager().checkBindings();
}

TexturePtr CShader::getEditorImage() 
{
    if (!_editorTexture) 
    {
        // Pass the call to the GLTextureManager to realise this image 
        _editorTexture = GetTextureManager().getBinding(
            _template->getEditorTexture()
        );
        if (!_editorTexture)
        {
            _editorTexture = GetTextureManager().getShaderNotFound();
        }
    }
	
    return _editorTexture;
}

// Return the falloff texture name
std::string CShader::getFalloffName() const {
	return _template->getLightFalloff()->getIdentifier();
}

/*
 * Return the light falloff texture (Z dimension).
 */
TexturePtr CShader::lightFalloffImage() {

	// Construct the texture if necessary
	if (!_texLightFalloff) {

		// Create image. If there is no falloff image defined, use the
		// default.
		if (_template->getLightFalloff()) {
			// create the image
			_texLightFalloff = GetTextureManager().getBinding(_template->getLightFalloff());
		}
		else {
			// Find the default light shader in the ShaderSystem and query its
			// falloff texture name.
			std::string defLight = GlobalRegistry().get(DEFAULT_LIGHT_PATH);
			MaterialPtr defLightShader = GetShaderSystem()->getMaterialForName(defLight);

			// Cast to a CShader so we can call getFalloffName().
			CShaderPtr cshaderPtr = boost::static_pointer_cast<CShader>(defLightShader);
			
			// create the image
			_texLightFalloff = GetTextureManager().getBinding(cshaderPtr->_template->getLightFalloff());
		}
	
	}
	// Return the texture
	return _texLightFalloff;
}



/*
 * Return name of shader.
 */
std::string CShader::getName() const 
{
	return _name;
}

std::string CShader::getDescription() const {
	return _template->getDescription();
}

bool CShader::IsInUse() const {
	return m_bInUse;
}

void CShader::SetInUse(bool bInUse) {
	m_bInUse = bInUse;
	GetShaderSystem()->activeShadersChangedNotify();
}

// get the shader flags
int CShader::getFlags() const {
	return _template->getFlags();
}

// get the transparency value
float CShader::getTrans() const {
	return _template->getTrans();
}

// test if it's a true shader, or a default shader created to wrap around a texture
bool CShader::IsDefault() const {
	return _fileName.empty();
}

// get the alphaFunc
void CShader::getAlphaFunc(EAlphaFunc *func, float *ref) {
	*func = _template->getAlphaFunc();
	*ref = _template->getAlphaRef();
};

// get the cull type
Material::ECull CShader::getCull() {
	return _template->getCull();
};

// get shader file name (ie the file where this one is defined)
const char* CShader::getShaderFileName() const {
	return _fileName.c_str();
}

std::string CShader::getDefinition()
{
	return _template->getBlockContents();
}

// -----------------------------------------

void CShader::realise() {
	realiseLighting();
}

void CShader::unrealise() {
	unrealiseLighting();
}

// Parse and load image maps for this shader
void CShader::realiseLighting() 
{
    // Only realises extra layers (no diffuse/bump/specular)
	for (ShaderTemplate::Layers::const_iterator i = _template->getLayers().begin();
	        i != _template->getLayers().end();
	        ++i)
	{
		_layers.push_back(*i);
	}
}

void CShader::unrealiseLighting() 
{
	_layers.clear();
}

/*
 * Set name of shader.
 */
void CShader::setName(const std::string& name) {
	_name = name;
}

const ShaderLayer* CShader::firstLayer() const {
	if (_layers.empty()) {
		return 0;
	}
	return _layers.front().get();
}

// Get all layers
const ShaderLayerVector& CShader::getAllLayers() const
{
    return _layers;
}

/* Required Material light type predicates */

bool CShader::isAmbientLight() const {
	return _template->isAmbientLight();
}

bool CShader::isBlendLight() const {
	return _template->isBlendLight();
}

bool CShader::isFogLight() const {
	return _template->isFogLight();
}

bool CShader::isVisible() const {
	return _visible;
}

void CShader::setVisible(bool visible) {
	_visible = visible;
}

bool CShader::m_lightingEnabled = false;

} // namespace shaders
