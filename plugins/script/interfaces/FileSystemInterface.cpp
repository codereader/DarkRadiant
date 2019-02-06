#include "FileSystemInterface.h"

#include "generic/callback.h"
#include "iarchive.h"
#include "itextstream.h"

namespace script
{

void FileSystemInterface::forEachFile(const std::string& basedir,
                                      const std::string& extension, 
                                      VirtualFileSystemVisitor& visitor,
                                      std::size_t depth)
{
    GlobalFileSystem().forEachFile(basedir, extension, [&](const std::string& filename, vfs::Visibility)
    {
        visitor.visit(filename);
    }, depth);
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

	do 
	{
		bytesRead = istream.read(buffer, READSIZE);

		// Copy the stuff to the string
		text.append(buffer, bytesRead);

	} while (bytesRead == READSIZE);

	return text;
}

int FileSystemInterface::getFileCount(const std::string& filename)
{
	return GlobalFileSystem().getFileCount(filename);
}

std::string FileSystemInterface::findFile(const std::string& name)
{
	return GlobalFileSystem().findFile(name);
}

std::string FileSystemInterface::findRoot(const std::string& name)
{
	return GlobalFileSystem().findRoot(name);
}

void FileSystemInterface::registerInterface(py::module& scope, py::dict& globals) 
{
	// Expose the FileVisitor interface
	py::class_<VirtualFileSystemVisitor, FileVisitorWrapper> visitor(scope, "FileVisitor");
	visitor.def(py::init<>());
	visitor.def("visit", &VirtualFileSystemVisitor::visit);

	// Add the VFS module declaration to the given python namespace
	py::class_<FileSystemInterface> filesystem(scope, "FileSystem");
	filesystem.def("forEachFile", &FileSystemInterface::forEachFile);
	filesystem.def("findFile", &FileSystemInterface::findFile);
	filesystem.def("findRoot", &FileSystemInterface::findRoot);
	filesystem.def("readTextFile", &FileSystemInterface::readTextFile);
	filesystem.def("getFileCount", &FileSystemInterface::getFileCount);

	// Now point the Python variable "GlobalFileSystem" to this instance
	globals["GlobalFileSystem"] = this;
}

} // namespace script
