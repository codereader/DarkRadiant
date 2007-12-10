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

#if !defined (INCLUDED_JPEG_H)
#define INCLUDED_JPEG_H

#include "ifilesystem.h"
#include "iimage.h"
#include "imagelib.h" // for RGBAImagePtr
#include <iostream>

ImagePtr LoadJPG(ArchiveFile& file);

/* greebo: A JPGLoader is capable of loading JPEG files.
 *  
 * Use load() to actually retrieve an Image* object with the loaded image.
 * 
 * Shouldn't be used to load textures directly, use the 
 * GlobalShaderSystem() module instead.  
 * 
 * Complies with the ImageLoader interface defined in "iimage.h" 
 */
class JPGLoader :
	public ImageLoader
{
public:
	/* greebo: This loads the file and returns the pointer to 
	 * the allocated Image object (or NULL, if the load failed). 
	 */
	ImagePtr load(ArchiveFile& file) const {
		// Pass the call to the according load function
		return LoadJPG(file);
	}
	
	/* greebo: Gets the file extension of the supported image file type (e.g. "jpg") 
	 */
	std::string getExtension() const {
		return "jpg";
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ImageLoaderJPG");
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
  		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "ImageLoaderJPG::initialiseModule called.\n";
	}
};

#endif

