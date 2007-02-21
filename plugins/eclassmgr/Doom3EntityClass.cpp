#include "Doom3EntityClass.h"

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
  _maxs(maxs)
{
	// Capture the shaders
	captureColour();		
}

// Static function to create an EntityClass (named constructor idiom)
IEntityClassPtr Doom3EntityClass::create(const std::string& name, bool brushes) {
	if (!brushes) {
		return IEntityClassPtr(new Doom3EntityClass(name, 
													Vector3(0, 0.4, 0),
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

	GlobalShaderCache().release(fillCol);
	GlobalShaderCache().release(wireCol);
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


} // namespace eclass
