#include "OpenGLModule.h"

#include "irender.h"
#include "itextstream.h"
#include "module/StaticModule.h"

#include <stdexcept>
#include <FTGL/ftgl.h>

OpenGLModule::OpenGLModule() :
	_unknownError("Unknown error.")
{}

#ifdef ENABLE_KHR_DEBUG_EXTENSION
void OpenGLModule::onGLDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, 
    const GLchar* message, const void* userParam)
{
    rError() << "OpenGL says: " << message << std::endl;
}
#endif

void OpenGLModule::sharedContextCreated()
{
	// Initialise the font before firing the extension initialised signal
	_font.reset(new gl::GLFont(IGLFont::Style::Sans, 14));

#ifdef ENABLE_KHR_DEBUG_EXTENSION
    // Debugging
    if (glewGetExtension("GL_KHR_debug"))
    {
        glDebugMessageCallback(onGLDebugMessage, this);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
#endif
}

void OpenGLModule::sharedContextDestroyed()
{
	_font.reset();
}

IGLFont::Ptr OpenGLModule::getFont(IGLFont::Style style, std::size_t size)
{
    // No caching in this first implementation
    return std::make_shared<gl::GLFont>(style, size);
}

void OpenGLModule::drawString(const std::string& string) const
{
    if (_font)
    {
        _font->drawString(string);
    }
}

void OpenGLModule::drawChar(char character) const
{
	std::string str(1,character);
	drawString(str);
}

int OpenGLModule::getFontHeight() 
{
	return _font ? _font->getLineHeight() : 0;
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
