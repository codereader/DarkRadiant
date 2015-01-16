#pragma once

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

    bool _shaderProgramsAvailable;

public:
	OpenGLModule();

	virtual void assertNoErrors() override;

	virtual void drawString(const std::string& string) const override;
	virtual void drawChar(char character) const override;
	virtual int getFontHeight() override;

    virtual wxGLContext& getwxGLContext() override;
    virtual void registerGLCanvas(wxutil::GLWidget* widget) override;
    virtual void unregisterGLCanvas(wxutil::GLWidget* widget) override;
	virtual bool wxContextValid() const override;

    virtual bool shaderProgramsAvailable() const override;
    virtual void setShaderProgramsAvailable(bool available) override;

	// RegisterableModule implementation
    virtual const std::string& getName() const override;
    virtual const StringSet& getDependencies() const override;
    virtual void initialiseModule(const ApplicationContext& ctx) override;

private:
	void sharedContextCreated();
	void sharedContextDestroyed();
};
