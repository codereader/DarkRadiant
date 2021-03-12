#pragma once

#include "ShaderDefinition.h"
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

    // The template this mateiral is working with - if this instance 
    // has not been altered, this is the same as _originalTemplate
	ShaderTemplatePtr _template;

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
	ShaderLayerVector _layers;

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
    int getSortRequest() const override;
    float getPolygonOffset() const override;
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
	ClampType getClampType() const override;
	int getSurfaceFlags() const override;
	SurfaceType getSurfaceType() const override;
    void setSurfaceType(SurfaceType type) override;
	DeformType getDeformType() const override;
	IShaderExpressionPtr getDeformExpression(std::size_t index) override;
    std::string getDeformDeclName() override;
	int getSpectrum() const override;
	DecalInfo getDecalInfo() const override;
	Coverage getCoverage() const override;
	std::string getDescription() const override;
    void setDescription(const std::string& description) override;
	std::string getDefinition() override;
	bool isAmbientLight() const override;
	bool isBlendLight() const override;
	bool isCubicLight() const;
	bool isFogLight() const override;
	bool lightCastsShadows() const override;
	bool surfaceCastsShadow() const override;
	bool isDrawn() const override;
	bool isDiscrete() const override;
	bool isVisible() const override;
	void setVisible(bool visible) override;
    const ShaderLayerVector& getAllLayers() const;

    IMapExpression::Ptr getLightFalloffExpression() override;
	IMapExpression::Ptr getLightFalloffCubeMapExpression() override;
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

	ShaderLayer* firstLayer() const;
    int getParseFlags() const override;

    bool isModified() override;

private:
    void ensureTemplateCopy();
};
typedef std::shared_ptr<CShader> CShaderPtr;

} // namespace shaders
