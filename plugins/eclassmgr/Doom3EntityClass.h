#ifndef DOOM3ENTITYCLASS_H_
#define DOOM3ENTITYCLASS_H_

#include "ieclass.h"
#include "irender.h"

#include "math/Vector3.h"

#include <string>
#include <vector>
#include <map>

/* FORWARD DECLS */

class Shader;

namespace eclass
{

/** Implementation of the IEntityClass interface. This represents a single
 * Doom 3 entity class, such as "light_moveable" or "monster_mancubus".
 */

class Doom3EntityClass
: public IEntityClass
{
	// The name of this entity class
	std::string _name;

	// Usage string for this entity class
	std::string _usage;

    // Should this entity type be treated as a light?
    bool _isLight;
    
	// Colour of this entity and flag to indicate it has been specified
	Vector3	_colour;
	bool _colourSpecified;

	// Shader versions of the colour
	Shader* _fillShader;
	Shader* _wireShader;

	// Does this entity have a fixed size?
	bool _fixedSize;
	
	// Map of named EntityAttribute structures. EntityAttributes are picked
	// up from the DEF file during parsing.
	typedef std::map<std::string, EntityClassAttribute> EntityAttributeMap;
	EntityAttributeMap _attributes;
	
	// The model and skin for this entity class (if it has one)
	std::string _model;
	std::string _skin;
	
	// Minimum and maximum sizes, and the size specified flag
	Vector3	_mins;
	Vector3 _maxs;
	bool _sizeSpecified;

	// The parent entity classname, if there is one, and the IEntityClass
	// of the parent itself.
	std::string _parentName;
	IEntityClass* _parentClass;
	
private:

	// Capture the shaders corresponding to the current colour
	void captureColour();
	
	// Release the shaders associated with the current colour
	void releaseColour();

public:

	/** Static function to create a default entity class.
	 * 
	 * @param name
	 * The name of the entity class to create.
	 * 
	 * @param brushes
	 * Whether the entity contains brushes or not.
	 */
	static IEntityClass* create(const std::string& name, bool brushes);
	
    /** Default constructor.
     * 
     * @param name
     * Entity class name.
     * 
     * @param colour
     * Display colour for this entity.
     */
    Doom3EntityClass(const std::string& name, 
				 	 const Vector3& colour = Vector3(0, 0.4, 0),
					 bool fixedSize = false,
				 	 const Vector3& mins = Vector3(1, 1, 1),
				 	 const Vector3& maxs = Vector3(-1, -1, -1));
    				 
    /** Destructor.
     */
	~Doom3EntityClass() {
		// Release the shaders
		releaseColour();
	}		
    
    /** Return the name of this entity class.
     */
	const std::string& getName() const {
		return _name;
	}
	
	/** Query whether this entity has a fixed size.
	 */
	bool isFixedSize() const {
		return _fixedSize;
	}
    
    /** Set minimum size.
     */
	void setMins(const Vector3& mins) {
		_mins = mins;
		_sizeSpecified = true;
		_fixedSize = true;
	}
		
    /** Set maximum size.
     */
	void setMaxs(const Vector3& maxs) {
		_maxs = maxs;
		_sizeSpecified = true;
		_fixedSize = true;
	}
    
    /** Get minimum size.
     */
	Vector3 getMins() const {
		return _mins;
	}
	
	/** Get maximum size.
	 */
	Vector3 getMaxs() const {
		return _maxs;
	}
    
    /** Set the usage information for this entity class.
     */
	void setUsage(const std::string& usage) {
		_usage = usage;
	}
	
	/** Get the usage information.
	 */
	const std::string& getUsage() const {
		return _usage;
	}
    
    /** Get whether this entity type is a light entity
     * 
     * @returns
     * true if this is a light, false otherwise
     */
    bool isLight() const {
        return _isLight;
    }
    
    /** Set whether this entity type is a light entity
     * 
     * @param val
     * true to set this as a light entity, false to disable
     */
    void setIsLight(bool val) {
        _isLight = val;
        if (_isLight)
        	_fixedSize = true;
    }

	/** Set the display colour for this entity.
	 * 
	 * @param colour
	 * The new colour to use.
	 */
	void setColour(const Vector3& colour) {
		// Set the specified flag
		_colourSpecified = true;
		
		// Release the current shaders, then capture the new ones
		releaseColour();
		_colour = colour;
		captureColour();
	}
     
	/** Get this entity's colour.
	 * 
	 * @returns
	 * A Vector3 containing the current colour.
	 */
	const Vector3& getColour() const {
		return _colour;
	}

	/** Return this entity's wireframe shader.
	 */
	Shader* getWireShader() const {
		return _wireShader;
	}

	/** Return this entity's fill shader.
	 */
	Shader* getFillShader() const {
		return _fillShader;
	}
	
	/* ATTRIBUTES */
	
	/** Insert an EntityClassAttribute.
	 */
	void addAttribute(const EntityClassAttribute& attribute) {
		_attributes.insert(EntityAttributeMap::value_type(attribute.name, attribute));
	}

	/** Look up the given key in the list of attributes and return
	 * the value.
	 */
	std::string getValueForKey(const std::string& key) const {
		EntityAttributeMap::const_iterator i = _attributes.find(key);
		if (i != _attributes.end()) {
			return i->second.value;
		}
		else {
			return "";
		}	
	}

	/** Enumerate the EntityClassAttributes.
	 */
	void forEachClassAttribute(EntityClassAttributeVisitor& visitor) const {
		for (EntityAttributeMap::const_iterator i = _attributes.begin();
			 i != _attributes.end();
			 ++i)
		{
			visitor.visit(i->second);
		}
	}
	
	/** Set a model on this entity class.
	 * 
	 * @param
	 * The VFS model path.
	 */
	void setModelPath(const std::string& path) {
		_fixedSize = true;
		_model = path;
	}
	
	/** Return the model path
	 */
	const std::string& getModelPath() const {
		return _model;
	}
	
	/** Set the skin.
	 */
	void setSkin(const std::string& skin) {
		_skin = skin;
	}
	
	/** Get the skin.
	 */
	const std::string& getSkin() const {
		return _skin;
	}
	
	/** Set the parent entityclass name.
	 */
	void setParent(const std::string& p) {
		_parentName = p;
	}
	
	/** Return the parent entityclass name.
	 */
	const std::string& getParent() const {
		return _parentName;
	}
	
	/** Set the parent entityclass object.
	 */
	void setParentEntity(IEntityClass* p) {
		_parentClass = p;
	}
	
	/** Recursively resolve inheritance.
	 */
	void resolveInheritance();

};

}

#endif /*DOOM3ENTITYCLASS_H_*/
