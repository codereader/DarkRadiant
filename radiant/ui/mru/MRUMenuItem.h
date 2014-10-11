#pragma once

#include <string>
#include "icommandsystem.h"

// Forward declaration
class wxMenuItem;

namespace ui {

// Forward declaration
class MRU;

/* greebo: An MRUMenuItem holds the information of a single menu entry,
 * this consists mainly of the map filename and the MRU number.
 */
class MRUMenuItem
{
private:
	// The filename behind this item
	std::string _mapFilename;

	// The reference to the main class for loading maps and stuff
	MRU& _mru;

	// The number of this MRU item to be displayed
	unsigned int _index;

public:
	// Constructor
	MRUMenuItem(const std::string& mapFilename, ui::MRU& _mru, unsigned int index);

	// Copy Constructor
	MRUMenuItem(const ui::MRUMenuItem& other);

	// Triggers loading the map represented by this widget
	void activate(const cmd::ArgumentList& args);

	// Sets/Retrieves the label
	void setMapFilename(const std::string& filename);
	const std::string& getMapFilename() const;

	int getIndex() const;

}; // class MRUMenuItem

} // namespace ui
