#include "OpenGLModule.h"

#include "irender.h"
#include "itextstream.h"
#include "debugging/debugging.h"
#include "modulesystem/StaticModule.h"

#include "gtkutil/GLWidget.h"
#include <gtk/gtkwidget.h>
#include <gtk/gtkglwidget.h>

#include <boost/lexical_cast.hpp>
#include <stdexcept>

OpenGLModule::OpenGLModule() :
	_unknownError("Unknown error."),
	_font(0, 0),
	_sharedContext(NULL),
	_realisedGLWidgets(0)
{ }

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
        allErrString += "(" + std::string(strErr) + ") ";
	}

    // This is a logic_error because we should handle GL errors correctly.
    throw std::logic_error(allErrString);
#endif
}

void OpenGLModule::sharedContextCreated()
{
	// report OpenGL information
	globalOutputStream() << "GL_VENDOR: "
		<< reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << "\n";
	globalOutputStream() << "GL_RENDERER: "
		<< reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << "\n";
	globalOutputStream() << "GL_VERSION: "
		<< reinterpret_cast<const char*>(glGetString(GL_VERSION)) << "\n";
	globalOutputStream() << "GL_EXTENSIONS: "
		<< reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) << "\n";

	GLenum err = glewInit();
	if (err != GLEW_OK)	{
		// glewInit failed
		globalErrorStream() << "GLEW error: " << 
			reinterpret_cast<const char*>(glewGetErrorString(err));
	}

	GlobalRenderSystem().extensionsInitialised();
	GlobalRenderSystem().realise();

	_font = glfont_create("Sans 8");
	m_font = _font.getDisplayList();
	m_fontHeight = _font.getPixelHeight();
}
	
void OpenGLModule::sharedContextDestroyed()
{
	GlobalRenderSystem().unrealise();
}

GtkWidget* OpenGLModule::getGLContextWidget()
{
	return _sharedContext;
}

GtkWidget* OpenGLModule::registerGLWidget(GtkWidget* widget)
{
	if (++_realisedGLWidgets == 1)
	{
		_sharedContext = widget;
		gtk_widget_ref(_sharedContext);

		// Create a context
		gtkutil::GLWidget::makeCurrent(_sharedContext);
        assertNoErrors();

#ifdef DEBUG_GL_WIDGETS
        std::cout << "GLWidget: created shared context using ";

        if (gdk_gl_context_is_direct(
                gtk_widget_get_gl_context(_sharedContext)
            ) == TRUE)
        {
            std::cout << "DIRECT rendering" << std::endl;
        }
        else
        {
            std::cout << "INDIRECT rendering" << std::endl;
        }
#endif

		contextValid = true;

		sharedContextCreated();
	}

	return _sharedContext;
}

void OpenGLModule::unregisterGLWidget(GtkWidget* widget)
{
	assert(_realisedGLWidgets > 0);

	if (--_realisedGLWidgets == 0)
	{
		// This was the last active GL widget
		contextValid = false;

		sharedContextDestroyed();

		gtk_widget_unref(_sharedContext);
		_sharedContext = NULL;
	}
}

void OpenGLModule::drawString(const std::string& string) const {
	glListBase(m_font);
	glCallLists(GLsizei(string.size()), GL_UNSIGNED_BYTE, reinterpret_cast<const GLubyte*>(string.c_str()));
}

void OpenGLModule::drawChar(char character) const {
	glListBase(m_font);
	glCallLists(1, GL_UNSIGNED_BYTE, reinterpret_cast<const GLubyte*>(&character));
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
