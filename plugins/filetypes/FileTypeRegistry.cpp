#include "FileTypeRegistry.h"

#include "i18n.h"
#include "itextstream.h"
#include "os/path.h"
#include "debugging/debugging.h"

#include <boost/algorithm/string/predicate.hpp>
#include "string/case_conv.h"

FileTypeRegistry::FileTypeRegistry()
{
	registerPattern("*", FileTypePattern(_("All Files"), "*", "*.*"));
}

void FileTypeRegistry::registerPattern(const std::string& fileType, const FileTypePattern& pattern)
{
	// Convert the file extension to lowercase
	std::string fileTypeLower = string::to_lower_copy(fileType);

	// Find or insert the fileType into the map
	FileTypes::iterator i = _fileTypes.find(fileTypeLower);

	if (i == _fileTypes.end())
	{
		// Not found yet, insert an empty pattern list
		i = _fileTypes.insert(FileTypes::value_type(fileTypeLower, FileTypePatterns())).first;
	}

	// At this point we have a valid iterator
	FileTypePatterns& patternList = i->second;

	// Ensure the pattern contains a lowercase extension
	FileTypePattern patternLocal = pattern;
	string::to_lower(patternLocal.extension);
	string::to_lower(patternLocal.pattern);

	// Check if the pattern is already associated
	for (const FileTypePattern& pattern : patternList)
	{
		if (pattern.extension == patternLocal.extension)
		{
			// Ignore this pattern
			return;
		}
	}

	// Insert the pattern at the end of the list
	patternList.push_back(patternLocal);
}

FileTypePatterns FileTypeRegistry::getPatternsForType(const std::string& fileType)
{
	// Convert the file extension to lowercase and try to find the matching list
	FileTypes::const_iterator i = _fileTypes.find(string::to_lower_copy(fileType));

	return i != _fileTypes.end() ? i->second : FileTypePatterns();
}

const std::string& FileTypeRegistry::getName() const
{
	static std::string _name(MODULE_FILETYPES);
	return _name;
}

const StringSet& FileTypeRegistry::getDependencies() const
{
	static StringSet _dependencies; // no dependencies
	return _dependencies;
}

void FileTypeRegistry::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;
}

// This will be called by the DarkRadiant main binary's ModuleRegistry
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<FileTypeRegistry>());
}
