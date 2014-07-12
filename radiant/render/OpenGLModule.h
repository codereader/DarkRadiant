#ifndef OPENGLMODULE_H_
#define OPENGLMODULE_H_

#include "igl.h"
#include <map>
#include <string>

#include "wxutil/GLFont.h"
#include "wxutil/GLWidget.h"

/// Implementation of OpenGLBinding module
class OpenGLModule :
	public OpenGLBinding
{
private:
	const std::string _unknownError;

	wxutil::GLFontPtr _font;

	wxGLContext* _wxSharedContext;

	typedef std::set<wxutil::GLWidget*> wxGLWidgets;
	wxGLWidgets _wxGLWidgets;

	bool _contextValid;
	bool _wxContextValid;

public:
	OpenGLModule();

	virtual void assertNoErrors();

	virtual void drawString(const std::string& string) const;
	virtual void drawChar(char character) const;
	virtual int getFontHeight();

	virtual wxGLContext& getwxGLContext();
    virtual void registerGLCanvas(wxutil::GLWidget* widget);
    virtual void unregisterGLCanvas(wxutil::GLWidget* widget);
	virtual bool wxContextValid() const;

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

private:

	void sharedContextCreated();
	void sharedContextDestroyed();
};

#endif /*OPENGLMODULE_H_*/
