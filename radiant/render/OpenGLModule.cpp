#include "OpenGLModule.h"

#include "irender.h"
#include "itextstream.h"
#include "module/StaticModule.h"

#include <stdexcept>
#include <FTGL/ftgl.h>

OpenGLModule::OpenGLModule() :
	_unknownError("Unknown error.")
{}

void OpenGLModule::sharedContextCreated()
{
	// Initialise the font before firing the extension initialised signal
	_font.reset(new wxutil::GLFont(wxutil::GLFont::FONT_SANS, 12));
}

void OpenGLModule::sharedContextDestroyed()
{
	_font.reset();
}

void OpenGLModule::drawString(const std::string& string) const
{
	ftglRenderFont(_font->getFtglFont(),string.c_str(),0xFFFF);//FTGL_RENDER_ALL);
}

void OpenGLModule::drawChar(char character) const
{
	std::string str(1,character);
	drawString(str);
}

int OpenGLModule::getFontHeight() 
{
	return _font->getPixelHeight();
}

const std::string& OpenGLModule::getName() const
{
	static std::string _name(MODULE_OPENGL);
	return _name;
}

const StringSet& OpenGLModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_SHARED_GL_CONTEXT);
	}

	return _dependencies;
}

void OpenGLModule::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	_contextCreated = GlobalOpenGLContext().signal_sharedContextCreated().connect(
		sigc::mem_fun(this, &OpenGLModule::sharedContextCreated));

	_contextDestroyed = GlobalOpenGLContext().signal_sharedContextDestroyed().connect(
		sigc::mem_fun(this, &OpenGLModule::sharedContextDestroyed));
}

void OpenGLModule::shutdownModule()
{
	_contextCreated.disconnect();
	_contextDestroyed.disconnect();
}

// Define the static OpenGLModule module
module::StaticModule<OpenGLModule> openGLModule;
