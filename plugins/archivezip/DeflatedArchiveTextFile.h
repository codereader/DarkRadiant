#ifndef DEFLATEDARCHIVETEXTFILE_H_
#define DEFLATEDARCHIVETEXTFILE_H_

#include "iarchive.h"

/**
 * ArchiveFile stored in a ZIP in DEFLATE format.
 */
class DeflatedArchiveTextFile :
	public ArchiveTextFile
{
	std::string m_name;
	FileInputStream m_istream;
	SubFileInputStream m_substream;
	DeflatedInputStream m_zipstream;
	BinaryToTextInputStream<DeflatedInputStream> m_textStream;
  
    // Mod directory containing this file
    const std::string _modDir;
    
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
                            const std::string& archiveName,
                            const std::string& modDir,
                            position_type position, 
                            size_type stream_size)
    : m_name(name), 
      m_istream(archiveName), 
      m_substream(m_istream, position, stream_size), 
      m_zipstream(m_substream), 
      m_textStream(m_zipstream),
      _modDir(os::getContainingDir(modDir))
    {}

	TextInputStream& getInputStream() {
		return m_textStream;
	}
	
	const std::string& getName() const {
		return m_name;
	}
  
    /**
     * Return mod directory of this file.
     */
    std::string getModName() const {
        return _modDir;
    }
};

#endif /*DEFLATEDARCHIVETEXTFILE_H_*/
