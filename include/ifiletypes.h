#pragma once

#include <list>
#include <string>
#include "imodule.h"

/**
 * Simple structure to store a file pattern (e.g. "*.map") 
 * along with its name (e.g. "Map files") and extension.
 *
 * If a module has been registering itself for a certain
 * filetype/extension combo, its name is in associatedModule.
 */
struct FileTypePattern
{
	// The user-friendly name ("Doom 3 Map")
	std::string name;

	// The extension in lowercase ("map")
	std::string extension;

	// The mask pattern ("*.map")
	std::string pattern;

	// The module associated with this specific extension
	// This is initially empty, will be filled when
	// GlobalFiletypes().registerModule() is called.
	std::string associatedModule;

	// Constructor with optional initialisation parameters
	FileTypePattern(const std::string& name_ = "", 
					const std::string& extension_ = "", 
					const std::string& pattern_ = "") : 
		name(name_),
		extension(extension_),
		pattern(pattern_)
	{}
};
typedef std::list<FileTypePattern> FileTypePatterns;

const char* const MODULE_FILETYPES = "FileTypes";

/**
 * Interface for the FileType registry module. This module retains a list of
 * FileTypePattern objects along with their associated module names.
 */
class IFileTypeRegistry :
	public RegisterableModule
{
public:
	/**
	 * greebo: Registers an extension (e.g. "map") for a certain file type ("prefab").
	 * Common file types are "map", "prefab" and "model", each of them can have one 
	 * or more extensions associated in a certain order ("prefab" => "pfb", "map", "reg",
	 * or "map" => "map", "reg", "pfb"). The order is important e.g. for the file 
	 * filters in the Map Open dialog.
	 *
	 * The pattern argument is a structure containing the lowercase extensions as well
	 * as the display name and the filter pattern used in the file chooser dialogs.
	 *
	 * If an extension is already associated with the given filetype, it is ignored.
	 * New extensions are added to the end of the list.
	 *
	 * @param fileType: the file type which an extension should be associated to.
	 * @param pattern: the extension as well as the display name and a pattern ("*.map")
	 */
	virtual void registerPattern(const std::string& fileType, 
								 const FileTypePattern& pattern) = 0;

	/**
	 * Retrieve a list of patterns for the given file type (e.g. "prefab" or "map").
	 *
	 * @returns: a list of FileTypePatterns containing extension, display name, etc.
	 */
	virtual FileTypePatterns getPatternsForType(const std::string& fileType) = 0;

	/**
	 * Registers the named module with the extension for the given fileType.
	 *
	 * Example: Associate the module "ModelLoaderASE" with the "ase" extension 
	 * for the "model" filetype.
	 *
	 * Only a single module can be registered for a file type/extension combo 
	 * at any time.
	 *
	 * @returns: TRUE if the module could be registered, FALSE if the filetype/extension
	 * combination is already "in use" or the file type is not registered at all.
	 */
	virtual bool registerModule(const std::string& fileType, 
								const std::string& extension,
								const std::string& moduleName) = 0;

	/**
	 * Removes the module from all filetypes - usually done at module shutdown.
	 */
	virtual void unregisterModule(const std::string& moduleName) = 0;

	/**
	 * Find the name of the module which is able to handle the given extension.
	 *
	 * Note that Map Loader Modules are handled slightly differently - they don't
	 * add themselves directly to a specific extension, but register in the 
	 * GlobalMapFormatManager(). The Map loading algorithm is querying the format
	 * manager for any module that is capable of loading the map, so findModule() 
	 * isn't necessary here. The main purpose of findModule() is to deliver
	 * a ModelLoader for a specific model file extension, e.g. "ase" or "lwo".
	 */
	virtual std::string findModule(const std::string& fileType, 
								   const std::string& extension) = 0;
};

inline IFileTypeRegistry& GlobalFiletypes()
{
	// Cache the reference locally
	static IFileTypeRegistry& _fileTypes(
		*boost::static_pointer_cast<IFileTypeRegistry>(
			module::GlobalModuleRegistry().getModule(MODULE_FILETYPES)
		)
	);
	return _fileTypes;
}
