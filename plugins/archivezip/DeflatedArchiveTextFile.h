#pragma once

#include "iarchive.h"
#include "iregistry.h"
#include "archivelib.h"

namespace archive
{

/**
 * ArchiveFile stored in a ZIP in DEFLATE format.
 */
class DeflatedArchiveTextFile :
	public ArchiveTextFile
{
private:
	std::string _name;
	FileInputStream _istream;
	SubFileInputStream _substream;	// reads subset of _istream
	DeflatedInputStream _zipstream;	// inflates data from _substream
	BinaryToTextInputStream<DeflatedInputStream> _textStream; // converts data from _zipstream

    // Mod directory containing this file
    const std::string _modName;

public:
	typedef FileInputStream::size_type size_type;
	typedef FileInputStream::position_type position_type;

    /**
     * Constructor.
     *
     * @param modDir
     * The name of the mod directory this file's archive is located in.
     */
    DeflatedArchiveTextFile(const std::string& name,
                            const std::string& archiveName, // full path to ZIP file
                            const std::string& modName,
                            position_type position,
                            size_type stream_size) : 
		_name(name),
		_istream(archiveName),
		_substream(_istream, position, stream_size),
		_zipstream(_substream),
		_textStream(_zipstream),
		_modName(modName)
    {}

	TextInputStream& getInputStream() override
	{
		return _textStream;
	}

	const std::string& getName() const override
	{
		return _name;
	}

    /**
     * Return mod directory of this file.
     */
    std::string getModName() const override
	{
        return _modName;
    }
};

}
