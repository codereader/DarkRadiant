#pragma once

#include "ShaderDefinition.h"
#include <memory>

namespace shaders {

/**
 * \brief
 * Implementation class for Material.
 */
class CShader
: public Material
{
private:
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
	bool IsDefault() const override;
	const char* getShaderFileName() const override;
    const vfs::FileInfo* getShaderFileInfo() const override;
	CullType getCullType() const override;
	ClampType getClampType() const override;
	int getSurfaceFlags() const override;
	SurfaceType getSurfaceType() const override;
	DeformType getDeformType() const override;
	int getSpectrum() const override;
	DecalInfo getDecalInfo() const override;
	Coverage getCoverage() const override;
	std::string getDescription() const override;
	std::string getDefinition() override;
	bool isAmbientLight() const override;
	bool isBlendLight() const override;
	bool isFogLight() const override;
	bool lightCastsShadows() const override;
	bool surfaceCastsShadow() const override;
	bool isDrawn() const override;
	bool isDiscrete() const override;
	bool isVisible() const override;
	void setVisible(bool visible) override;
    ShaderLayerVector getAllLayers() const;

	/**
	 * Return the name of the light falloff texture for use with this shader.
	 * Shaders which do not define their own falloff will take the default from
	 * the defaultPointLight shader.
	 */
	std::string getFalloffName() const;

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
}; // class CShader

typedef std::shared_ptr<CShader> CShaderPtr;

} // namespace shaders
