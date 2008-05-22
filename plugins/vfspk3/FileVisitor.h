#ifndef FILEVISITOR_H_
#define FILEVISITOR_H_

#include "ifilesystem.h"
#include "iarchive.h"

#include "os/path.h"
#include "generic/callback.h"

#include <set>
#include <boost/algorithm/string/case_conv.hpp>

class FileVisitor 
: public Archive::Visitor
{
	// The FileNameCallback to call for each located file
	const FileNameCallback& _callback;
	
	// Set of already-visited files
	std::set<std::string>& _visitedFiles;
	
	// Directory to search within
	std::string _directory;
	
	// Extension to match
	std::string _extension;

public:
	
	// Constructor
	FileVisitor(const FileNameCallback& cb, 
				const std::string& dir, 
				const std::string& ext,
				std::set<std::string>& visitedFiles)
    : _callback(cb), 
      _visitedFiles(visitedFiles),
      _directory(dir), 
      _extension(ext)
    {}
	
	// Required visit function 
	void visit(const std::string& name)
	{
		std::string subname = os::getRelativePath(name, _directory);
	    if(subname != name)
	    {
	    	if (_extension == "*" || 
				extensionMatches(os::getExtension(subname)) && 
				_visitedFiles.find(subname) == _visitedFiles.end())
	    	{
	    		// Matching file, call the callback and add to visited file set
	    		_callback(subname);
	    		_visitedFiles.insert(subname);
	    	}
	    }
	}

	// Returns true if the extension matches the required one
	// treats extensions case-insensitively in Windows
	bool extensionMatches(const std::string& ext) const {
#ifdef WIN32
		return boost::to_lower_copy(ext) == _extension;
#else
		return ext == _extension;
#endif
	}
};
    

#endif /*FILEVISITOR_H_*/
