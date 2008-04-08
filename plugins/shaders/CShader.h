#ifndef CSHADER_H_
#define CSHADER_H_

#include "ShaderDefinition.h"
#include "generic/callback.h"
#include <boost/shared_ptr.hpp>

namespace shaders {

class CShader : 
	public IShader 
{
	const ShaderTemplate& _template;
	
	// The shader file name (i.e. the file where this one is defined)
	std::string _fileName;

	// Name of shader
	std::string _name;

	// Textures for this shader
	TexturePtr _editorTexture;
	TexturePtr _diffuse;
	TexturePtr _bump;
	TexturePtr _specular;
	TexturePtr _texLightFalloff;

	BlendFunc m_blendFunc;

	bool m_bInUse;

	bool _visible;

public:
	static bool m_lightingEnabled;
	
	/*
	 * Constructor. Sets the name and the ShaderDefinition to use.
	 */
	CShader(const std::string& name, const ShaderDefinition& definition);

	virtual ~CShader();

	// get/set the TexturePtr Radiant uses to represent this shader object
	TexturePtr getTexture();
	
	// getDiffuse() retrieves the TexturePtr and realises the shader if necessary
	TexturePtr getDiffuse();
	
	// Return bumpmap if it exists, otherwise _flat
	TexturePtr getBump();
	
	// Return specular map or a black texture
	TexturePtr getSpecular();

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
	const char* getName() const;

	bool IsInUse() const;
	
	void SetInUse(bool bInUse);
	
	// get the shader flags
	int getFlags() const;
	
	// get the transparency value
	float getTrans() const;
	
	// test if it's a true shader, or a default shader created to wrap around a texture
	bool IsDefault() const;
	
	// get the alphaFunc
	void getAlphaFunc(EAlphaFunc *func, float *ref);
	
	BlendFunc getBlendFunc() const;
	
	// get the cull type
	ECull getCull();
	
	// get shader file name (ie the file where this one is defined)
	const char* getShaderFileName() const;

	// Returns the description string of this material
	virtual std::string getDescription() const;
	
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

	class MapLayer : public ShaderLayer {
		TexturePtr m_texture;
		BlendFunc m_blendFunc;
		bool m_clampToBorder;
		float m_alphaTest;
	
	public:
		MapLayer(TexturePtr texture, BlendFunc blendFunc, bool clampToBorder, float alphaTest) :
				m_texture(texture),
				m_blendFunc(blendFunc),
				m_clampToBorder(false),
				m_alphaTest(alphaTest) 
		{}
		
		TexturePtr texture() const {
			return m_texture;
		}
		
		BlendFunc blendFunc() const {
			return m_blendFunc;
		}
		
		bool clampToBorder() const {
			return m_clampToBorder;
		}
		
		float alphaTest() const {
			return m_alphaTest;
		}
	};

	static MapLayer evaluateLayer(const LayerTemplate& layerTemplate);

	typedef std::vector<MapLayer> MapLayers;
	MapLayers m_layers;

	const ShaderLayer* firstLayer() const;
	
	void forEachLayer(const ShaderLayerCallback& callback) const;

	/* Required IShader light type predicates */
	bool isAmbientLight() const;

	bool isBlendLight() const;
	bool isFogLight() const;

	bool isVisible() const;
	void setVisible(bool visible);

}; // class CShader

typedef boost::shared_ptr<CShader> ShaderPtr;

} // namespace shaders

#endif /*CSHADER_H_*/
