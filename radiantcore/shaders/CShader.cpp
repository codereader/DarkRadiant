#include "CShader.h"
#include "MaterialManager.h"
#include "MapExpression.h"

#include "iregistry.h"
#include "ishaders.h"
#include "texturelib.h"
#include "gamelib.h"
#include "decl/DeclLib.h"
#include "materials/ParseLib.h"
#include "parser/DefTokeniser.h"

/* CONSTANTS */
namespace {

	// Registry path for default light shader
	const std::string DEFAULT_LIGHT_PATH = "/defaults/lightShader";

}

namespace shaders
{

/* Constructor. Sets the name and the ShaderTemplate to use.
 */
CShader::CShader(const std::string& name, const ShaderTemplate::Ptr& declaration) :
    CShader(name, declaration, false)
{}

CShader::CShader(const std::string& name, const ShaderTemplate::Ptr& declaration, bool isInternal) :
    _isInternal(isInternal),
    _originalTemplate(declaration),
    _template(declaration),
    _name(name),
    m_bInUse(false),
    _visible(true)
{
    subscribeToTemplateChanges();

    // Realise the shader
    realise();
}

CShader::~CShader()
{
    _templateChanged.disconnect();
	unrealise();
	GetTextureManager().checkBindings();
}

float CShader::getSortRequest() const
{
    return _template->getSortRequest();
}

void CShader::setSortRequest(float sortRequest)
{
    ensureTemplateCopy();
    _template->setSortRequest(sortRequest);
}

void CShader::resetSortRequest()
{
    ensureTemplateCopy();
    _template->resetSortRequest();
}

float CShader::getPolygonOffset() const
{
    return _template->getPolygonOffset();
}

void CShader::setPolygonOffset(float offset)
{
    ensureTemplateCopy();
    _template->setPolygonOffset(offset);
}

TexturePtr CShader::getEditorImage()
{
    if (!_editorTexture)
    {
        auto editorTex = _template->getEditorTexture();

        if (!editorTex)
        {
            // If there is no editor expression defined, use the an image from a layer, but no Bump or speculars
            for (const auto& layer : _template->getLayers())
            {
                if (layer->getType() != IShaderLayer::BUMP && layer->getType() != IShaderLayer::SPECULAR &&
                    std::dynamic_pointer_cast<MapExpression>(layer->getMapExpression()))
                {
                    editorTex = std::static_pointer_cast<MapExpression>(layer->getMapExpression());
                    break;
                }
            }
        }

        // Pass the call to the GLTextureManager to realise this image
        _editorTexture = GetTextureManager().getBinding(editorTex);
    }

    return _editorTexture;
}

IMapExpression::Ptr CShader::getEditorImageExpression()
{
    return _template->getEditorTexture();
}

void CShader::setEditorImageExpressionFromString(const std::string& editorImagePath)
{
    ensureTemplateCopy();

    _editorTexture.reset();
    _template->setEditorImageExpressionFromString(editorImagePath);
}

bool CShader::isEditorImageNoTex()
{
	return (getEditorImage() == GetTextureManager().getShaderNotFound());
}

IMapExpression::Ptr CShader::getLightFalloffExpression()
{
	return _template->getLightFalloff();
}

void CShader::setLightFalloffExpressionFromString(const std::string& expressionString)
{
    ensureTemplateCopy();
    _template->setLightFalloffExpressionFromString(expressionString);
}

IShaderLayer::MapType CShader::getLightFalloffCubeMapType()
{
    return _template->getLightFalloffCubeMapType();
}

void CShader::setLightFalloffCubeMapType(IShaderLayer::MapType type)
{
    ensureTemplateCopy();
    _template->setLightFalloffCubeMapType(type);
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
			MaterialPtr defLightShader = GetShaderSystem()->getMaterial(defLight);

			// Cast to a CShader so we can call getFalloffName().
			CShaderPtr cshaderPtr = std::static_pointer_cast<CShader>(defLightShader);

			// create the image
			_texLightFalloff = GetTextureManager().getBinding(cshaderPtr->_template->getLightFalloff());
		}

	}
	// Return the texture
	return _texLightFalloff;
}

std::string CShader::getName() const
{
	return _name;
}

std::string CShader::getDescription() const
{
	return _template->getDescription();
}

void CShader::setDescription(const std::string& description)
{
    ensureTemplateCopy();
    _template->setDescription(description);
}

Material::FrobStageType CShader::getFrobStageType()
{
    return _template->getFrobStageType();
}

void CShader::setFrobStageType(Material::FrobStageType type)
{
    ensureTemplateCopy();
    _template->setFrobStageType(type);
}

IMapExpression::Ptr CShader::getFrobStageMapExpression()
{
    return _template->getFrobStageMapExpression();
}

void CShader::setFrobStageMapExpressionFromString(const std::string& expr)
{
    ensureTemplateCopy();
    _template->setFrobStageMapExpressionFromString(expr);
}

Vector3 CShader::getFrobStageRgbParameter(std::size_t index)
{
    return _template->getFrobStageRgbParameter(index);
}

