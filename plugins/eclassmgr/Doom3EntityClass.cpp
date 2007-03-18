#include "Doom3EntityClass.h"
#include "AttributeCopyingVisitor.h"

#include "iradiant.h"

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
  _mins(mins),
  _maxs(maxs),
  _inheritanceResolved(false)
{
	// Set the entity colour to default, if none was specified
	if (_colour == Vector3(-1, -1, -1)) {
		_colour = GlobalRadiant().getColour("default_entity");
	}
	// Capture the shaders
	captureColour();		
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
	// Release fill and wire versions of the entity colour
	std::string fillCol = (boost::format("(%g %g %g)") % _colour[0] % _colour[1] % _colour[2]).str();
	std::string wireCol = (boost::format("<%g %g %g>") % _colour[0] % _colour[1] % _colour[2]).str();
}

// Enumerate entity class attributes
void Doom3EntityClass::forEachClassAttribute(EntityClassAttributeVisitor& visitor) const {
	for (EntityAttributeMap::const_iterator i = _attributes.begin();
		 i != _attributes.end();
		 ++i)
	{
		// Visit only if it is not an "editor_" key
		if (!boost::algorithm::istarts_with(i->first, "editor_"))
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
	std::string parName = getValueForKey("inherit");
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

		// Copy attributes from the parent to the child
		AttributeCopyingVisitor visitor(*this);
		par->forEachClassAttribute(visitor);
	}
	else {
		std::cout << "[eclassmgr] Warning: Entity class "
				  << _name << " specifies parent "
				  << parName << " which is not found." << std::endl;
	} 

	// Set the resolved flag
	_inheritanceResolved = true;
}

} // namespace eclass
