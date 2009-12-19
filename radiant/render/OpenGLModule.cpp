#include "OpenGLModule.h"

#include "irender.h"
#include "itextstream.h"
#include "debugging/debugging.h"
#include "modulesystem/StaticModule.h"

#include "gtkutil/GLWidget.h"
#include <gtk/gtkwidget.h>
#include <gtk/gtkglwidget.h>

OpenGLModule::OpenGLModule() :
	_unknownError("Unknown error."),
	_font(0, 0),
	_sharedContext(NULL),
	_realisedGLWidgets(0)
{
	// Populate the error list
	_errorList[GL_NO_ERROR] = "GL_NO_ERROR - no error";
	_errorList[GL_INVALID_ENUM] = "GL_INVALID_ENUM - An unacceptable value is specified for an enumerated argument.";
	_errorList[GL_INVALID_VALUE] = "GL_INVALID_VALUE - A numeric argument is out of range.";
	_errorList[GL_INVALID_OPERATION] = "GL_INVALID_OPERATION - The specified operation is not allowed in the current state.";
	_errorList[GL_STACK_OVERFLOW] = "GL_STACK_OVERFLOW - Function would cause a stack overflow.";
	_errorList[GL_STACK_UNDERFLOW] = "GL_STACK_UNDERFLOW - Function would cause a stack underflow.";
	_errorList[GL_OUT_OF_MEMORY] = "GL_OUT_OF_MEMORY - There is not enough memory left to execute the function.";
}

void OpenGLModule::assertNoErrors()
{
#ifdef _DEBUG
	GLenum error = glGetError();
	while (error != GL_NO_ERROR) {
		const std::string& errorString = getGLErrorString(error);
		
		if (error == GL_OUT_OF_MEMORY) {
			ERROR_MESSAGE("OpenGL out of memory error: " + errorString);
		}
		else {
			ERROR_MESSAGE("OpenGL error: " + errorString);
		}

		error = glGetError();
	}
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

const std::string& OpenGLModule::getGLErrorString(GLenum errorCode) const {
	GLErrorList::const_iterator found = _errorList.find(errorCode);
	
	if (found != _errorList.end()) {
		return found->second;
	}
	
	// Not found
	return _unknownError;
}

// Define the static OpenGLModule module
module::StaticModule<OpenGLModule> openGLModule;
