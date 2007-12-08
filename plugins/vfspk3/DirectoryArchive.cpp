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

void DirectoryArchive::forEachFile(VisitorFunc visitor, const char* root) {
	std::vector<Directory*> dirs;
	UnixPath path(_root);
	path.push(root);
	dirs.push_back(directory_open(path.c_str()));

	while (!dirs.empty() && directory_good(dirs.back())) {
		const char* name = directory_read_and_increment(dirs.back());

		if (name == 0) {
			directory_close(dirs.back());
			dirs.pop_back();
			path.pop();
		}
		else if (!string_equal(name, ".") && !string_equal(name, "..")) {
			path.push_filename(name);

			bool is_directory = file_is_directory(path.c_str());

			if (!is_directory) {
				visitor.file(path_make_relative(path.c_str(), _root.c_str()));
			}

			path.pop();

			if (is_directory) {
				path.push(name);

				if (!visitor.directory(path_make_relative(path.c_str(),
						_root.c_str()), dirs.size()))
				{
					dirs.push_back(directory_open(path.c_str()));
				}
				else {
					path.pop();
				}
			}
		}
	}
}
