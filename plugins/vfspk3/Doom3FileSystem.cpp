/*
Copyright (c) 2001, Loki software, inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list 
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Neither the name of Loki software nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY 
DIRECT,INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

//
// Rules:
//
// - Directories should be searched in the following order: ~/.q3a/baseq3,
//   install dir (/usr/local/games/quake3/baseq3) and cd_path (/mnt/cdrom/baseq3).
//
// - Pak files are searched first inside the directories.
// - Case insensitive.
// - Unix-style slashes (/) (windows is backwards .. everyone knows that)
//
// Leonardo Zide (leo@lokigames.com)
//

#include "Doom3FileSystem.h"
#include "FileVisitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <glib/gdir.h>
#include <glib/gstrfuncs.h>

#include "iradiant.h"
#include "idatastream.h"
#include "ifilesystem.h"
#include "igame.h"

#include "generic/callback.h"
#include "string/string.h"
#include "os/path.h"
#include "os/dir.h"
#include "moduleobservers.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "DirectoryArchive.h"
#include "SortedFilenames.h"

Doom3FileSystem::Doom3FileSystem() :
	_numDirectories(0)
{}

void Doom3FileSystem::initDirectory(const std::string& inputPath) {
    if (_numDirectories == (VFS_MAXDIRS-1)) {
		return;
    }

    // greebo: Normalise path: Replace backslashes and ensure trailing slash
    _directories[_numDirectories] = os::standardPathWithSlash(inputPath);

    // Shortcut
	const std::string& path = _directories[_numDirectories];

	_numDirectories++;
	
	{
		ArchiveDescriptor entry;
		entry.name = path;
		entry.archive = DirectoryArchivePtr(new DirectoryArchive(path));
		entry.is_pakfile = false;
		_archives.push_back(entry);
	}
	
	// Instantiate a new sorting container for the filenames
	SortedFilenames filenameList;
	
	// Traverse the directory using the filename list as functor
    try {
        Directory_forEach(path, filenameList);
    }
    catch (DirectoryNotFoundException e) {
        std::cout << "[vfs] Directory '" << path << "' not found." 
                  << std::endl;
    }
	
	if (filenameList.size() == 0) {
		return; // nothing found
	}
	
	globalOutputStream() << "[vfs] searched directory: " << path.c_str() << "\n";
	
	// Get the ArchiveLoader and try to load each file
	ArchiveLoader& archiveModule = GlobalArchive("PK4");  
	
	// add the entries to the vfs
	for (SortedFilenames::iterator i = filenameList.begin(); i != filenameList.end(); ++i) {
		// Assemble the filename and try to load the archive
		initPakFile(archiveModule, path + *i);
	}
}

void Doom3FileSystem::initialise() {
    globalOutputStream() << "filesystem initialised\n";
    
    for (ObserverList::iterator i = _observers.begin(); i != _observers.end(); i++) {
    	(*i)->onFileSystemInitialise();
    }
}

void Doom3FileSystem::shutdown() {
	for (ObserverList::iterator i = _observers.begin(); i != _observers.end(); i++) {
    	(*i)->onFileSystemShutdown();
    }
	
	globalOutputStream() << "filesystem shutdown\n";
	
	_archives.clear();
	_numDirectories = 0;
}

void Doom3FileSystem::addObserver(Observer& observer) {
	_observers.insert(&observer);
}

void Doom3FileSystem::removeObserver(Observer& observer) {
	_observers.erase(&observer);
}

int Doom3FileSystem::getFileCount(const std::string& filename) {
	int count = 0;
	std::string fixedFilename(os::standardPathWithSlash(filename));

	for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
		if (i->archive->containsFile(fixedFilename.c_str())) {
			++count;
		}
	}

	return count;
}

ArchiveFilePtr Doom3FileSystem::openFile(const std::string& filename) {
	if (filename.find("\\") != std::string::npos) {
		globalErrorStream() << "Filename contains backslash: " << filename.c_str() << "\n";
		return ArchiveFilePtr();
	}
	
	for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
		ArchiveFilePtr file = i->archive->openFile(filename);
		if (file != NULL) {
			return file;
		}
	}
	
	// not found
	return ArchiveFilePtr();
}

ArchiveTextFilePtr Doom3FileSystem::openTextFile(const std::string& filename) {
	for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
		ArchiveTextFilePtr file = i->archive->openTextFile(filename);
		if (file != NULL) {
			return file;
		}
	}

	return ArchiveTextFilePtr();
}

std::size_t Doom3FileSystem::loadFile(const std::string& filename, void **buffer) {
	std::string fixedFilename(os::standardPathWithSlash(filename));

	ArchiveFilePtr file = openFile(fixedFilename);

	if (file != NULL) {
		// Allocate one byte more for the trailing zero
		*buffer = malloc(file->size()+1);
		
		// we need to end the buffer with a 0
		((char*) (*buffer))[file->size()] = 0;

		std::size_t length = file->getInputStream().read(
			reinterpret_cast<InputStream::byte_type*>(*buffer), 
			file->size()
		);
		
		return length;
	}

	*buffer = NULL;
	return 0;
}

void Doom3FileSystem::freeFile(void *p) {
	free(p);
}

// Call the specified callback function for each file matching extension
// inside basedir.
void Doom3FileSystem::forEachFile(const std::string& basedir, 
				const std::string& extension,
				const FileNameCallback& callback, 
				std::size_t depth)
{
	// Set of visited files, to avoid name conflicts
	std::set<std::string> visitedFiles;
	
	// Visit each Archive, applying the FileVisitor to each one (which in
	// turn calls the callback for each matching file.
	for (ArchiveList::iterator i = _archives.begin(); 
		 i != _archives.end(); 
		 ++i)
    {
		FileVisitor visitor(callback, basedir, extension, visitedFiles);
		i->archive->forEachFile(
						Archive::VisitorFunc(
								visitor, Archive::eFiles, depth), basedir);
    }
}

std::string Doom3FileSystem::findFile(const std::string& name) {
	for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
		if (!i->is_pakfile && i->archive->containsFile(name.c_str())) {
			return i->name;
		}
	}

	return "";
}

std::string Doom3FileSystem::findRoot(const std::string& name) {
	for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
		if (!i->is_pakfile && path_equal_n(name.c_str(), i->name.c_str(), i->name.size())) {
			return i->name;
		}
	}

	return "";
}

void Doom3FileSystem::initPakFile(ArchiveLoader& archiveModule, const std::string& filename) {
	std::string fileExt(os::getExtension(filename));
	boost::to_upper(fileExt);
	
	// matching extension?
	if (fileExt == archiveModule.getExtension()) {
		ArchiveDescriptor entry;
		
		entry.name = filename;
		entry.archive = archiveModule.openArchive(filename);
		entry.is_pakfile = true;
		_archives.push_back(entry);
		
		globalOutputStream() << "[vfs] pak file: " << filename.c_str() << "\n";
	}
}

// RegisterableModule implementation
const std::string& Doom3FileSystem::getName() const {
	static std::string _name(MODULE_VIRTUALFILESYSTEM);
	return _name;
}

const StringSet& Doom3FileSystem::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert("ArchivePK4");
		_dependencies.insert(MODULE_GAMEMANAGER);
	}

	return _dependencies;
}

void Doom3FileSystem::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "VFS::initialiseModule called\n";
	
	// Get the VFS search paths from the game manager
	const game::IGameManager::PathList& paths = 
		GlobalGameManager().getVFSSearchPaths();
	
	// Initialise the paths, in the given order
	for (game::IGameManager::PathList::const_iterator i = paths.begin();
		 i != paths.end(); i++)
	{
		initDirectory(*i);
	}
	
	initialise();
}
