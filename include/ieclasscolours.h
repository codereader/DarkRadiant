#pragma once

#include <functional>
#include <sigc++/signal.h>
#include "imodule.h"
#include "ieclass.h"
#include "math/Vector3.h"

namespace eclass
{

/**
 * Manages the entity class colour overrides that are applied
 * to certain eclasses as defined in the currently active
 * colour scheme.
 */
class IColourManager :
    public RegisterableModule
{
public:
    virtual ~IColourManager() {}

    // Register an override for the given entity class, such that the wire/fill shader
    // colours of the named entity class are using the given colour instead of the one
    // defined in the entityDef block.
    // Adding an override for an entity class will replace any existing overrides.
    // The colour is given in RGB values with each component in the interval [0..1].
    virtual void addOverrideColour(const std::string& eclass, const Vector4& colour) = 0;

    /**
     * \brief Applies a possible colour override to the given entity class.
     *
     * If an override was found, the entity class's colour will be changed with
     * setColour().
     *
     * \return true if an override was found, false otherwise.
     */
    virtual bool applyColours(IEntityClass& eclass) = 0;

    // Visit each override definition with the given functor
    virtual void foreachOverrideColour(const std::function<void(const std::string&, const Vector4&)>& functor) = 0;

    // Removes the override colour for the given entity class
    virtual void removeOverrideColour(const std::string& eclass) = 0;

    // Removes all registered overrides
    virtual void clearOverrideColours() = 0;

    // Signal invoked when an override of a specific eclass is added, changed or removed
    // function signature: void(const std::string& eclass, bool hasBeenRemoved)
    virtual sigc::signal<void, const std::string&, bool>& sig_overrideColourChanged() = 0;
};

}

const char* const MODULE_ECLASS_COLOUR_MANAGER("EclassColourManager");

/**
 * Return the global IEClassColourManager to the application.
 * \ingroup eclass
 */
inline eclass::IColourManager& GlobalEclassColourManager()
{
    static module::InstanceReference<eclass::IColourManager> _reference(MODULE_ECLASS_COLOUR_MANAGER);
    return _reference;
}
