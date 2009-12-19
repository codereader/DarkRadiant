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

	// The (singleton) widget holding the context
	GtkWidget* _sharedContext;
	
	// Holds the number of realised GL widgets
	std::size_t _realisedGLWidgets;
	
public:
	OpenGLModule();
	
	virtual void assertNoErrors();
	
	virtual void drawString(const std::string& string) const;
	virtual void drawChar(char character) const;

	// GtkGLext context management
	virtual GtkWidget* getGLContextWidget();
	virtual GtkWidget* registerGLWidget(GtkWidget* widget);
	virtual void unregisterGLWidget(GtkWidget* widget);
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

private:
	const std::string& getGLErrorString(GLenum errorCode) const;

	void sharedContextCreated();
	void sharedContextDestroyed();
};

#endif /*OPENGLMODULE_H_*/
