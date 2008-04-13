#ifndef OPENGLMODULE_H_
#define OPENGLMODULE_H_

#include "igl.h"
#include <map>
#include <string>
#include "gtkutil/glfont.h"

class OpenGLModule :
	public OpenGLBinding
{
	typedef std::map<GLenum, std::string> GLErrorList;
	GLErrorList _errorList;
	
	const std::string _unknownError;
		
	GLFont _font;
	
public:
	OpenGLModule();
	
	virtual void assertNoErrors();
	
	virtual void sharedContextCreated();
	virtual void sharedContextDestroyed();
	
	virtual void drawString(const std::string& string) const;
	virtual void drawChar(char character) const;
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

private:
	const std::string& getGLErrorString(GLenum errorCode) const;
};

#endif /*OPENGLMODULE_H_*/
