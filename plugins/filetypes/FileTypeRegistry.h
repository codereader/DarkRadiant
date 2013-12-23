#pragma once

#include "ifiletypes.h"
#include <map>

/**
 * Implementation of the file type registry. The registry is associating file types
 * with FileTypePatterns. Each FileTypePattern can have a single associated module,
 * which is used to associated ModelLoaders with patterns.
 *
 * <type> => [ <pattern>, <pattern> ]
 *
 * "map" => [	("Map", "*.map", "map", ""), 
 *			    ("Map Backup", "*.bak", "bak", "") ]
 * "prefab" => [ ("Prefab", "*.pfb", "pfb", "") ]
 * "region" => [ ("Region", "*.reg", "reg", "") ]
 *
 * "model" => [ ("ASE", "*.ase", "ase", "ModelLoaderASE"),
 *				("LWO", "*.lwo", "lwo", "ModelLoaderLWO"),
 *				... ]
 *
 * This mapping can be used to retrieve a list of modules capable of
 * loading a file with a given extension. Furthermore it is used by the
 * gtkutil::FileChooser to populate the file type dropdown list.
 */
class FileTypeRegistry :
	public IFileTypeRegistry
{
private:
	typedef std::map<std::string, FileTypePatterns> FileTypes;
	FileTypes _fileTypes;

public:
	/*
	 * Constructor, adds the All Files type.
	 */
	FileTypeRegistry();
	
	// IFileTypeRegistry implementation
	void registerPattern(const std::string& fileType, const FileTypePattern& pattern);

	FileTypePatterns getPatternsForType(const std::string& fileType);

	bool registerModule(const std::string& fileType, 
						const std::string& extension,
						const std::string& moduleName);

	void unregisterModule(const std::string& moduleName);
	
	std::string findModule(const std::string& fileType, const std::string& extension);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<FileTypeRegistry> FileTypeRegistryPtr;
