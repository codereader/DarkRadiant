#ifndef _FILESYSTEM_INTERFACE_H_
#define _FILESYSTEM_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

#include "ifilesystem.h"

namespace script {

// A File visitor used for calls to GlobalFileSystem.foreachFile
class FileVisitor
{
public:
	virtual void visit(const std::string& filename) = 0;
};

// Scripts will derive from this class
class FileVisitorWrapper :
	public FileVisitor,
	public boost::python::wrapper<FileVisitor>
{
public:
	void visit(const std::string& filename) {
		// Wrap this method to python
		this->get_override("visit")(filename);
	}
};

/**
 * greebo: This class registers the VFS interface with the
 * scripting system.
 */
class FileSystemInterface :
	public IScriptInterface
{
public:
	void forEachFile(const std::string& basedir, const std::string& extension, 
					  FileVisitor& visitor, std::size_t depth);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<FileSystemInterface> FileSystemInterfacePtr;

} // namespace script

#endif /* _FILESYSTEM_INTERFACE_H_ */
