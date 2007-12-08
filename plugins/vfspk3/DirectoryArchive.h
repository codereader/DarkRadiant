#ifndef DIRECTORYARCHIVE_H_
#define DIRECTORYARCHIVE_H_

#include "iarchive.h"

/**
 * greebo: This wraps around a certain path in the "real"
 *         filesystem on the user's hard drive. 
 * 
 * A real folder is treated like any other "archive" and gets
 * added to the list of PK4 archives, using this class.
 */
class DirectoryArchive : 
	public Archive
{
	std::string _root;
public:
	// Pass the root path to the constructor
	DirectoryArchive(const std::string& root);

	virtual ArchiveFilePtr openFile(const std::string& name);
	
	virtual ArchiveTextFilePtr openTextFile(const std::string& name);
	
	virtual bool containsFile(const char* name);
	
	virtual void forEachFile(VisitorFunc visitor, const std::string& root);
};
typedef boost::shared_ptr<DirectoryArchive> DirectoryArchivePtr;

#endif /*DIRECTORYARCHIVE_H_*/
