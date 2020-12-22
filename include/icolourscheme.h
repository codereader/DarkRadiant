#pragma once

#include "imodule.h"
#include <functional>
#include "math/Vector3.h"

namespace colours
{

/**
 * Registry key set when light volumes should be rendered in a single colour
 * set by the colourscheme, rather than the colour contained in their _color
 * key.
 */
constexpr const char* RKEY_OVERRIDE_LIGHTCOL = "user/ui/colour/overrideLightColour";

class IColourItem
{
public:
	virtual ~IColourItem() {}

	// Const and non-const colour accessors
	virtual const Vector3& getColour() const = 0;
	virtual Vector3& getColour() = 0;
};

/// Interface for a single colour scheme
class IColourScheme
{
public:
	virtual ~IColourScheme() {}

	// Iterate over all colours in this scheme
	virtual void foreachColour(const std::function<void(const std::string& name, IColourItem& colour)>& functor) = 0;
	virtual void foreachColour(const std::function<void(const std::string& name, const IColourItem& colour)>& functor) const = 0;

	// Returns the requested colour object
	virtual IColourItem& getColour(const std::string& colourName) = 0;

	// returns the name of this colour scheme
	virtual const std::string& getName() const = 0;

	// returns true if the scheme is read-only
	virtual bool isReadOnly() const = 0;
};

/// Module providing access to current and available colour schemes
class IColourSchemeManager :
	public RegisterableModule
{
public:
	virtual ~IColourSchemeManager() {}

	// Visit each known colour scheme with the given functor
	virtual void foreachScheme(const std::function<void(const std::string&, IColourScheme&)>& functor) = 0;

	// Get the named scheme
	virtual IColourScheme& getColourScheme(const std::string& schemeName) = 0;

	// Checks if the specified scheme already exists
	virtual bool schemeExists(const std::string& name) = 0;

	virtual void setActive(const std::string& name) = 0;

    /// Get a reference to the currently active colour scheme
	virtual IColourScheme& getActiveScheme() = 0;

	// greebo: Returns the named colour, returns <0,0,0> if not found
	virtual Vector3 getColour(const std::string& colourName) = 0;

	virtual void deleteScheme(const std::string& name) = 0;
	virtual void copyScheme(const std::string& fromName, const std::string& toName) = 0;

	// Loads/Saves all the schemes from/to the registry
	virtual void loadColourSchemes() = 0;
	virtual void saveColourSchemes() = 0;

	// Reverts all changes to the current objects and re-load them from the registry
	virtual void restoreColourSchemes() = 0;

    // Explicitly store the entity class overrides in the EclassColourManager
    // This is done by restore/saveColourSchemes automatically and doesn't need to be
    // called by client code. This is used to let the colour scheme editor force an 
    // update of the eclasses in the current scene to allow for a better preview
    virtual void emitEclassOverrides() = 0;
};

}

const char* const MODULE_COLOURSCHEME_MANAGER("ColourSchemeManager");

inline colours::IColourSchemeManager& GlobalColourSchemeManager()
{
	static module::InstanceReference<colours::IColourSchemeManager> _reference(MODULE_COLOURSCHEME_MANAGER);
	return _reference;
}
