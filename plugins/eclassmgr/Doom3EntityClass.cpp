#include "Doom3EntityClass.h"
#include "AttributeCopyingVisitor.h"
#include "AttributeSuffixComparator.h"

#include "iuimanager.h"
#include "os/path.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace eclass
{

// Constructor	
Doom3EntityClass::Doom3EntityClass(const std::string& name, 
								   const Vector3& colour,
								   bool fixedSize,
								   const Vector3& mins,
								   const Vector3& maxs)				 
: _name(name),
  _isLight(false),
  _colour(colour),
  _colourSpecified(false),
  _fixedSize(fixedSize),
  _model(""),
  _skin(""),
  _inheritanceResolved(false),
  _modName("base")
{
	// Set the entity colour to default, if none was specified
	if (_colour == Vector3(-1, -1, -1)) {
		_colour = ColourSchemes().getColour("default_entity");
	}
	// Capture the shaders
	captureColour();		
}

Doom3EntityClass::~Doom3EntityClass() {
	// Release the shaders
	releaseColour();
}

const std::string& Doom3EntityClass::getName() const {
	return _name;
}

/** Query whether this entity has a fixed size.
 */
bool Doom3EntityClass::isFixedSize() const {
    if (_fixedSize) {
        return true;
    }
    else {
        // Check for the existence of editor_mins/maxs attributes, and that
        // they do not contain only a question mark
		return (getAttribute("editor_mins").value.size() > 1
                && getAttribute("editor_maxs").value.size() > 1);
    }
}

AABB Doom3EntityClass::getBounds() const {
    if (isFixedSize()) {
        return AABB::createFromMinMax(
        	getAttribute("editor_mins").value, 
        	getAttribute("editor_maxs").value
        );
    }
    else {
        return AABB(); // null AABB
    }
}

bool Doom3EntityClass::isLight() const {
    return _isLight;
}

/** Set whether this entity type is a light entity
 * 
 * @param val
 * true to set this as a light entity, false to disable
 */
void Doom3EntityClass::setIsLight(bool val) {
    _isLight = val;
    if (_isLight)
    	_fixedSize = true;
}

/** Set the display colour for this entity.
 * 
 * @param colour
 * The new colour to use.
 */
void Doom3EntityClass::setColour(const Vector3& colour) {
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
const Vector3& Doom3EntityClass::getColour() const {
	return _colour;
}

/** Return this entity's wireframe shader.
 */
ShaderPtr Doom3EntityClass::getWireShader() const {
	return _wireShader;
}

/** Return this entity's fill shader.
 */
ShaderPtr Doom3EntityClass::getFillShader() const {
	return _fillShader;
}

/* ATTRIBUTES */

/** 
 * Insert an EntityClassAttribute, without overwriting previous values.
 */
void Doom3EntityClass::addAttribute(const EntityClassAttribute& attribute) {
	// Try to insert the class attribute
	std::pair<EntityAttributeMap::iterator, bool> result = _attributes.insert(
		EntityAttributeMap::value_type(attribute.name, attribute)
	);

	if (!result.second) {
		EntityClassAttribute& existing = result.first->second;

		// greebo: Attribute already existed, check if we have some 
		// descriptive properties to be added to the existing one.
		if (!attribute.description.empty() && existing.description.empty()) {
			existing.description = attribute.description;
		}

		// Check if we have a more descriptive type than "text"
		if (attribute.type != "text" && existing.type == "text") {
			existing.type = attribute.type;
		}
	}
}

// Static function to create an EntityClass (named constructor idiom)
IEntityClassPtr Doom3EntityClass::create(const std::string& name, bool brushes) {
	if (!brushes) {
		return IEntityClassPtr(new Doom3EntityClass(name, 
													Vector3(-1, -1, -1),
													true, 
													Vector3(-8, -8, -8),
													Vector3(8, 8, 8)));
	}
	else {
		return IEntityClassPtr(new Doom3EntityClass(name));
	}
}

// Capture the shaders for the current colour
void Doom3EntityClass::captureColour() {
	// Capture fill and wire versions of the entity colour
	std::string fillCol = (boost::format("(%g %g %g)") % _colour[0] % _colour[1] % _colour[2]).str();
	std::string wireCol = (boost::format("<%g %g %g>") % _colour[0] % _colour[1] % _colour[2]).str();

	_fillShader = GlobalShaderCache().capture(fillCol);
	_wireShader = GlobalShaderCache().capture(wireCol);
}

// Release the shaders for the current colour
void Doom3EntityClass::releaseColour() {
	_fillShader = ShaderPtr();
	_wireShader = ShaderPtr();
}

// Enumerate entity class attributes
void Doom3EntityClass::forEachClassAttribute(
	EntityClassAttributeVisitor& visitor,
	bool editorKeys) const 
{
	for (EntityAttributeMap::const_iterator i = _attributes.begin();
		 i != _attributes.end();
		 ++i)
	{
		// Visit if it is a non-editor key or we are visiting all keys
		if (editorKeys || !boost::algorithm::istarts_with(i->first, "editor_"))
			visitor.visit(i->second);
	}
}

// Resolve inheritance for this class
void Doom3EntityClass::resolveInheritance(EntityClasses& classmap)
{
	// If we have already resolved inheritance, do nothing
	if (_inheritanceResolved)
		return;
		
	// Lookup the parent name and return if it is not set. Also return if the
	// parent name is the same as our own classname, to avoid infinite
	// recursion.
	std::string parName = getAttribute("inherit").value;
	if (parName.empty() || parName == _name)
		return;

	// Find the parent entity class
	EntityClasses::iterator pIter = classmap.find(parName);
	if (pIter != classmap.end()) {
		
		// Get the class object pointer and cast it to a Doom3EntityClass
		boost::shared_ptr<Doom3EntityClass> par =
			boost::static_pointer_cast<Doom3EntityClass>(pIter->second);

		// Recursively resolve inheritance of parent
		par->resolveInheritance(classmap);

		// Copy attributes from the parent to the child, including editor keys
		AttributeCopyingVisitor visitor(*this);
		par->forEachClassAttribute(visitor, true);
	}
	else {
		std::cout << "[eclassmgr] Warning: Entity class "
				  << _name << " specifies parent "
				  << parName << " which is not found." << std::endl;
	} 

	// Set the resolved flag
	_inheritanceResolved = true;

	// Construct the inheritance list
	buildInheritanceChain();

	if (getAttribute("model").value != "") {
		// We have a model path (probably an inherited one)
		setModelPath(getAttribute("model").value);
	}
}

// Find a single attribute
EntityClassAttribute& 
Doom3EntityClass::getAttribute(const std::string& name) {
	EntityAttributeMap::iterator f = _attributes.find(name);
	if (f != _attributes.end()) {
		return f->second;
	}
	else {
		return _emptyAttribute;
	}
}

// Find a single attribute
const EntityClassAttribute& 
Doom3EntityClass::getAttribute(const std::string& name) const {
	EntityAttributeMap::const_iterator f = _attributes.find(name);
	if (f != _attributes.end()) {
		return f->second;
	}
	else {
		return _emptyAttribute;
	}
}

// Find all matching attributes
EntityClassAttributeList 
Doom3EntityClass::getAttributeList(const std::string& name) const {
	
	// Build the list of matching attributes
	EntityClassAttributeList matches;
	for (EntityAttributeMap::const_iterator i = _attributes.begin();
		 i != _attributes.end();
		 ++i)
	{
		// Prefix matches, add to list
		if (boost::algorithm::istarts_with(i->first, name))
			matches.push_back(i->second);
	}
	
	// Sort the list based on the numerical order of suffices
	AttributeSuffixComparator comp(name.length());
	std::sort(matches.begin(), matches.end(), comp);
	
	// Return the list of matches
	return matches;
}

void Doom3EntityClass::parseFromTokens(parser::DefTokeniser& tokeniser) {
	// Get the (lowercase) entity name
    const std::string sName = 
    	boost::algorithm::to_lower_copy(tokeniser.nextToken());
    
    // Store the name
    _name = sName;

	// Required open brace (the name has already been parsed by the EClassManager)
    tokeniser.assertNextToken("{");

    // Loop over all of the keys in this entitydef
    while (true) {
        const std::string key = tokeniser.nextToken();
        
        if (key == "}") {
        	break; // end of def
        }

        const std::string value = tokeniser.nextToken();
    
        // Otherwise, switch on the key name
        
        if (key == "model") {
        	setModelPath(os::standardPath(value));
        }
        else if (key == "editor_color") {
            setColour(value);
        }
        else if (key == "editor_light") {
            if (value == "1") {
                setIsLight(true);
            }
        }
        else if (key == "spawnclass") {
            if (value == "idLight") {
                setIsLight(true);
            }
        }
		else if (boost::algorithm::istarts_with(key, "editor_")) {
			// "editor_yyy" represents an attribute that may be set on this
        	// entity. Construct a value-less EntityClassAttribute to add to
        	// the class, so that it will show in the entity inspector.

			// Locate the space in "editor_bool myVariable", starting after "editor_"
			std::size_t spacePos = key.find(' ', 7);

			// Only proceed if we have a space (some keys like "editor_displayFolder" don't have spaces)
			if (spacePos != std::string::npos) {
				// The part beyond the space is the name of the attribute
				std::string attName = key.substr(spacePos + 1);
				
				// Get the type by trimming the string left and right
				std::string type = key.substr(7, key.length() - attName.length() - 8);
		
				if (!attName.empty()) {

					// Transform the type into a better format
					if (type == "var" || type == "string") {
						type = "text";
					}

        			addAttribute(EntityClassAttribute(type, attName, "", value));
        		}
			}
		}
        
		// Following key-specific processing, add the keyvalue to the eclass
		EntityClassAttribute attribute("text", key, value, "");

		if (getAttribute(key).type.empty()) {
			// Type is empty, attribute does not exist, add it.
			addAttribute(attribute);
		}
		else if (getAttribute(key).value.empty() ) {
			// Attribute type is set, but value is empty, set the value.
			getAttribute(key).value = value;
		}
		else {
			// Both type and value are not empty, emit a warning
			std::cerr << "[eclassmgr] attribute " << key << " already set on entityclass " 
            		  << sName << std::endl;
		}
    } // while true
}

const IEntityClass::InheritanceChain& Doom3EntityClass::getInheritanceChain() {
	return _inheritanceChain;
}

void Doom3EntityClass::buildInheritanceChain() {
	_inheritanceChain.clear();

	// We start with the name of this class
	_inheritanceChain.push_back(_name);
	
	// Walk up the inheritance chain
	std::string parentClassName = getAttribute("inherit").value;
	while (!parentClassName.empty()) {
		_inheritanceChain.push_front(parentClassName);

		// Get the parent eclass
		IEntityClassPtr parentClass = GlobalEntityClassManager().findClass(parentClassName);

		if (parentClass == NULL) {
			break;
		}

		parentClassName = parentClass->getAttribute("inherit").value;
	}
}

} // namespace eclass
