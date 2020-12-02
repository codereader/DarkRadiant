#pragma once

#include "igl.h"
#include <map>
#include <string>
#include <sigc++/connection.h>

#include "GLFont.h"
#include "wxutil/GLWidget.h"

#ifndef NDEBUG
//#define ENABLE_KHR_DEBUG_EXTENSION
#endif

/// Implementation of OpenGLBinding module
class OpenGLModule :
	public OpenGLBinding
{
private:
	const std::string _unknownError;

	IGLFont::Ptr _font;

	sigc::connection _contextCreated;
	sigc::connection _contextDestroyed;

    // Map the font+size combo to weak pointers
    std::map<std::pair<IGLFont::Style, int>, std::weak_ptr<IGLFont>> _fontCache;

public:
	OpenGLModule();

    IGLFont::Ptr getFont(IGLFont::Style style, std::size_t size) override;

	void drawString(const std::string& string) const override;
	int getFontHeight() override;

	// RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
	void sharedContextCreated();
	void sharedContextDestroyed();

#ifdef ENABLE_KHR_DEBUG_EXTENSION
    static void onGLDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, 
        GLsizei length, const GLchar* message, const void* userParam);
#endif
};
