#include "DirectoryArchive.h"

#include "archivelib.h"
#include "UnixPath.h"
#include "os/file.h"
#include "os/dir.h"
#include <vector>

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

bool DirectoryArchive::containsFile(const char* name) {
	UnixPath path(_root);
	path.push_filename(name);
	return file_readable(path.c_str());
}

#include <iostream>

void DirectoryArchive::forEachFile(VisitorFunc visitor, const std::string& root) {
	std::vector<Directory*> dirs;
	
	// Initialise the search's starting point
	UnixPath path(_root);
	path.push(root);
	
	// Open the starting directory and enter the recursion
	dirs.push_back(directory_open(path));

	while (!dirs.empty() && directory_good(dirs.back())) {
		// Get a new filename
		const char* name = directory_read_and_increment(dirs.back());

		if (name == 0) {
			// Finished traversing this directory
			directory_close(dirs.back());
			dirs.pop_back();
			path.pop();
			continue;
		}
		
		// Non-NULL filename
		std::string filename(name);
		if (filename == "." || filename == "..") {
			continue;
		}
		
		// Assemble the full filename
		path.push_filename(filename);

		bool is_directory = file_is_directory(path.c_str());

		if (!is_directory) {
			visitor.file(os::getRelativePath(path, _root));
			
			// Remove the filename again, continue to search the dir
			path.pop();
		}
		else {
			// We've got a folder, push a slash and attempt to traverse
			path.push("/");

			if (!visitor.directory(os::getRelativePath(path, _root), dirs.size())) {
				// Open this new directory and push the handle
				dirs.push_back(directory_open(path));
			}
			else {
				// Visitor says: don't traverse, pop the path and continue
				path.pop();
			}
		}
	}
}
