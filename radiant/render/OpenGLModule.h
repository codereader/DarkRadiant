#pragma once

#include "igl.h"
#include <map>
#include <string>
#include <sigc++/connection.h>

#include "wxutil/GLFont.h"
#include "wxutil/GLWidget.h"

/// Implementation of OpenGLBinding module
class OpenGLModule :
	public OpenGLBinding
{
private:
	const std::string _unknownError;

	wxutil::GLFontPtr _font;

	sigc::connection _contextCreated;
	sigc::connection _contextDestroyed;

public:
	OpenGLModule();

	void drawString(const std::string& string) const override;
	void drawChar(char character) const override;
	int getFontHeight() override;

	// RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
	void sharedContextCreated();
	void sharedContextDestroyed();
};
