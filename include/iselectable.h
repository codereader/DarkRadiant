#pragma once

/**
 * greebo: A Selectable is everything that can be highlighted
 *         by the user in the scene (e.g. by interaction with the mouse).
 */
class Selectable
{
public:
    // destructor
	virtual ~Selectable() {}

	// Set the selection status of this object
	virtual void setSelected(bool select) = 0;

	// Check the selection status of this object (TRUE == selected)
	virtual bool isSelected() const = 0;

	// Toggle the selection status
	virtual void invertSelected() = 0;
};
typedef boost::shared_ptr<Selectable> SelectablePtr;
