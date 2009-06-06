#ifndef SHADERTEMPLATE_H_
#define SHADERTEMPLATE_H_

#include "MapExpression.h"
#include "Doom3ShaderLayer.h"

#include "ishaders.h"
#include "parser/DefTokeniser.h"
#include "math/Vector3.h"

#include <map>
#include <boost/shared_ptr.hpp>

namespace shaders { class MapExpression; }

namespace shaders
{

/**
 * Data structure storing parsed material information from a material decl. This
 * class parses the decl using a tokeniser and stores the relevant information
 * internally, for later use by a CShader.
 */
class ShaderTemplate
{
	// Template name
	std::string _name;
  
	// Temporary current layer (used by the parsing functions)
	Doom3ShaderLayerPtr _currentLayer;
  
public:

  	// Vector of LayerTemplates representing each stage in the material
  	typedef std::vector<Doom3ShaderLayerPtr> Layers;

private:
	Layers m_layers;

    // Editorimage texture
	NamedBindablePtr _editorTex;

	// Map expressions
	shaders::MapExpressionPtr _lightFalloff;

	/* Light type booleans */	
	bool fogLight;
	bool ambientLight;
	bool blendLight;

	// The description tag of the material
	std::string description;

  int m_nFlags;
  float m_fTrans;

  // alphafunc stuff
  Material::EAlphaFunc m_AlphaFunc;
  float m_AlphaRef;
  // cull stuff
  Material::ECull m_Cull;

	std::string _blockContents;

	// Whether the block has been parsed
	bool _parsed;

public:

    /**
     * \brief
     * Construct a ShaderTemplate.
     */
	ShaderTemplate(const std::string& name, const std::string& blockContents) 
	: _name(name),
      _currentLayer(new Doom3ShaderLayer),
      fogLight(false),
      ambientLight(false),
      blendLight(false),
	  _blockContents(blockContents),
	  _parsed(false)
	{
    	m_nFlags = 0;
    	m_fTrans = 1.0f;    
	}

	/**
	 * Get the name of this shader template.
	 */
	std::string getName() const {
    	return _name;
	}
	
	/**
	 * Set the name of this shader template.
	 */
	void setName(const std::string& name) {
		_name = name;
	}

	const std::string& getDescription() {
		if (!_parsed) parseDefinition();
		return description;
	}

	int getFlags() {
		if (!_parsed) parseDefinition();
		return m_nFlags;
	}

	float getTrans() {
		if (!_parsed) parseDefinition();
		return m_fTrans;
	}

	Material::EAlphaFunc getAlphaFunc() {
		if (!_parsed) parseDefinition();
		return m_AlphaFunc;
	}

	float getAlphaRef() {
		if (!_parsed) parseDefinition();
		return m_AlphaRef;
	}

	Material::ECull getCull() {
		if (!_parsed) parseDefinition();
		return m_Cull;
	}

	const Layers& getLayers() {
		if (!_parsed) parseDefinition();
		return m_layers;
	}

	bool isFogLight() {
		if (!_parsed) parseDefinition();
		return fogLight;
	}

	bool isAmbientLight() {
		if (!_parsed) parseDefinition();
		return ambientLight;
	}

	bool isBlendLight() {
		if (!_parsed) parseDefinition();
		return blendLight;
	}

	// Sets the raw block definition contents, will be parsed on demand
	void setBlockContents(const std::string& blockContents) {
		_blockContents = blockContents;
	}

	const std::string& getBlockContents() const
	{
		return _blockContents;
	}

    /**
     * \brief
     * Return the named bindable corresponding to the editor preview texture
     * (qer_editorimage).
     */
	NamedBindablePtr getEditorTexture();

	const shaders::MapExpressionPtr& getLightFalloff() {
		if (!_parsed) parseDefinition();
		return _lightFalloff;
	}

private:
	/**
	 * Parse a Doom 3 material decl. This is the master parse function, it
	 * returns no value but exceptions may be thrown at any stage of the 
	 * parsing.
	 */
	void parseDefinition();

    // Parse helpers. These scan for possible matches, this is not a
    // recursive-descent parser
	void parseShaderFlags(parser::DefTokeniser&, const std::string&);
	void parseLightFlags(parser::DefTokeniser&, const std::string&);
	void parseBlendShortcuts(parser::DefTokeniser&, const std::string&);
	void parseBlendType(parser::DefTokeniser&, const std::string&);
	void parseBlendMaps(parser::DefTokeniser&, const std::string&);
    void parseColourModulation(parser::DefTokeniser&, const std::string&);

	bool saveLayer();
  
};

/* TYPEDEFS */

// Pointer to ShaderTemplate
typedef boost::shared_ptr<ShaderTemplate> ShaderTemplatePtr;

// Map of named ShaderTemplates
typedef std::map<std::string, ShaderTemplatePtr> ShaderTemplateMap;

}

#endif /*SHADERTEMPLATE_H_*/
