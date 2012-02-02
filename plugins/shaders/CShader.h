#pragma once

#include "ShaderDefinition.h"
#include <boost/shared_ptr.hpp>

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
	std::string _fileName;

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

    int getSortRequest() const;
    float getPolygonOffset() const;
	TexturePtr getEditorImage();
	bool isEditorImageNoTex();

	// Return the light falloff texture (Z dimension).
	TexturePtr lightFalloffImage();

	/**
	 * Return the name of the light falloff texture for use with this shader.
	 * Shaders which do not define their own falloff will take the default from
	 * the defaultPointLight shader.
	 */
	std::string getFalloffName() const;

	/*
	 * Return name of shader.
	 */
	std::string getName() const;

	bool IsInUse() const;

	void SetInUse(bool bInUse);

	// get the shader flags
	int getMaterialFlags() const;

	// test if it's a true shader, or a default shader created to wrap around a texture
	bool IsDefault() const;

	// get the cull type
	CullType getCullType() const;

	// Clamp type
	ClampType getClampType() const;

	int getSurfaceFlags() const;

	SurfaceType getSurfaceType() const;

	// Deform types
	DeformType getDeformType() const;

	// Get the spectrum, -1 is "not defined"
	int getSpectrum() const;

	const DecalInfo& getDecalInfo() const;

	Coverage getCoverage() const;

	// get shader file name (ie the file where this one is defined)
	const char* getShaderFileName() const;

	// Returns the description string of this material
	std::string getDescription() const;

	// returns the raw definition block
	std::string getDefinition();

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

    /* Material implementation */

    const ShaderLayerVector& getAllLayers() const;

	bool isAmbientLight() const;
	bool isBlendLight() const;
	bool isFogLight() const;
	bool lightCastsShadows() const;
	bool surfaceCastsShadow() const;
	bool isDrawn() const;
	bool isDiscrete() const;

	bool isVisible() const;
	void setVisible(bool visible);

}; // class CShader

typedef boost::shared_ptr<CShader> CShaderPtr;

} // namespace shaders