void CShader::setFrobStageParameter(std::size_t index, double value)
{
    ensureTemplateCopy();
    _template->setFrobStageParameter(index, value);
}

void CShader::setFrobStageRgbParameter(std::size_t index, const Vector3& value)
{
    ensureTemplateCopy();
    _template->setFrobStageRgbParameter(index, value);
}

bool CShader::IsInUse() const {
	return m_bInUse;
}

void CShader::SetInUse(bool bInUse) {
	m_bInUse = bInUse;
	GetShaderSystem()->activeShadersChangedNotify();
}

int CShader::getMaterialFlags() const
{
	return _template->getMaterialFlags();
}

void CShader::setMaterialFlag(Flags flag)
{
    ensureTemplateCopy();
    _template->setMaterialFlag(flag);
}

void CShader::clearMaterialFlag(Flags flag)
{
    ensureTemplateCopy();
    _template->clearMaterialFlag(flag);
}

bool CShader::IsDefault() const
{
	return _isInternal || _template->getBlockSyntax().fileInfo.name.empty();
}

// get the cull type
Material::CullType CShader::getCullType() const
{
	return _template->getCullType();
}

void CShader::setCullType(CullType type)
{
    ensureTemplateCopy();
    _template->setCullType(type);
}

ClampType CShader::getClampType() const
{
	return _template->getClampType();
}

void CShader::setClampType(ClampType type)
{
    ensureTemplateCopy();
    _template->setClampType(type);
}

int CShader::getSurfaceFlags() const
{
	return _template->getSurfaceFlags();
}

void CShader::setSurfaceFlag(Material::SurfaceFlags flag)
{
    ensureTemplateCopy();
    _template->setSurfaceFlag(flag);
}

void CShader::clearSurfaceFlag(Material::SurfaceFlags flag)
{
    ensureTemplateCopy();
    _template->clearSurfaceFlag(flag);
}

Material::SurfaceType CShader::getSurfaceType() const
{
	return _template->getSurfaceType();
}

void CShader::setSurfaceType(SurfaceType type)
{
    ensureTemplateCopy();
    _template->setSurfaceType(type);
}

Material::DeformType CShader::getDeformType() const
{
	return _template->getDeformType();
}

IShaderExpression::Ptr CShader::getDeformExpression(std::size_t index)
{
    return _template->getDeformExpression(index);
}

std::string CShader::getDeformDeclName()
{
    return _template->getDeformDeclName();
}

int CShader::getSpectrum() const
{
	return _template->getSpectrum();
}

void CShader::setSpectrum(int spectrum)
{
    ensureTemplateCopy();
    _template->setSpectrum(spectrum);
}

Material::DecalInfo CShader::getDecalInfo() const
{
	return _template->getDecalInfo();
}

void CShader::setDecalInfo(const DecalInfo& info)
{
    ensureTemplateCopy();
    return _template->setDecalInfo(info);
}

Material::Coverage CShader::getCoverage() const
{
	return _template->getCoverage();
}

// get shader file name (ie the file where this one is defined)
const char* CShader::getShaderFileName() const
{
	return _template->getBlockSyntax().fileInfo.name.c_str();
}

void CShader::setShaderFileName(const std::string& fullPath)
{
    auto materialsFolder = getMaterialsFolderName();
    auto pathRelativeToMaterialsFolder = decl::geRelativeDeclSavePath(fullPath, getMaterialsFolderName(), getMaterialFileExtension());

    _template->setFileInfo(vfs::FileInfo(materialsFolder, pathRelativeToMaterialsFolder, vfs::Visibility::NORMAL));
}

const vfs::FileInfo& CShader::getShaderFileInfo() const
{
    return _template->getBlockSyntax().fileInfo;
}

std::string CShader::getDefinition()
{
	return _template->getBlockContents();
}

int CShader::getParseFlags() const
{
    return _template->getParseFlags();
}

bool CShader::isModified()
{
    return _template != _originalTemplate;
}

void CShader::setIsModified()
{
    ensureTemplateCopy();
}

void CShader::revertModifications()
{
    _template = _originalTemplate;

    subscribeToTemplateChanges();

    // We need to update that layer reference vector on change
    unrealise();
    realise();

    _sigMaterialModified.emit();
}

sigc::signal<void>& CShader::sig_materialChanged()
{
    return _sigMaterialModified;
}

std::string CShader::getRenderBumpArguments()
{
    return _template->getRenderBumpArguments();
}

std::string CShader::getRenderBumpFlatArguments()
{
    return _template->getRenderBumpFlatArguments();
}

const std::string& CShader::getGuiSurfArgument()
{
    return _template->getGuiSurfArgument();
}

// -----------------------------------------

void CShader::realise() {
}

void CShader::unrealise()
{
    _editorTexture.reset();
    _texLightFalloff.reset();
}

void CShader::setName(const std::string& name)
{
	_name = name;
    _sigMaterialModified.emit();
}

IShaderLayer* CShader::firstLayer()
{
    const auto& layers = _template->getLayers();

	return layers.empty() ? nullptr : layers.front().get();
}

