#pragma once

#include "ShaderTemplate.h"
#include <sigc++/connection.h>
#include <memory>

namespace shaders {

/**
 * \brief
 * Implementation class for Material.
 */
class CShader final : 
    public Material
{
private:
    bool _isInternal;

    // The unmodified template
    ShaderTemplate::Ptr _originalTemplate;

    // The template this material is working with - if this instance 
    // has not been altered, this is the same as _originalTemplate
    ShaderTemplate::Ptr _template;

    sigc::connection _templateChanged;

	// Name of shader
	std::string _name;

	// The 2D editor texture
	TexturePtr _editorTexture;

	TexturePtr _texLightFalloff;

	bool m_bInUse;

	bool _visible;

    sigc::signal<void> _sigMaterialModified;

public:
	/*
	 * Constructor. Sets the name and the ShaderTemplate to use.
	 */
	CShader(const std::string& name, const ShaderTemplate::Ptr& declaration);

    // Creates a named material from the given declaration, with an option to flag this material as "internal"
	CShader(const std::string& name, const ShaderTemplate::Ptr& declaration, bool isInternal);

	~CShader();

    /* Material implementation */
    float getSortRequest() const override;
    void setSortRequest(float sortRequest) override;
    void resetSortRequest() override;
    float getPolygonOffset() const override;
    void setPolygonOffset(float offset) override;
	TexturePtr getEditorImage() override;
    IMapExpression::Ptr getEditorImageExpression() override;
    void setEditorImageExpressionFromString(const std::string& editorImagePath) override;
	bool isEditorImageNoTex() override;
	TexturePtr lightFalloffImage() override;
	std::string getName() const override;
	bool IsInUse() const override;
	void SetInUse(bool bInUse) override;
	int getMaterialFlags() const override;
    void setMaterialFlag(Flags flag) override;
    void clearMaterialFlag(Flags flag) override;
	bool IsDefault() const override;
	const char* getShaderFileName() const override;
	void setShaderFileName(const std::string& fullPath) override;
    const vfs::FileInfo& getShaderFileInfo() const override;
	CullType getCullType() const override;
    void setCullType(CullType type) override;
	ClampType getClampType() const override;
    void setClampType(ClampType type) override;
	int getSurfaceFlags() const override;
    void setSurfaceFlag(Material::SurfaceFlags flag) override;
    void clearSurfaceFlag(Material::SurfaceFlags flag) override;
	SurfaceType getSurfaceType() const override;
    void setSurfaceType(SurfaceType type) override;
	DeformType getDeformType() const override;
	IShaderExpression::Ptr getDeformExpression(std::size_t index) override;
    std::string getDeformDeclName() override;
	int getSpectrum() const override;
    void setSpectrum(int spectrum) override;
	DecalInfo getDecalInfo() const override;
    void setDecalInfo(const DecalInfo& info) override;
	Coverage getCoverage() const override;
	std::string getDescription() const override;
    void setDescription(const std::string& description) override;
    FrobStageType getFrobStageType() override;
    void setFrobStageType(FrobStageType type) override;
    IMapExpression::Ptr getFrobStageMapExpression() override;
    void setFrobStageMapExpressionFromString(const std::string& expr) override;
    Vector3 getFrobStageRgbParameter(std::size_t index) override;
    void setFrobStageParameter(std::size_t index, double value) override;
    void setFrobStageRgbParameter(std::size_t index, const Vector3& value) override;
	std::string getDefinition() override;
	bool isAmbientLight() const override;
	bool isBlendLight() const override;
	bool isCubicLight() const override;
	bool isFogLight() const override;
    void setIsAmbientLight(bool newValue) override;
    void setIsBlendLight(bool newValue) override;
    void setIsFogLight(bool newValue) override;
    void setIsCubicLight(bool newValue) override;
	bool lightCastsShadows() const override;
	bool surfaceCastsShadow() const override;
	bool isDrawn() const override;
	bool isDiscrete() const override;
	bool isVisible() const override;
	void setVisible(bool visible) override;
    std::size_t getNumLayers() override;
    IShaderLayer::Ptr getLayer(std::size_t index) override;
    void foreachLayer(const std::function<bool(const IShaderLayer::Ptr&)>& functor) override;
    std::size_t addLayer(IShaderLayer::Type type) override;
    void removeLayer(std::size_t index) override;
    void swapLayerPosition(std::size_t first, std::size_t second) override;
    std::size_t duplicateLayer(std::size_t index) override;
    IEditableShaderLayer::Ptr getEditableLayer(std::size_t index) override;

    IMapExpression::Ptr getLightFalloffExpression() override;
    void setLightFalloffExpressionFromString(const std::string& expressionString) override;
    IShaderLayer::MapType getLightFalloffCubeMapType() override;
    void setLightFalloffCubeMapType(IShaderLayer::MapType type) override;
    std::string getRenderBumpArguments() override;
    std::string getRenderBumpFlatArguments() override;

    const std::string& getGuiSurfArgument() override;

	// -----------------------------------------

	void realise();
	void unrealise();

	/*
	 * Set name of shader.
	 */
	void setName(const std::string& name);

	IShaderLayer* firstLayer() override;
    int getParseFlags() const override;

    bool isModified() override;

    // Set this material to modified (this just creates the internal backup copy)
    void setIsModified();

    void commitModifications();
    void revertModifications() override;
    sigc::signal<void>& sig_materialChanged() override;

    void refreshImageMaps() override;

    ParseResult updateFromSourceText(const std::string& sourceText) override;

    // Returns the current template (including any modifications) of this material
    const ShaderTemplate::Ptr& getTemplate();

private:
    void ensureTemplateCopy();
    void subscribeToTemplateChanges();
    void updateEditorImage();
};
typedef std::shared_ptr<CShader> CShaderPtr;

} // namespace shaders
