#pragma once

#include <pybind11/pybind11.h>

#include "iscript.h"
#include "ifilesystem.h"

namespace script
{

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
    public VirtualFileSystemVisitor
{
public:
	void visit(const std::string& filename) override
	{
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,			/* Return type */
			VirtualFileSystemVisitor,    /* Parent class */
			visit,			/* Name of function in C++ (must match Python name) */
			filename		/* Argument(s) */
		);
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
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