std::size_t CShader::getNumLayers()
{
    return _template->getLayers().size();
}

IShaderLayer::Ptr CShader::getLayer(std::size_t index)
{
    return _template->getLayers().at(index);
}

void CShader::foreachLayer(const std::function<bool(const IShaderLayer::Ptr&)>& functor)
{
    for (const auto& layer : _template->getLayers())
    {
        // Abort traversal when the functor returns false
        if (!functor(layer)) break;
    }
}

std::size_t CShader::addLayer(IShaderLayer::Type type)
{
    ensureTemplateCopy();

    auto newIndex = _template->addLayer(type);

    // We need another signal after the realiseLighting call
    _sigMaterialModified.emit();

    return newIndex;
}

void CShader::removeLayer(std::size_t index)
{
    ensureTemplateCopy();

    _template->removeLayer(index);

    // We need another signal after the realiseLighting call
    _sigMaterialModified.emit();
}

void CShader::swapLayerPosition(std::size_t first, std::size_t second)
{
    ensureTemplateCopy();

    _template->swapLayerPosition(first, second);

    // We need another signal after the realiseLighting call
    _sigMaterialModified.emit();
}

std::size_t CShader::duplicateLayer(std::size_t index)
{
    ensureTemplateCopy();

    auto newIndex = _template->duplicateLayer(index);

    // We need another signal after the realiseLighting call
    _sigMaterialModified.emit();

    return newIndex;
}

IEditableShaderLayer::Ptr CShader::getEditableLayer(std::size_t index)
{
    ensureTemplateCopy();

    const auto& layers = _template->getLayers();
    assert(index >= 0 && index < layers.size());

    return layers[index];
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

bool CShader::isCubicLight() const
{
    return _template->isCubicLight();
}

void CShader::setIsAmbientLight(bool newValue)
{
    ensureTemplateCopy();
    _template->setIsAmbientLight(newValue);
}

void CShader::setIsBlendLight(bool newValue)
{
    ensureTemplateCopy();
    _template->setIsBlendLight(newValue);
}

void CShader::setIsFogLight(bool newValue)
{
    ensureTemplateCopy();
    _template->setIsFogLight(newValue);
}

void CShader::setIsCubicLight(bool newValue)
{
    ensureTemplateCopy();
    _template->setIsCubicLight(newValue);
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

void CShader::setVisible(bool visible)
{
	_visible = visible;
}

void CShader::ensureTemplateCopy()
{
    if (_template != _originalTemplate)
    {
        return; // copy is already in place
    }

    // Create a clone of the original template
    _template = _originalTemplate->clone();

    subscribeToTemplateChanges();

    // We need to update that layer reference vector
    // as long as it's there
    unrealise();
    realise();
}

void CShader::commitModifications()
{
    if (_template == _originalTemplate) return;

    // Replace the contents with our working copy
    _originalTemplate->setBlockSyntax(_template->getBlockSyntax());

    // Overwrite the working template reference, the material is now unmodified again
    _template = _originalTemplate;
}

const ShaderTemplate::Ptr& CShader::getTemplate()
{
    return _template;
}

void CShader::subscribeToTemplateChanges()
{
    // Disconnect from any signal first
    _templateChanged.disconnect();

    _templateChanged = _template->sig_TemplateChanged().connect([this]
    {
        // Check if the editor image needs an update, do this before firing the handler
        updateEditorImage();

        _sigMaterialModified.emit();
    });
}

void CShader::updateEditorImage()
{
    // In case the editor tex is pointing to the fallback "shader not found"
    // check if we have some possible replacements
    if (!_editorTexture) return;

    if (isEditorImageNoTex() || !_template->getEditorTexture())
    {
        // The editor image is "shader not found", but we might have layers and/or an qer_editorImage defined
        // In case we don't have an editor texture expression, remove it from the slot
        // we will update it again as soon as it will be requested
        _editorTexture.reset();
    }
}

void CShader::refreshImageMaps()
{
    if (_template->getEditorTexture())
    {
        GetTextureManager().clearCacheForBindable(_template->getEditorTexture());
    }

    if (_template->getLightFalloff())
    {
        GetTextureManager().clearCacheForBindable(_template->getLightFalloff());
    }

    for (const auto& layer : _template->getLayers())
    {
        layer->refreshImageMaps();
    }

    _editorTexture.reset();
    _texLightFalloff.reset();

    _sigMaterialModified.emit();
}

Material::ParseResult CShader::updateFromSourceText(const std::string& sourceText)
{
    ensureTemplateCopy();

    // Attempt to parse the template (separately from the active one)
    auto newTemplate= std::make_shared<ShaderTemplate>(getName());

    // Only replace the contents of the block syntax, leave the rest unchanged
    auto syntax = _template->getBlockSyntax();
    syntax.contents = sourceText;
    newTemplate->setBlockSyntax(syntax);

    const auto& error = newTemplate->getParseErrors();

    if (error.empty())
    {
        // Parse seems to be successful, assign the text to the actual template
        _template->setBlockSyntax(syntax);
    }

    return ParseResult{ error.empty(), error };
}

} // namespace shaders
