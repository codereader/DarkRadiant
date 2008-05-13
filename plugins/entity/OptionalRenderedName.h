#ifndef OPTIONALRENDEREDNAME_H_
#define OPTIONALRENDEREDNAME_H_

#include "iregistry.h"

/**
 * \addtogroup entity Entity creator
 */

namespace entity
{

namespace 
{
	const std::string RKEY_SHOW_ENTITY_NAMES("user/ui/xyview/showEntityNames");
}

/**
 * \brief Parent class providing automatic togglable rendered name 
 * functionality to entity subclasses.
 *
 * This class operates as a RegistryKeyObserver which automatically detects
 * changes to the "Show entity names" registry key, and exposes the current
 * status through a simple boolean access function rather than an expensive
 * registry lookup.
 */
class OptionalRenderedName
: public RegistryKeyObserver
{
    // Current rendered name state
    bool _renderName;

protected:

    /**
     * Default constructor. Initialises the name render state to false.
     */
    OptionalRenderedName()
    : _renderName(GlobalRegistry().get(RKEY_SHOW_ENTITY_NAMES) == "1")
    { 
        GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ENTITY_NAMES);
    }

	~OptionalRenderedName()
	{
		GlobalRegistry().removeKeyObserver(this);
	}

    /**
     * Get the current status of the rendered name.
     *
     * @return
     * true if the name should be rendered, false otherwise.
     */
    bool isNameVisible() const
    {
        return _renderName;
    }

public:

    // RegistryKeyObserver implementation
    void keyChanged(const std::string& key, const std::string& value)
    {
        assert(key == RKEY_SHOW_ENTITY_NAMES);
        if (value == "1")
            _renderName = true;
        else
            _renderName = false;
    }
};

} // namespace entity

#endif

