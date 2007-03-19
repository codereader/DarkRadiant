#include "CShader.h"
#include "Doom3ShaderSystem.h"
#include "textures/DefaultConstructor.h"
#include "textures/FileLoader.h"

#include "ishaders.h"
#include "texturelib.h"

/* CONSTANTS */
namespace {
	
	// Default image maps for optional material stages
	const std::string IMAGE_FLAT = "_flat.bmp";
	const std::string IMAGE_BLACK = "_black.bmp";
	
	// Registry path for default light shader
	const std::string DEFAULT_LIGHT_PATH = "game/defaults/lightShader";	
	
}

// Map string blend functions to their enum equivalents
BlendFactor evaluateBlendFactor(const std::string& value) {
	if (value == "gl_zero") {
		return BLEND_ZERO;
	}
	if (value == "gl_one") {
		return BLEND_ONE;
	}
	if (value == "gl_src_color") {
		return BLEND_SRC_COLOUR;
	}
	if (value == "gl_one_minus_src_color") {
		return BLEND_ONE_MINUS_SRC_COLOUR;
	}
	if (value == "gl_src_alpha") {
		return BLEND_SRC_ALPHA;
	}
	if (value == "gl_one_minus_src_alpha") {
		return BLEND_ONE_MINUS_SRC_ALPHA;
	}
	if (value == "gl_dst_color") {
		return BLEND_DST_COLOUR;
	}
	if (value == "gl_one_minus_dst_color") {
		return BLEND_ONE_MINUS_DST_COLOUR;
	}
	if (value == "gl_dst_alpha") {
		return BLEND_DST_ALPHA;
	}
	if (value == "gl_one_minus_dst_alpha") {
		return BLEND_ONE_MINUS_DST_ALPHA;
	}
	if (value == "gl_src_alpha_saturate") {
		return BLEND_SRC_ALPHA_SATURATE;
	}

	return BLEND_ZERO;
}

///\todo BlendFunc parsing
BlendFunc evaluateBlendFunc(const BlendFuncExpression& blendFunc) {
	return BlendFunc(BLEND_ONE, BLEND_ZERO);
}

/* Constructor. Sets the name and the ShaderDefinition to use.
 */
CShader::CShader(const std::string& name, const ShaderDefinition& definition) : 
	_template(*definition.shaderTemplate),
	_fileName(definition.filename),
	_name(name),
	m_blendFunc(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA),
	m_bInUse(false) 
{
	// Obtain texture names from the template
	_editorTexName = _template._texture->getTextureName();
	_diffuseName = _template._diffuse->getTextureName();
	_bumpName = _template._bump->getTextureName();
	_specularName = _template._specular->getTextureName();
	_falloffName = _template._lightFallOff->getTextureName();

	// Realise the shader
	realise();
}

CShader::~CShader() {
	unrealise();
	GetTextureManager().checkBindings();
}

// get/set the Texture* Radiant uses to represent this shader object
TexturePtr CShader::getTexture() {

	// Check if the boost::shared_ptr is still uninitialised
	if (!_editorTexture) {
		
		// Create constructor
		TextureConstructorPtr cons(new DefaultConstructor(_editorTexName));
		
		// Pass the call to the GLTextureManager to realise this image 
		_editorTexture = GetTextureManager().getBinding(_editorTexName,	cons);
	}
	
	return _editorTexture;
}

TexturePtr CShader::getDiffuse() {

	// Check if the boost::shared_ptr is still uninitialised
	if (!_diffuse) {
		
		// Create constructor
		TextureConstructorPtr cons(new DefaultConstructor(_diffuseName));
		
		// Pass the call to the GLTextureManager to realise this image 
		_diffuse = GetTextureManager().getBinding(_diffuseName, cons); 
	}

	return _diffuse;
}

// Return bumpmap if it exists, otherwise _flat
TexturePtr CShader::getBump() {
	
	// Check if the boost::shared_ptr is still uninitialised
	if (!_bump) {
	
		// Create constructor. If the bump map is not set, we need to use the
		// flat image here
		TextureConstructorPtr cons;
		if (!_bumpName.empty()) {
			cons = TextureConstructorPtr(new DefaultConstructor(_bumpName));
		}
		else {
			// Create a FileLoader for the _flat.bmp local image. We also set
			// the bump name so that the correct caching key is passed to the
			// GLTextureManager ("" would return SHADER NOT FOUND).
			_bumpName = IMAGE_FLAT;
			cons = TextureConstructorPtr(
				new FileLoader(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_FLAT, "bmp")
			);
		}
		
		// Pass the call to the GLTextureManager to realise this image 
		_bump = GetTextureManager().getBinding(_bumpName, cons);
	}
	
	return _bump;
}

// Get the specular texture
TexturePtr CShader::getSpecular() {

	// Check if the boost::shared_ptr is still uninitialised
	if (!_specular) {
		
		// Create constructor. If the specular map is not set, we need to use 
		// the _black image here
		TextureConstructorPtr cons;
		if (!_specularName.empty()) {
			cons = TextureConstructorPtr(new DefaultConstructor(_specularName));
		}
		else {
			// Load the _black image
			_specularName = IMAGE_BLACK;
			cons = TextureConstructorPtr(
				new FileLoader(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_BLACK, "bmp")
			);
		}

		// Pass the call to the GLTextureManager to realise this image 
		_specular = GetTextureManager().getBinding(_specularName, cons); 
	}

	return _specular;
}

