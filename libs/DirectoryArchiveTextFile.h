#pragma once

#include "iarchive.h"
#include "archivelib.h"

namespace archive
{

/// \brief An ArchiveTextFile which is stored as a single file on disk.
class DirectoryArchiveTextFile :
	public ArchiveTextFile
{
private:
	std::string _name;
	TextFileInputStream _inputStream;

	// Mod directory
	std::string _modName;

public:

	DirectoryArchiveTextFile(const std::string& name,
							 const std::string& modName,
							 const std::string& filename) : 
		_name(name),
		_inputStream(filename),
		_modName(modName)
	{}

	bool failed() const 
	{
		return _inputStream.failed();
	}

	const std::string& getName() const override
	{
		return _name;
	}

	TextInputStream& getInputStream() override
	{
		return _inputStream;
	}

	/**
	* Get mod directory.
	*/
	std::string getModName() const override
	{
		return _modName;
	}
};

}
