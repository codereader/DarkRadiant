#pragma once

#include <boost/python.hpp>
#include "iscript.h"

#include "ifilesystem.h"

namespace script {

/**
* Adaptor interface for a VFS traversor object.
*/
class VirtualFileSystemVisitor
{
public:
    virtual ~VirtualFileSystemVisitor() {}

    /**
    * Required visit method. Takes the filename is relative
    * to the base path passed to the GlobalFileSystem().foreachFile method.
    */
    virtual void visit(const std::string& filename) = 0;
};

// Scripts will derive from this class
class FileVisitorWrapper :
    public VirtualFileSystemVisitor,
    public boost::python::wrapper<VirtualFileSystemVisitor>
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
					  VirtualFileSystemVisitor& visitor, std::size_t depth);

	int getFileCount(const std::string& filename);
	std::string findFile(const std::string& name);
	std::string findRoot(const std::string& name);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef std::shared_ptr<FileSystemInterface> FileSystemInterfacePtr;

} // namespace script
