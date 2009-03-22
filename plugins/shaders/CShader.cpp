#include "CShader.h"
#include "Doom3ShaderSystem.h"

#include "iregistry.h"
#include "ishaders.h"
#include "texturelib.h"
#include "parser/DefTokeniser.h"

#include <boost/lexical_cast.hpp>

/* CONSTANTS */
namespace {
	
	// Default image maps for optional material stages
	const std::string IMAGE_FLAT = "_flat.bmp";
	const std::string IMAGE_BLACK = "_black.bmp";
	
	// Registry path for default light shader
	const std::string DEFAULT_LIGHT_PATH = "game/defaults/lightShader";	
	
}

namespace shaders 
{

// Map string blend functions to their GLenum equivalents
GLenum glBlendFromString(const std::string& value) 
{
	if (value == "gl_zero") {
		return GL_ZERO;
	}
	if (value == "gl_one") {
		return GL_ONE;
	}
	if (value == "gl_src_color") {
		return GL_SRC_COLOR;
	}
	if (value == "gl_one_minus_src_color") {
		return GL_ONE_MINUS_SRC_COLOR;
	}
	if (value == "gl_src_alpha") {
		return GL_SRC_ALPHA;
	}
	if (value == "gl_one_minus_src_alpha") {
		return GL_ONE_MINUS_SRC_ALPHA;
	}
	if (value == "gl_dst_color") {
		return GL_DST_COLOR;
	}
	if (value == "gl_one_minus_dst_color") {
		return GL_ONE_MINUS_DST_COLOR;
	}
	if (value == "gl_dst_alpha") {
		return GL_DST_ALPHA;
	}
	if (value == "gl_one_minus_dst_alpha") {
		return GL_ONE_MINUS_DST_ALPHA;
	}
	if (value == "gl_src_alpha_saturate") {
		return GL_SRC_ALPHA_SATURATE;
	}

	return GL_ZERO;
}

// Convert a string pair describing a blend function into a BlendFunc object
BlendFunc blendFuncFromStrings(const StringPair& blendFunc) 
{
    // Handle predefined blend modes first: add, modulate, filter
    if (blendFunc.first == "add")
    {
        return BlendFunc(GL_ONE, GL_ONE);
    }
    else if (blendFunc.first == "modulate" || blendFunc.first == "filter")
    {
        return BlendFunc(GL_ZERO, GL_SRC_COLOR);
    }
    else
    {
        // Not predefined, just use the specified blend function directly
        return BlendFunc(
            glBlendFromString(blendFunc.first),
            glBlendFromString(blendFunc.second)
        );
    }
}

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
		_editorTexture = GetTextureManager().getBinding(_template->getTexture());
	}
	
	return _editorTexture;
}

TexturePtr CShader::getDiffuse() 
{
	// If we have no diffuse texture but the template contains a map expression,
    // get a texture binding for it
	if (!_diffuse && _template->getDiffuse()) 
    {
		// Pass the call to the GLTextureManager to realise this image 
		_diffuse = GetTextureManager().getBinding(_template->getDiffuse()); 
	}

	return _diffuse;
}

// Return bumpmap if it exists, otherwise _flat
TexturePtr CShader::getBump() {
	
	// Check if the boost::shared_ptr is still uninitialised
	if (!_bump) {
	
		// Create image. If the bump map is not set, we need to use the
		// flat image here
		if (_template->getBump()) {
			_bump = GetTextureManager().getBinding(_template->getBump());
		}
		else {
			_bump = GetTextureManager().getBinding(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_FLAT);
		}
	}
	return _bump;
}

// Get the specular texture
TexturePtr CShader::getSpecular() {

	// Check if the boost::shared_ptr is still uninitialised
	if (!_specular) {
		
		// Create image. If the specular map is not set, we need to use 
		// the _black image here
		if (_template->getSpecular()) {
			_specular = GetTextureManager().getBinding(_template->getSpecular());
		}
		else {
			// Create a Black MapExpression
			_specular = GetTextureManager().getBinding(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_BLACK);
		}
	}
	return _specular;
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
			IShaderPtr defLightShader = GetShaderSystem()->getShaderForName(defLight);

			// Cast to a CShader so we can call getFalloffName().
			ShaderPtr cshaderPtr = boost::static_pointer_cast<CShader>(defLightShader);
			
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
IShader::ECull CShader::getCull() {
	return _template->getCull();
};

// get shader file name (ie the file where this one is defined)
const char* CShader::getShaderFileName() const {
	return _fileName.c_str();
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
	for (ShaderTemplate::Layers::const_iterator i = _template->getLayers().begin();
	        i != _template->getLayers().end();
	        ++i)
	{
		_layers.push_back(getShaderLayerFromTemplate(*i));
	}

#if 0
	if (_layers.size() == 1) {
		const BlendFuncExpression& blendFunc =
		    _template->getLayers().front().m_blendFunc;

		// If explicit blend function (2-components), evaluate it, otherwise
		// use a standard one
		if (!blendFunc.second.empty()) {
			m_blendFunc = BlendFunc(glBlendFromString(blendFunc.first),
			                        glBlendFromString(blendFunc.second));
		}
		else {
			if (blendFunc.first == "add") {
				m_blendFunc = BlendFunc(GL_ONE, GL_ONE);
			}
			else if (blendFunc.first == "filter") {
				m_blendFunc = BlendFunc(GL_DST_COLOR, GL_ZERO);
			}
			else if (blendFunc.first == "blend") {
				m_blendFunc = BlendFunc(GL_SRC_ALPHA,
				                        GL_ONE_MINUS_SRC_ALPHA);
			}
		}
	}
#endif
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
	return &_layers.front();
}

// Get all layers
const ShaderLayerVector& CShader::getAllLayers() const
{
    return _layers;
}

/* Required IShader light type predicates */

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

ShaderLayer CShader::getShaderLayerFromTemplate(const LayerTemplate& layerTemp) 
{
    // Construct a ShaderLayer and copy across all parameters from the template.
    ShaderLayer layer(
		GetTextureManager().getBinding(layerTemp.mapExpr),
		blendFuncFromStrings(layerTemp.blendFunc),
		layerTemp.m_clampToBorder,
		boost::lexical_cast<float>(layerTemp.m_alphaTest)
	);
    layer.colour = layerTemp.colour;
    layer.vertexColourMode = layerTemp.vertexColourMode;

    return layer;
}

bool CShader::m_lightingEnabled = false;

} // namespace shaders
