#ifndef CSHADER_H_
#define CSHADER_H_

#include "ShaderDefinition.h"
#include "generic/callback.h"

class CShader : 
	public IShader 
{
	// Internal reference count
	int _nRef;

	const ShaderTemplate& m_template;
	std::string m_filename;

	// Name of shader
	std::string _name;

	// Textures for this shader
	Texture* m_pTexture;
	Texture* m_notfound;
	Texture* m_pDiffuse;
	float m_heightmapScale;
	Texture* m_pBump;
	Texture* m_pSpecular;
	Texture* _texLightFalloff;
	BlendFunc m_blendFunc;

	bool m_bInUse;

public:
	static bool m_lightingEnabled;

	/*
	 * Constructor. Sets the name and the ShaderDefinition to use.
	 */
	CShader(const std::string& name, const ShaderDefinition& definition);

	virtual ~CShader();

	// Increase reference count
	void IncRef();

	// Decrease reference count
	void DecRef();

	std::size_t refcount();

	// get/set the Texture* Radiant uses to represent this shader object
	Texture* getTexture() const;
	Texture* getDiffuse() const;

	// Return bumpmap if it exists, otherwise _flat
	Texture* getBump() const;
	Texture* getSpecular() const;

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
		Texture* m_texture;
		BlendFunc m_blendFunc;
		bool m_clampToBorder;
		float m_alphaTest;
	public:
		MapLayer(Texture* texture, BlendFunc blendFunc, bool clampToBorder, float alphaTest) :
				m_texture(texture),
				m_blendFunc(blendFunc),
				m_clampToBorder(false),
				m_alphaTest(alphaTest) {}
		Texture* texture() const {
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

	// Return the light falloff texture (Z dimension).
	Texture* lightFalloffImage() const;

}; // class CShader

extern Callback g_ActiveShadersChangedNotify;

#endif /*CSHADER_H_*/
