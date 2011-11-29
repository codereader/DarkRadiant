#include "DirectoryArchive.h"

#include "archivelib.h"
#include "UnixPath.h"
#include "os/file.h"
#include "os/dir.h"
#include "os/fs.h"
#include <vector>

namespace fs = boost::filesystem;

DirectoryArchive::DirectoryArchive(const std::string& root) :
	_root(root)
{}

ArchiveFilePtr DirectoryArchive::openFile(const std::string& name) {
	UnixPath path(_root);
	path.push_filename(name);

	DirectoryArchiveFilePtr file(new DirectoryArchiveFile(name, path));

	if (!file->failed()) {
		return file;
	}

	return ArchiveFilePtr();
}

ArchiveTextFilePtr DirectoryArchive::openTextFile(const std::string& name) {
	UnixPath path(_root);
	path.push_filename(name);

	DirectoryArchiveTextFilePtr file(new DirectoryArchiveTextFile(name, _root, path));

	if (!file->failed()) {
		return file;
	}

	return ArchiveTextFilePtr();
}

bool DirectoryArchive::containsFile(const std::string& name) {
	UnixPath path(_root);
	path.push_filename(name);
	return file_readable(path.c_str());
}

#include <iostream>

void DirectoryArchive::forEachFile(VisitorFunc visitor, const std::string& root) {
	// Initialise the search's starting point
	fs::path start(_root + root);

	if (!fs::exists(start)) {
		return;
	}

	// For cutting off the base path
	std::size_t rootLen = _root.length();

	typedef fs::recursive_directory_iterator DirIter;
	for (fs::recursive_directory_iterator it(start); it != fs::recursive_directory_iterator(); ++it)
	{
		// Get the candidate
		const fs::path& candidate = *it;
        std::string candidateStr = os::string_from_path(candidate);

		if (fs::is_directory(candidate))
		{
			// Check if we should traverse further
			if (visitor.directory(candidateStr.substr(rootLen), it.level()+1))
			{
				// Visitor returned true, prevent going deeper into it
				it.no_push();
			}
		}
		else
		{
			// File
			visitor.file(candidateStr.substr(rootLen));
		}
	}
}
