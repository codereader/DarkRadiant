#include "CShader.h"
#include "Doom3ShaderSystem.h"

#include "iregistry.h"
#include "ishaders.h"
#include "texturelib.h"
#include "gamelib.h"
#include "parser/DefTokeniser.h"

#include <boost/lexical_cast.hpp>

/* CONSTANTS */
namespace {

	// Registry path for default light shader
	const std::string DEFAULT_LIGHT_PATH = "/defaults/lightShader";

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

int CShader::getSortRequest() const
{
    return _template->getSortRequest();
}

float CShader::getPolygonOffset() const
{
    return _template->getPolygonOffset();
}

TexturePtr CShader::getEditorImage()
{
    if (!_editorTexture)
    {
        // Pass the call to the GLTextureManager to realise this image
        _editorTexture = GetTextureManager().getBinding(
            _template->getEditorTexture()
        );
    }

    return _editorTexture;
}

bool CShader::isEditorImageNoTex()
{
	return (getEditorImage() == GetTextureManager().getShaderNotFound());
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
			std::string defLight = game::current::getValue<std::string>(DEFAULT_LIGHT_PATH);
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
int CShader::getMaterialFlags() const
{
	return _template->getMaterialFlags();
}

// test if it's a true shader, or a default shader created to wrap around a texture
bool CShader::IsDefault() const {
	return _fileName.empty();
}

// get the cull type
Material::CullType CShader::getCullType() const
{
	return _template->getCullType();
}

ClampType CShader::getClampType() const
{
	return _template->getClampType();
}

int CShader::getSurfaceFlags() const
{
	return _template->getSurfaceFlags();
}

Material::SurfaceType CShader::getSurfaceType() const
{
	return _template->getSurfaceType();
}

Material::DeformType CShader::getDeformType() const
{
	return _template->getDeformType();
}

int CShader::getSpectrum() const
{
	return _template->getSpectrum();
}

const Material::DecalInfo& CShader::getDecalInfo() const
{
	return _template->getDecalInfo();
}

Material::Coverage CShader::getCoverage() const
{
	return _template->getCoverage();
}

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

ShaderLayer* CShader::firstLayer() const
{
	if (_layers.empty())
	{
		return NULL;
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

bool CShader::lightCastsShadows() const
{
	int flags = getMaterialFlags();
	return (flags & FLAG_FORCESHADOWS) ||
		   (!isFogLight() && !isAmbientLight() && !isBlendLight() && !(flags & FLAG_NOSHADOWS));
}

bool CShader::surfaceCastsShadow() const
{
	int flags = getMaterialFlags();
	return (flags & FLAG_FORCESHADOWS) || !(flags & FLAG_NOSHADOWS);
}

bool CShader::isDrawn() const
{
	return _template->getLayers().size() > 0 || (getSurfaceFlags() & SURF_ENTITYGUI) /*|| gui != NULL*/;
}

bool CShader::isDiscrete() const
{
	int flags = getSurfaceFlags();
	return (flags & SURF_ENTITYGUI) /*|| gui*/ || getDeformType() != DEFORM_NONE || 
			getSortRequest() == SORT_SUBVIEW || (flags & SURF_DISCRETE);
}

bool CShader::isVisible() const {
	return _visible;
}

void CShader::setVisible(bool visible) {
	_visible = visible;
}

bool CShader::m_lightingEnabled = false;

} // namespace shaders
