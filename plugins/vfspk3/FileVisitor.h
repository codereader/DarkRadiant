#ifndef FILEVISITOR_H_
#define FILEVISITOR_H_

#include "ifilesystem.h"
#include "iarchive.h"

#include "os/path.h"
#include "generic/callback.h"

#include <iostream>

class FileVisitor 
: public Archive::Visitor
{
	// The FileNameCallback to call for each located file
	const FileNameCallback& _callback;
	
	const char* m_directory;
  const char* m_extension;
public:
	
	/* Constructor */
	FileVisitor(const FileNameCallback& cb, const char* dir, const char* ext)
    : _callback(cb), m_directory(dir), m_extension(ext)
    {}
	
	/* Required visit function */
	void visit(const std::string& name)
	{
		const char* subname = path_make_relative(name.c_str(), m_directory);
	    if(subname != name.c_str())
	    {
	      if(subname[0] == '/')
	        ++subname;
	      if (m_extension[0] == '*' 
	    	  || extension_equal(path_get_extension(subname), m_extension))
	      {
	    	  // Matching file, call the callback
	    	  _callback(subname);
	      }
	    }
	}
};
    

#endif /*FILEVISITOR_H_*/
