#include "FileSystemInterface.h"

#include "generic/callback.h"

namespace script {

void FileSystemInterface::forEachFile(const std::string& basedir, 
	const std::string& extension, FileVisitor& visitor, std::size_t depth)
{
	GlobalFileSystem().forEachFile(
		basedir, 
		extension, 
		MemberCaller1<FileVisitor, const std::string&, &FileVisitor::visit>(visitor),
		depth
	);
}

void FileSystemInterface::registerInterface(boost::python::object& nspace) {
	// Expose the FileVisitor interface
	nspace["FileVisitor"] = 
		boost::python::class_<FileVisitorWrapper, boost::noncopyable>("FileVisitor")
		.def("visit", boost::python::pure_virtual(&FileVisitorWrapper::visit))
	;

	// Add the VFS module declaration to the given python namespace
	nspace["GlobalFileSystem"] = boost::python::class_<FileSystemInterface>("GlobalFileSystem")
		.def("forEachFile", &FileSystemInterface::forEachFile)
	;

	// Now point the Python variable "GlobalFileSystem" to this instance
	nspace["GlobalFileSystem"] = boost::python::ptr(this);
}

} // namespace script
