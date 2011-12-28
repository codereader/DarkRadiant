#include "OpenGLModule.h"

#include "irender.h"
#include "itextstream.h"
#include "imainframe.h"
#include "debugging/debugging.h"
#include "modulesystem/StaticModule.h"

#include "gtkutil/GLWidget.h"
#include "gtkutil/dialog/MessageBox.h"
#include <gdkmm/gl/context.h>

#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <FTGL/ftgl.h>

OpenGLModule::OpenGLModule() :
	_unknownError("Unknown error."),
	_sharedContext(NULL),
	_contextValid(false)
{}

void OpenGLModule::assertNoErrors()
{
#ifdef _DEBUG
    // Return if no error
    GLenum error = glGetError();

    if (error == GL_NO_ERROR)
    {
        return;
    }

    // Build list of all GL errors
    std::string allErrString = "GL errors encountered: ";

    for ( ; error != GL_NO_ERROR; error = glGetError())
    {
        const char* strErr = reinterpret_cast<const char*>(
            gluErrorString(error)
        );
        allErrString += boost::lexical_cast<std::string>(error);
        allErrString += " (" + std::string(strErr) + ") ";
	}

    // Show the error message and terminate
	gtkutil::MessageBox::ShowFatalError(allErrString, GlobalMainFrame().getTopLevelWindow());
#endif
}

void OpenGLModule::sharedContextCreated()
{
	// report OpenGL information
	globalOutputStream() << "GL_VENDOR: "
		<< reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << std::endl;
	globalOutputStream() << "GL_RENDERER: "
		<< reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << std::endl;
	globalOutputStream() << "GL_VERSION: "
		<< reinterpret_cast<const char*>(glGetString(GL_VERSION)) << std::endl;
	globalOutputStream() << "GL_EXTENSIONS: "
		<< reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) << std::endl;

	GLenum err = glewInit();

	if (err != GLEW_OK)
	{
		// glewInit failed
		globalErrorStream() << "GLEW error: " <<
			reinterpret_cast<const char*>(glewGetErrorString(err));
	}

	GlobalRenderSystem().extensionsInitialised();
	GlobalRenderSystem().realise();

	_font.reset(new gtkutil::GLFont(gtkutil::GLFont::FONT_SANS, 12));
}

void OpenGLModule::sharedContextDestroyed()
{
	_font.reset();
	GlobalRenderSystem().unrealise();
}

Gtk::Widget* OpenGLModule::getGLContextWidget()
{
	return _sharedContext;
}

Gtk::Widget* OpenGLModule::registerGLWidget(Gtk::Widget* widget)
{
	std::pair<GLWidgets::iterator, bool> result = _glWidgets.insert(widget);

	if (result.second && _glWidgets.size() == 1)
	{
		// First non-duplicated widget registered, take this as context
		_sharedContext = widget;
		_sharedContext->reference();

		// Create a context
		gtkutil::GLWidget::makeCurrent(*_sharedContext);
        assertNoErrors();

#ifdef DEBUG_GL_WIDGETS
        std::cout << "GLWidget: created shared context using ";

		if (Gtk::GL::widget_get_gl_context(*_sharedContext)->is_direct())
        {
            std::cout << "DIRECT rendering" << std::endl;
        }
        else
        {
            std::cout << "INDIRECT rendering" << std::endl;
        }
#endif

		_contextValid = true;

		sharedContextCreated();
	}

	return _sharedContext;
}

void OpenGLModule::unregisterGLWidget(Gtk::Widget* widget)
{
	GLWidgets::iterator found = _glWidgets.find(widget);

	assert(found != _glWidgets.end());

	if (found != _glWidgets.end())
	{
		if (_glWidgets.size() == 1)
		{
			// This was the last active GL widget
			_contextValid = false;

			sharedContextDestroyed();

			_sharedContext->unreference();
			_sharedContext = NULL;
		}

		_glWidgets.erase(found);
	}
}

bool OpenGLModule::contextValid() const
{
	return _contextValid;
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

// RegisterableModule implementation
const std::string& OpenGLModule::getName() const {
	static std::string _name(MODULE_OPENGL);
	return _name;
}

const StringSet& OpenGLModule::getDependencies() const {
	static StringSet _dependencies; // no dependencies
	return _dependencies;
}

void OpenGLModule::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "OpenGL::initialiseModule called.\n";
}

// Define the static OpenGLModule module
module::StaticModule<OpenGLModule> openGLModule;
