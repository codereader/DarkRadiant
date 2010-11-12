#ifndef _FILESYSTEM_INTERFACE_H_
#define _FILESYSTEM_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

#include "ifilesystem.h"

namespace script {

// Scripts will derive from this class
class FileVisitorWrapper :
	public VirtualFileSystem::Visitor,
	public boost::python::wrapper<VirtualFileSystem::Visitor>
{
public:
	void visit(const std::string& filename)
	{
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
	// Reads a text file and returns the contents to the script
	// Filename is a VFS path, not an absolute OS path
	// Returns an empty string if not found
	std::string readTextFile(const std::string& filename);

	// Wrapped methods, see "ifilesystem.h" for documentation
	void forEachFile(const std::string& basedir, const std::string& extension,
					  VirtualFileSystem::Visitor& visitor, std::size_t depth);

	int getFileCount(const std::string& filename);
	std::string findFile(const std::string& name);
	std::string findRoot(const std::string& name);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<FileSystemInterface> FileSystemInterfacePtr;

} // namespace script

#endif /* _FILESYSTEM_INTERFACE_H_ */
