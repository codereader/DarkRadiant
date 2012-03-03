#ifndef OPENGLMODULE_H_
#define OPENGLMODULE_H_

#include "igl.h"
#include <map>
#include <string>

#include "gtkutil/GLFont.h"
#include "gtkutil/GLWidget.h"

/// Implementation of OpenGLBinding module
class OpenGLModule :
	public OpenGLBinding
{
private:
	const std::string _unknownError;

	gtkutil::GLFontPtr _font;

	// The (singleton) widget holding the context
    gtkutil::GLWidget* _sharedContextWidget;

    // All widgets
	typedef std::set<gtkutil::GLWidget*> GLWidgets;
	GLWidgets _glWidgets;

	bool _contextValid;

public:
	OpenGLModule();

	virtual void assertNoErrors();

	virtual void drawString(const std::string& string) const;
	virtual void drawChar(char character) const;
	virtual int getFontHeight();

	// GtkGLext context management
	virtual gtkutil::GLWidget* getGLContextWidget();
	virtual void registerGLWidget(gtkutil::GLWidget* widget);
	virtual void unregisterGLWidget(gtkutil::GLWidget* widget);
	virtual bool contextValid() const;

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

private:

	void sharedContextCreated();
	void sharedContextDestroyed();
};

#endif /*OPENGLMODULE_H_*/
