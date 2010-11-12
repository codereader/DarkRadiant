#include "FileSystemInterface.h"

#include "generic/callback.h"
#include "iarchive.h"
#include "itextstream.h"

namespace script {

void FileSystemInterface::forEachFile(const std::string& basedir,
	const std::string& extension, VirtualFileSystem::Visitor& visitor, std::size_t depth)
{
	GlobalFileSystem().forEachFile(basedir, extension, visitor, depth);
}

std::string FileSystemInterface::readTextFile(const std::string& filename)
{
	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(filename);

	if (file == NULL) return "";

	TextInputStream& istream = file->getInputStream();

	const std::size_t READSIZE = 16384;

	std::string text;
	char buffer[READSIZE];
	std::size_t bytesRead = READSIZE;

	do {
		bytesRead = istream.read(buffer, READSIZE);

		// Copy the stuff to the string
		text.append(buffer, bytesRead);

	} while (bytesRead == READSIZE);

	return text;
}

int FileSystemInterface::getFileCount(const std::string& filename) {
	return GlobalFileSystem().getFileCount(filename);
}

std::string FileSystemInterface::findFile(const std::string& name) {
	return GlobalFileSystem().findFile(name);
}

std::string FileSystemInterface::findRoot(const std::string& name) {
	return GlobalFileSystem().findRoot(name);
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
		.def("findFile", &FileSystemInterface::findFile)
		.def("findRoot", &FileSystemInterface::findRoot)
		.def("readTextFile", &FileSystemInterface::readTextFile)
		.def("getFileCount", &FileSystemInterface::getFileCount)
	;

	// Now point the Python variable "GlobalFileSystem" to this instance
	nspace["GlobalFileSystem"] = boost::python::ptr(this);
}

} // namespace script
