#include "MRU.h"

#include "i18n.h"
#include "imap.h"
#include "icommandsystem.h"
#include "ipreferencesystem.h"

#include "string/string.h"
#include <fmt/format.h>
#include "os/file.h"
#include "registry/registry.h"
#include "command/ExecutionFailure.h"
#include "module/StaticModule.h"

namespace map
{

namespace
{
	const std::string RKEY_MAP_ROOT = "user/ui/map";
	const std::string RKEY_MAP_MRUS = RKEY_MAP_ROOT + "/MRU";
	const std::string RKEY_MRU_LENGTH = RKEY_MAP_ROOT + "/numMRU";
}

sigc::signal<void>& MRU::signal_MapListChanged()
{
	return _signalMapListChanged;
}

void MRU::loadRecentFiles()
{
	// Loads the registry values from the last to the first (recentMap4 ... recentMap1) and
	// inserts them. After everything is loaded, the file list is sorted correctly.
	for (std::size_t i = _numMaxFiles; i > 0; i--)
	{
		const std::string key = RKEY_MAP_MRUS + "/map" + string::to_string(i);
		auto fileName = registry::getValue<std::string>(key);

		if (!fileName.empty())
		{
			// Insert the item into the filelist
			_list->insert(fileName);
		}
	}
}

void MRU::saveRecentFiles()
{
	// Delete all existing MRU/element nodes
	GlobalRegistry().deleteXPath(RKEY_MAP_MRUS);

	std::size_t counter = 1;

	// Now wade through the list and save them in the correct order
	for (MRUList::const_iterator i = _list->begin(); i != _list->end(); ++counter, ++i)
	{
		const std::string key = RKEY_MAP_MRUS + "/map" + string::to_string(counter);

		// Save the string into the registry
		registry::setValue(key, (*i));
	}
}

void MRU::constructPreferences()
{
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Map Files"));

	page.appendEntry(_("Number of most recently used files"), RKEY_MRU_LENGTH);
	page.appendCheckBox(_("Open last map on startup"), RKEY_LOAD_LAST_MAP);
}

std::string MRU::getLastMapName()
{
	return _list->empty() ? "" : *_list->begin();
}

std::size_t MRU::getMaxNumberOfItems() const
{
	return _numMaxFiles;
}

void MRU::foreachItem(const ItemFunctor& functor)
{
	std::size_t counter = 1;

	// Now wade through the list and save them in the correct order
	for (auto i = _list->begin(); i != _list->end(); ++counter, ++i)
	{
		functor(counter, *i);
	}
}

void MRU::insert(const std::string& fileName) {

	if (!fileName.empty())
	{
		// Insert the item into the filelist
		_list->insert(fileName);

		// Persist MRU to the registry on each change
		saveRecentFiles();

		_signalMapListChanged.emit();
	}
}

void MRU::loadMRUMap(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: LoadMRUMap <index:1..N>" << std::endl;
		return;
	}

	int index = args[0].getInt();

	if (index < 1 || static_cast<std::size_t>(index) > _numMaxFiles)
	{
		throw cmd::ExecutionFailure(fmt::format(_("Index out of range: {0:d}"), index));
	}

	// Look up the item with the given index and execute it
	foreachItem([=](std::size_t n, const std::string& filename)
	{
		if (static_cast<std::size_t>(index) == n)
		{
			GlobalCommandSystem().executeCommand("OpenMap", filename);
		}
	});
}

const std::string& MRU::getName() const
{
	static std::string _name(MODULE_MRU_MANAGER);
	return _name;
}

const StringSet& MRU::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void MRU::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	constructPreferences();

	_numMaxFiles = registry::getValue<std::size_t>(RKEY_MRU_LENGTH);
	_list.reset(new MRUList(_numMaxFiles));

	GlobalCommandSystem().addCommand(LOAD_MRU_MAP_CMD,
		std::bind(&MRU::loadMRUMap, this, std::placeholders::_1), { cmd::ARGTYPE_INT });

	// Create shortcuts for the items we can hold
	for (std::size_t i = 1; i <= _numMaxFiles; i++)
	{
		auto statementName = fmt::format(LOAD_MRU_STATEMENT_FORMAT, i);
		auto statementValue = fmt::format("{0} {1:d}", LOAD_MRU_MAP_CMD, i);

		GlobalCommandSystem().addStatement(statementName, statementValue, false);
	}

	// Load the most recently used files list from the registry
	loadRecentFiles();
}

void MRU::shutdownModule()
{
	saveRecentFiles();
}

module::StaticModuleRegistration<MRU> mruModule;

} // namespace
