#pragma once

#include "imodule.h"
#include <sigc++/signal.h>

namespace map
{

const char* const LOAD_MRU_MAP_CMD = "LoadMRUMap";
const char* const LOAD_MRU_STATEMENT_FORMAT = "MRUOpen{0:d}";

class IMRUManager :
	public RegisterableModule
{
public:
	virtual ~IMRUManager() {}

	typedef std::function<void(std::size_t index, const std::string & fileName)> ItemFunctor;

	virtual std::size_t getMaxNumberOfItems() const = 0;

	// Visits each item, ordered by the 0-based index
	virtual void foreachItem(const ItemFunctor& functor) = 0;

	// Inserts the given map filename at the top of the list
	// Duplicates are relocated, the number of list items is constrained
	virtual void insert(const std::string& fileName) = 0;

	// Returns the filename of the last opened map, or "" if there doesn't exist one
	virtual std::string getLastMapName() = 0;

	virtual sigc::signal<void>& signal_MapListChanged() = 0;
};

}

const char* const MODULE_MRU_MANAGER = "MRUManager";

inline map::IMRUManager& GlobalMRU()
{
	// Cache the reference locally
	static map::IMRUManager& _manager(
		*std::static_pointer_cast<map::IMRUManager>(
			module::GlobalModuleRegistry().getModule(MODULE_MRU_MANAGER)
		)
	);
	return _manager;
}
