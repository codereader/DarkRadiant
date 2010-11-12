#ifndef OPENGLMODULE_H_
#define OPENGLMODULE_H_

#include "igl.h"
#include <map>
#include <string>
#include "gtkutil/glfont.h"

class OpenGLModule :
	public OpenGLBinding
{
private:
	const std::string _unknownError;

	GLFont _font;

	// The (singleton) widget holding the context
	Gtk::Widget* _sharedContext;

	typedef std::set<Gtk::Widget*> GLWidgets;
	GLWidgets _glWidgets;

public:
	OpenGLModule();

	virtual void assertNoErrors();

	virtual void drawString(const std::string& string) const;
	virtual void drawChar(char character) const;

	// GtkGLext context management
	virtual Gtk::Widget* getGLContextWidget();
	virtual Gtk::Widget* registerGLWidget(Gtk::Widget* widget);
	virtual void unregisterGLWidget(Gtk::Widget* widget);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

private:

	void sharedContextCreated();
	void sharedContextDestroyed();
};

#endif /*OPENGLMODULE_H_*/
