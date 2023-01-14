#include "FileTypeRegistry.h"

#include "i18n.h"
#include "itextstream.h"
#include "os/path.h"
#include "debugging/debugging.h"

#include "string/case_conv.h"
#include "module/StaticModule.h"

void FileTypeRegistry::registerPattern(const std::string& fileType, const FileTypePattern& pattern)
{
	// Convert the file extension to lowercase
	std::string fileTypeLower = string::to_lower_copy(fileType);

	// Find or insert the fileType into the map
	auto i = _fileTypes.find(fileTypeLower);

	if (i == _fileTypes.end())
	{
		// Not found yet, insert an empty pattern list
		i = _fileTypes.emplace(fileTypeLower, FileTypePatterns()).first;
	}

	// At this point we have a valid iterator
	FileTypePatterns& patternList = i->second;

	// Ensure the pattern contains a lowercase extension
	FileTypePattern patternLocal = pattern;
	string::to_lower(patternLocal.extension);
	string::to_lower(patternLocal.pattern);

	// Check if the pattern is already associated
	for (const FileTypePattern& existing : patternList)
	{
		if (existing.extension == patternLocal.extension)
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
	auto i = _fileTypes.find(string::to_lower_copy(fileType));

	return i != _fileTypes.end() ? i->second : FileTypePatterns();
}

std::string FileTypeRegistry::getIconForExtension(const std::string& extension)
{
    auto extLower = string::to_lower_copy(extension);

    // We pick the first icon in any of the patterns matching the extension
    for (const auto& patterns : _fileTypes)
    {
        for (const auto& pattern : patterns.second)
        {
            if (pattern.extension == extension && !pattern.icon.empty())
            {
                return pattern.icon;
            }
        }
    }

    return std::string();
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

void FileTypeRegistry::initialiseModule(const IApplicationContext& ctx)
{
	registerPattern("*", FileTypePattern(_("All Files"), "*", "*.*"));
}

// Static module instance
module::StaticModuleRegistration<FileTypeRegistry> fileTypesModule;
