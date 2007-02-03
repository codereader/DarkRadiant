#include "CShader.h"

#include "ishaders.h"
#include "itextures.h"

#include "texturelib.h"

#include "textures/DefaultConstructor.h"
#include "textures/FileLoader.h"

Callback g_ActiveShadersChangedNotify;

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
CShader::CShader(const std::string& name, const ShaderDefinition& definition)
		: _nRef(0),
		m_template(*definition.shaderTemplate),
		m_filename(definition.filename),
		_name(name),
		m_blendFunc(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA),
		m_bInUse(false) {
	assert(definition.shaderTemplate != NULL); // otherwise we have NULL ref

	// Realise the shader
	realise();
}

CShader::~CShader() {
	unrealise();
}

// Increase reference count
void CShader::IncRef() {
	++_nRef;
}

// Decrease reference count
void CShader::DecRef()  {
	if (--_nRef == 0) {
		delete this;
	}
}

std::size_t CShader::refcount() {
	return _nRef;
}

// get/set the Texture* Radiant uses to represent this shader object
TexturePtr CShader::getTexture() const {
	return m_pTexture;
}

TexturePtr CShader::getDiffuse() const {
	return m_pDiffuse;
}

// Return bumpmap if it exists, otherwise _flat
TexturePtr CShader::getBump() const {
	return m_pBump;
}

TexturePtr CShader::getSpecular() const {
	return m_pSpecular;
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
	g_ActiveShadersChangedNotify();
}

// get the shader flags
int CShader::getFlags() const {
	return m_template.m_nFlags;
}

// get the transparency value
float CShader::getTrans() const {
	return m_template.m_fTrans;
}

// test if it's a true shader, or a default shader created to wrap around a texture
bool CShader::IsDefault() const {
	return m_filename.empty();
}

// get the alphaFunc
void CShader::getAlphaFunc(EAlphaFunc *func, float *ref) {
	*func = m_template.m_AlphaFunc;
	*ref = m_template.m_AlphaRef;
};

BlendFunc CShader::getBlendFunc() const {
	return m_blendFunc;
}

// get the cull type
IShader::ECull CShader::getCull() {
	return m_template.m_Cull;
};

// get shader file name (ie the file where this one is defined)
const char* CShader::getShaderFileName() const {
	return m_filename.c_str();
}

// -----------------------------------------

void CShader::realise() {

	// Grab the display texture (may be null)
	std::string displayTex = m_template._texture->getTextureName();

	// Allocate a default ImageConstructor with this name
	ImageConstructorPtr imageConstructor(new DefaultConstructor(displayTex));

	m_pTexture = GlobalTexturesCache().capture(imageConstructor, displayTex);

	// Has the texture been successfully realised?
	if (m_pTexture->texture_number == 0) {
		// No, it has not
		m_notfound = m_pTexture;

		std::string name = std::string(GlobalRadiant().getAppPath())
		                   + "bitmaps/"
		                   + (IsDefault() ? "notex.bmp" : "shadernotex.bmp");

		// Construct a new BMP loader
		ImageConstructorPtr bmpConstructor(new FileLoader(name, "bmp"));
		m_pTexture = GlobalTexturesCache().capture(bmpConstructor, name);
	}

	realiseLighting();
}

void CShader::unrealise() {
	GlobalTexturesCache().release(m_pTexture);

	if (m_notfound != 0) {
		GlobalTexturesCache().release(m_notfound);
	}

	unrealiseLighting();
}

// Parse and load image maps for this shader
void CShader::realiseLighting() {

	// Create a shortcut reference
	TexturesCache& cache = GlobalTexturesCache();

	// Set up the diffuse, bump and specular stages. Bump and specular will
	// be set to defaults _flat and _black respectively, if an image map is
	// not specified in the material.

	// Load the diffuse map
	ImageConstructorPtr diffuseConstructor(new DefaultConstructor(m_template._diffuse->getTextureName()));
	m_pDiffuse = cache.capture(diffuseConstructor, m_template._diffuse->getTextureName());

	// Load the bump map
	ImageConstructorPtr bumpConstructor(new DefaultConstructor(m_template._bump->getTextureName()));
	m_pBump = cache.capture(bumpConstructor, m_template._bump->getTextureName());

	if (m_pBump == 0 || m_pBump->texture_number == 0) {
		// Bump Map load failed
		cache.release(m_pBump); // release old object first

		// Flat image name
		std::string flatName = std::string(GlobalRadiant().getAppPath())
		                       + "bitmaps/_flat.bmp";

		// Construct a new BMP loader
		ImageConstructorPtr bmpConstructor(new FileLoader(flatName, "bmp"));
		m_pBump = cache.capture(bmpConstructor, flatName);
	}

	// Load the specular map
	ImageConstructorPtr specConstructor(new DefaultConstructor(m_template._specular->getTextureName()));
	m_pSpecular = cache.capture(specConstructor, m_template._specular->getTextureName());

	if (m_pSpecular == 0 || m_pSpecular->texture_number == 0) {
		cache.release(m_pSpecular);

		// Default specular (black image)
		std::string blackName = std::string(GlobalRadiant().getAppPath()) + "bitmaps/_black.bmp";

		// Construct a new BMP loader
		ImageConstructorPtr bmpConstructor(new FileLoader(blackName, "bmp"));
		m_pSpecular = cache.capture(bmpConstructor, blackName);
	}

	// Get light falloff image. If a falloff image is defined but invalid,
	// emit a warning since this will result in a black light
	std::string foTexName = m_template._lightFallOff->getTextureName();

	// Allocate a default ImageConstructor with this name
	ImageConstructorPtr imageConstructor(new DefaultConstructor(foTexName));

	_texLightFalloff = cache.capture(imageConstructor, foTexName);
	if (!foTexName.empty() && _texLightFalloff->texture_number == 0) {
		std::cerr << "[shaders] " << _name
		<< " : defines invalid lightfalloff \"" << foTexName
		<< "\"" << std::endl;
	}

	for (ShaderTemplate::Layers::const_iterator i = m_template.m_layers.begin();
	        i != m_template.m_layers.end();
	        ++i) {
		m_layers.push_back(evaluateLayer(*i));
	}

	if (m_layers.size() == 1) {
		const BlendFuncExpression& blendFunc =
		    m_template.m_layers.front().m_blendFunc;

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
	GlobalTexturesCache().release(m_pDiffuse);
	GlobalTexturesCache().release(m_pBump);
	GlobalTexturesCache().release(m_pSpecular);

	GlobalTexturesCache().release(_texLightFalloff);

	for (MapLayers::iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
		GlobalTexturesCache().release(i->texture());
	}
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
	// Allocate a default ImageConstructor with this name
	ImageConstructorPtr imageConstructor(
	    new DefaultConstructor(layerTemplate.mapExpr->getTextureName())
	);

	return MapLayer(
           GlobalTexturesCache().
           capture(imageConstructor, layerTemplate.mapExpr->getTextureName()),
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
	return m_template.ambientLight;
}

bool CShader::isBlendLight() const {
	return m_template.blendLight;
}

bool CShader::isFogLight() const {
	return m_template.fogLight;
}

/*
 * Return the light falloff texture (Z dimension).
 */
TexturePtr CShader::lightFalloffImage() const {
	if (m_template._lightFallOff)
		return _texLightFalloff;
	else
		return _emptyLightFallOff;
}

bool CShader::m_lightingEnabled = false;

TexturePtr CShader::_emptyLightFallOff(new Texture("$emptyLightFallOff"));
