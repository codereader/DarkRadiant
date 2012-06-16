#include "FileTypeRegistry.h"

#include "i18n.h"
#include "itextstream.h"
#include "os/path.h"
#include "debugging/debugging.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>

FileTypeRegistry::FileTypeRegistry()
{
	registerPattern("*", FileTypePattern(_("All Files"), "*", "*.*"));
}

void FileTypeRegistry::registerPattern(const std::string& fileType, 
									   const FileTypePattern& pattern)
{
	// Convert the file extension to lowercase
	std::string fileTypeLower = boost::algorithm::to_lower_copy(fileType);

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
	boost::algorithm::to_lower(patternLocal.extension);
	boost::algorithm::to_lower(patternLocal.pattern);

	// Don't accept pre-filled module association
	patternLocal.associatedModule.clear();

	// Check if the pattern is already associated
	for (FileTypePatterns::const_iterator p = patternList.begin(); p != patternList.end(); ++p)
	{
		if (p->extension == patternLocal.extension)
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
	FileTypes::iterator i = _fileTypes.find(boost::algorithm::to_lower_copy(fileType));

	return i != _fileTypes.end() ? i->second : FileTypePatterns();
}

bool FileTypeRegistry::registerModule(const std::string& fileType, 
									  const std::string& extension,
									  const std::string& moduleName)
{
	// Convert the file extension to lowercase and try to find the matching list
	FileTypes::iterator i = _fileTypes.find(boost::algorithm::to_lower_copy(fileType));

	if (i == _fileTypes.end())
	{
		return false;
	}

	FileTypePatterns& patterns = i->second;

	std::string extLower = boost::algorithm::to_lower_copy(extension);

	// Check if the pattern is already associated with a module
	for (FileTypePatterns::iterator p = patterns.begin(); p != patterns.end(); ++p)
	{
		if (p->extension == extLower)
		{
			if (p->associatedModule.empty())
			{
				// Found module, and it's not associated yet, go ahead
				p->associatedModule = moduleName;
				return true;
			}
			else
			{
				// Already associated, return false
				return false;
			}
		}
	}

	return false; // extension not found
}

void FileTypeRegistry::unregisterModule(const std::string& moduleName)
{
	// Iterate over all file types and patterns and remove any matching module associations
	for (FileTypes::iterator i = _fileTypes.begin(); i != _fileTypes.end(); ++i)
	{
		for (FileTypePatterns::iterator p = i->second.begin(); p != i->second.end(); ++p)
		{
			if (p->associatedModule == moduleName)
			{
				p->associatedModule.clear();
			}
		}
	}
}

std::string FileTypeRegistry::findModule(const std::string& fileType, 
										 const std::string& extension)
{
	// Convert the file extension to lowercase and try to find the matching list
	FileTypes::iterator i = _fileTypes.find(boost::algorithm::to_lower_copy(fileType));

	if (i == _fileTypes.end())
	{
		return "";
	}

	std::string extLower = boost::algorithm::to_lower_copy(extension);

	FileTypePatterns& patterns = i->second;
	
	for (FileTypePatterns::const_iterator p = patterns.begin(); p != patterns.end(); ++p)
	{
		if (p->extension == extLower)
		{
			// Extension matches, return module name (even if it's empty)
			return p->associatedModule;
		}
	}

	return ""; // nothing found
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
	registry.registerModule(FileTypeRegistryPtr(new FileTypeRegistry));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
