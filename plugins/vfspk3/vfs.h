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

#if !defined(INCLUDED_VFS_H)
#define INCLUDED_VFS_H

#include "ifilesystem.h"
#include "moduleobservers.h"

class Quake3FileSystem : 
	public VirtualFileSystem
{
	ModuleObservers _moduleObservers;
public:
	// Constructor
	Quake3FileSystem();
	
	void initDirectory(const std::string& path);
	void initialise();
	void shutdown();

	int getFileCount(const char *filename, int flags);
	ArchiveFile* openFile(const char* filename);
	ArchiveTextFile* openTextFile(const std::string& filename);
	std::size_t loadFile(const char *filename, void **buffer);
	void freeFile(void *p);

	// Call the specified callback function for each file matching extension
	// inside basedir.
	void forEachFile(const char* basedir, const char* extension,
		const FileNameCallback& callback, std::size_t depth);

	const char* findFile(const char *name);
	const char* findRoot(const char *name);

	void attach(ModuleObserver& observer);
	void detach(ModuleObserver& observer);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<Quake3FileSystem> Quake3FileSystemPtr;

#endif
