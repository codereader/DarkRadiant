#pragma once

#include "ShaderDefinition.h"
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
    ShaderTemplatePtr _originalTemplate;

    // The template this material is working with - if this instance 
    // has not been altered, this is the same as _originalTemplate
	ShaderTemplatePtr _template;

    sigc::connection _templateChanged;

	// The shader file name (i.e. the file where this one is defined)
	vfs::FileInfo _fileInfo;

	// Name of shader
	std::string _name;

	// The 2D editor texture
	TexturePtr _editorTexture;

	TexturePtr _texLightFalloff;

	bool m_bInUse;

	bool _visible;

    // Vector of shader layers
	IShaderLayerVector _layers;

    sigc::signal<void> _sigMaterialModified;

public:
	static bool m_lightingEnabled;

	/*
	 * Constructor. Sets the name and the ShaderDefinition to use.
	 */
	CShader(const std::string& name, const ShaderDefinition& definition);

    // Creates a named material from the given definition, with an option to flag this material as "internal"
	CShader(const std::string& name, const ShaderDefinition& definition, bool isInternal);

	~CShader();

    /* Material implementation */
    float getSortRequest() const override;
    void setSortRequest(float sortRequest) override;
    void resetSortReqest() override;
    float getPolygonOffset() const override;
    void setPolygonOffset(float offset) override;
	TexturePtr getEditorImage() override;
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
	Coverage getCoverage() const override;
	std::string getDescription() const override;
    void setDescription(const std::string& description) override;
	std::string getDefinition() override;
	bool isAmbientLight() const override;
	bool isBlendLight() const override;
	bool isCubicLight() const;
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
    const IShaderLayerVector& getAllLayers() const;
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

	// Parse and load image maps for this shader
	void realiseLighting();
	void unrealiseLighting();

	/*
	 * Set name of shader.
	 */
	void setName(const std::string& name);

	IShaderLayer* firstLayer() const;
    int getParseFlags() const override;

    bool isModified() override;
    void revertModifications() override;
    sigc::signal<void>& sig_materialChanged() override;

private:
    void ensureTemplateCopy();
    void subscribeToTemplateChanges();
};
typedef std::shared_ptr<CShader> CShaderPtr;

} // namespace shaders
