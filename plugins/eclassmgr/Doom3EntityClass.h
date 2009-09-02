#ifndef DOOM3ENTITYCLASS_H_
#define DOOM3ENTITYCLASS_H_

#include "ieclass.h"
#include "irender.h"

#include "math/Vector3.h"
#include "math/aabb.h"

#include "parser/DefTokeniser.h"

#include <vector>
#include <map>

/* FORWARD DECLS */

class Shader;

namespace eclass
{

class Doom3EntityClass;
typedef boost::shared_ptr<Doom3EntityClass> Doom3EntityClassPtr;

/** 
 * Implementation of the IEntityClass interface. This represents a single
 * Doom 3 entity class, such as "light_moveable" or "monster_mancubus".
 */
class Doom3EntityClass
: public IEntityClass
{
	// The name of this entity class
	std::string _name;

    // Should this entity type be treated as a light?
    bool _isLight;
    
	// Colour of this entity and flag to indicate it has been specified
	Vector3	_colour;
	bool _colourSpecified;

	// Shader versions of the colour
	ShaderPtr _fillShader;
	ShaderPtr _wireShader;

	// Does this entity have a fixed size?
	bool _fixedSize;
	
	// Map of named EntityAttribute structures. EntityAttributes are picked
	// up from the DEF file during parsing.
	typedef std::map<std::string, EntityClassAttribute> EntityAttributeMap;
	EntityAttributeMap _attributes;
	
	// The model and skin for this entity class (if it has one)
	std::string _model;
	std::string _skin;
	
	// Flag to indicate inheritance resolved. An EntityClass resolves its
	// inheritance by copying all values from the parent onto the child,
	// after recursively instructing the parent to resolve its own inheritance.
	bool _inheritanceResolved;
	
	// Name of the mod owning this class
	std::string _modName;

	// The empty attribute
	EntityClassAttribute _emptyAttribute;

	// The list of strings containing the ancestors and this eclass itself.
	InheritanceChain _inheritanceChain;

	// The time this def has been parsed
	std::size_t _parseStamp;

	typedef std::set<IEntityClass::Observer*> Observers;
	Observers _observers;

private:

	// Capture the shaders corresponding to the current colour
	void captureColour();
	
	// Release the shaders associated with the current colour
	void releaseColour();

	// Clear all contents (done before parsing from tokens)
	void clear();

public:

	/** 
	 * Static function to create a default entity class.
	 * 
	 * @param name
	 * The name of the entity class to create.
	 * 
	 * @param brushes
	 * Whether the entity contains brushes or not.
	 */
	static Doom3EntityClassPtr create(const std::string& name, bool brushes);
	
    /** 
     * Default constructor.
     * 
     * @param name
     * Entity class name.
     * 
     * @param colour
     * Display colour for this entity.
     */
    Doom3EntityClass(const std::string& name, 
				 	 const Vector3& colour = Vector3(-1, -1, -1),
					 bool fixedSize = false,
				 	 const Vector3& mins = Vector3(1, 1, 1),
				 	 const Vector3& maxs = Vector3(-1, -1, -1));
    				 
    /** Destructor.
     */
	~Doom3EntityClass();
    
    /** Return the name of this entity class.
     */
	const std::string& getName() const;
	
	void addObserver(Observer* observer);
	void removeObserver(Observer* observer);

	/** Query whether this entity has a fixed size.
	 */
	bool isFixedSize() const;
    
	/* Return the bounding AABB.
	 */
	AABB getBounds() const;

    /** Get whether this entity type is a light entity
     * 
     * @returns
     * true if this is a light, false otherwise
     */
    bool isLight() const;
    
    /** Set whether this entity type is a light entity
     * 
     * @param val
     * true to set this as a light entity, false to disable
     */
    void setIsLight(bool val);

	/** Set the display colour for this entity.
	 * 
	 * @param colour
	 * The new colour to use.
	 */
	void setColour(const Vector3& colour);
     
	/** Get this entity's colour.
	 * 
	 * @returns
	 * A Vector3 containing the current colour.
	 */
	const Vector3& getColour() const;

	/** Return this entity's wireframe shader.
	 */
	ShaderPtr getWireShader() const;

	/** Return this entity's fill shader.
	 */
	ShaderPtr getFillShader() const;
	
	/* ATTRIBUTES */
	
	/** 
	 * Insert an EntityClassAttribute, without overwriting previous values.
	 */
	void addAttribute(const EntityClassAttribute& attribute);
	
	/*
	 * Find a single attribute.
	 */
	EntityClassAttribute& getAttribute(const std::string& name);
	const EntityClassAttribute& getAttribute(const std::string& name) const;
	
	/*
	 * Return a list of all attributes matching the given name prefix.
	 */
	EntityClassAttributeList getAttributeList(const std::string& name) const;

	/** Enumerate the EntityClassAttributes.
	 */
	void forEachClassAttribute(EntityClassAttributeVisitor& visitor,
							   bool editorKeys) const;
	
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

	/**
	 * Returns the inheritance chain (including this eclass).
	 */
	virtual const InheritanceChain& getInheritanceChain();
	
	/**
	 * Resolve inheritance for this class.
	 * 
	 * @param classmap
	 * A reference to the global map of entity classes, which should be searched
	 * for the parent entity.
	 */
	typedef std::map<std::string, Doom3EntityClassPtr> EntityClasses;
	void resolveInheritance(EntityClasses& classmap);
	
	/**
	 * Return the mod name.
	 */
	std::string getModName() const {
	    return _modName;   
	}
	
	/**
	 * Set the mod name.
	 */
	void setModName(const std::string& mn) {
	    _modName = mn;
	}
	
	// Initialises this class from the given tokens
	void parseFromTokens(parser::DefTokeniser& tokeniser);

	void setParseStamp(std::size_t parseStamp)
	{
		_parseStamp = parseStamp;
	}

	std::size_t getParseStamp() const
	{
		return _parseStamp;
	}

private:
	// Rebuilds the inheritance chain (called after inheritance is resolved)
	void buildInheritanceChain();
};

/**
 * Pointer typedef.
 */
typedef boost::shared_ptr<Doom3EntityClass> Doom3EntityClassPtr;

}

#endif /*DOOM3ENTITYCLASS_H_*/
