#include "MRUMenuItem.h"

#include "MRU.h"

namespace ui
{

// Constructor
MRUMenuItem::MRUMenuItem(const std::string& filename, ui::MRU& mru, unsigned int index) :
	_mapFilename(filename),
	_mru(mru),
	_index(index)
{}

// Copy Constructor
MRUMenuItem::MRUMenuItem(const ui::MRUMenuItem& other) :
	_mapFilename(other._mapFilename),
	_mru(other._mru),
	_index(other._index)
{}

void MRUMenuItem::activate(const cmd::ArgumentList& args)
{
	// Only pass the call if the MenuItem is not the empty menu item (with index == 0)
	if (!_mapFilename.empty() && _index > 0)
	{
		// Pass the call back to the main MRU class to do the logistics
		_mru.loadMap(_mapFilename);
	}
}

void MRUMenuItem::setMapFilename(const std::string& filename)
{
	// Update the internal storage
	_mapFilename = filename;
}

const std::string& MRUMenuItem::getMapFilename() const
{
	return _mapFilename;
}

int MRUMenuItem::getIndex() const
{
	return _index;
}

} // namespace ui