// Return the falloff texture name
std::string CShader::getFalloffName() const {
	return _falloffName;
}

/*
 * Return the light falloff texture (Z dimension).
 */
TexturePtr CShader::lightFalloffImage() {

	// Construct the texture if necessary
	if (!_texLightFalloff) {

		// Create constructor. If there is no falloff image defined, use the
		// default.
		if (_falloffName.empty()) {
			
			// Find the default light shader in the ShaderSystem and query its
			// falloff texture name.
			std::string defLight = GlobalRegistry().get(DEFAULT_LIGHT_PATH);
			IShaderPtr defLightShader = GetShaderSystem().getShaderForName(
											defLight);
			
			// Cast to a CShader so we can call getFalloffName().
			boost::shared_ptr<CShader> cshaderPtr = 
				boost::static_pointer_cast<CShader>(defLightShader);
				
			// Set the falloff name
			_falloffName = cshaderPtr->getFalloffName();
		}
		
		// Create texture constructor
		TextureConstructorPtr cons(new DefaultConstructor(_falloffName));

		// Pass the call to the GLTextureManager to realise this image 
		_texLightFalloff = GetTextureManager().getBinding(_falloffName, cons); 
	}
	
	// Return the texture
	return _texLightFalloff;
}



/*
 * Return name of shader.
 */
const char* CShader::getName() const {
	return _name.c_str();
}

bool CShader::IsInUse() const {
	return m_bInUse;
}

void CShader::SetInUse(bool bInUse) {
	m_bInUse = bInUse;
	GetShaderSystem().activeShadersChangedNotify();
}

// get the shader flags
int CShader::getFlags() const {
	return _template.m_nFlags;
}

// get the transparency value
float CShader::getTrans() const {
	return _template.m_fTrans;
}

// test if it's a true shader, or a default shader created to wrap around a texture
bool CShader::IsDefault() const {
	return _fileName.empty();
}

// get the alphaFunc
void CShader::getAlphaFunc(EAlphaFunc *func, float *ref) {
	*func = _template.m_AlphaFunc;
	*ref = _template.m_AlphaRef;
};

BlendFunc CShader::getBlendFunc() const {
	return m_blendFunc;
}

// get the cull type
IShader::ECull CShader::getCull() {
	return _template.m_Cull;
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
void CShader::realiseLighting() {

	for (ShaderTemplate::Layers::const_iterator i = _template.m_layers.begin();
	        i != _template.m_layers.end();
	        ++i) {
		m_layers.push_back(evaluateLayer(*i));
	}

	if (m_layers.size() == 1) {
		const BlendFuncExpression& blendFunc =
		    _template.m_layers.front().m_blendFunc;

		// If explicit blend function (2-components), evaluate it, otherwise
		// use a standard one
		if (!blendFunc.second.empty()) {
			m_blendFunc = BlendFunc(evaluateBlendFactor(blendFunc.first),
			                        evaluateBlendFactor(blendFunc.second));
		}
		else {
			if (blendFunc.first == "add") {
				m_blendFunc = BlendFunc(BLEND_ONE, BLEND_ONE);
			}
			else if (blendFunc.first == "filter") {
				m_blendFunc = BlendFunc(BLEND_DST_COLOUR, BLEND_ZERO);
			}
			else if (blendFunc.first == "blend") {
				m_blendFunc = BlendFunc(BLEND_SRC_ALPHA,
				                        BLEND_ONE_MINUS_SRC_ALPHA);
			}
		}
	}
}

void CShader::unrealiseLighting() {
	m_layers.clear();
	m_blendFunc = BlendFunc(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
}

/*
 * Set name of shader.
 */
void CShader::setName(const std::string& name) {
	_name = name;
}

CShader::MapLayer CShader::evaluateLayer(const LayerTemplate& layerTemplate) {
	
	// Allocate a default TextureConstructor with this name
	TextureConstructorPtr constructor(
	    new DefaultConstructor(layerTemplate.mapExpr->getTextureName())
	);

	return MapLayer(
		GetTextureManager().getBinding(
			layerTemplate.mapExpr->getTextureName(),
        	constructor
        ),
		evaluateBlendFunc(layerTemplate.m_blendFunc),
		layerTemplate.m_clampToBorder,
		boost::lexical_cast<float>(layerTemplate.m_alphaTest)
	);
}

const ShaderLayer* CShader::firstLayer() const {
	if (m_layers.empty()) {
		return 0;
	}
	return &m_layers.front();
}

void CShader::forEachLayer(const ShaderLayerCallback& callback) const {
	for (MapLayers::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
		callback(*i);
	}
}

/* Required IShader light type predicates */

bool CShader::isAmbientLight() const {
	return _template.ambientLight;
}

bool CShader::isBlendLight() const {
	return _template.blendLight;
}

bool CShader::isFogLight() const {
	return _template.fogLight;
}

bool CShader::m_lightingEnabled = false;
