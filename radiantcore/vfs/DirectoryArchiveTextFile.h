#pragma once

#include "iarchive.h"
#include "gamelib.h"
#include "stream/TextFileInputStream.h"

namespace archive
{

/// \brief An ArchiveTextFile which is stored as a single file on disk.
class DirectoryArchiveTextFile :
	public ArchiveTextFile
{
private:
	std::string _name;
	TextFileInputStream _inputStream;

	// Mod directory root
	std::string _modRoot;

public:

	DirectoryArchiveTextFile(const std::string& name,
							 const std::string& modRoot,
							 const std::string& filename) : 
		_name(name),
		_inputStream(filename),
		_modRoot(modRoot)
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

	std::string getModName() const override
	{
		return game::current::getModPath(_modRoot);
	}
};

}
