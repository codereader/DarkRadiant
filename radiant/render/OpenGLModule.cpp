#include "OpenGLModule.h"

#include "irender.h"
#include "itextstream.h"
#include "imainframe.h"
#include "debugging/debugging.h"
#include "modulesystem/StaticModule.h"

#include <string/convert.h>
#include "wxutil/GLWidget.h"
#include "wxutil/dialog/MessageBox.h"

#include <stdexcept>
#include <FTGL/ftgl.h>

OpenGLModule::OpenGLModule() :
	_unknownError("Unknown error."),
	_wxSharedContext(NULL),
	_contextValid(false),
	_wxContextValid(false),
    _shaderProgramsAvailable(false)
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
        allErrString += string::to_string(error);
        allErrString += " (" + std::string(strErr) + ") ";
	}

    // Show the error message and terminate
	wxutil::Messagebox::ShowFatalError(allErrString);
#endif
}

void OpenGLModule::sharedContextCreated()
{
	// report OpenGL information
	rMessage() << "GL_VENDOR: "
		<< reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << std::endl;
	rMessage() << "GL_RENDERER: "
		<< reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << std::endl;
	rMessage() << "GL_VERSION: "
		<< reinterpret_cast<const char*>(glGetString(GL_VERSION)) << std::endl;
	rMessage() << "GL_EXTENSIONS: "
		<< reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) << std::endl;

	GLenum err = glewInit();

	if (err != GLEW_OK)
	{
		// glewInit failed
		rError() << "GLEW error: " <<
			reinterpret_cast<const char*>(glewGetErrorString(err));
	}

	// Initialise the font before firing the extension initialised signal
	_font.reset(new wxutil::GLFont(wxutil::GLFont::FONT_SANS, 12));

	// This call will automatically realise the render system
	GlobalRenderSystem().extensionsInitialised();
}

void OpenGLModule::sharedContextDestroyed()
{
	_font.reset();
	GlobalRenderSystem().unrealise();
}

wxGLContext& OpenGLModule::getwxGLContext()
{
	return *_wxSharedContext;
}

bool OpenGLModule::shaderProgramsAvailable() const
{
    return _shaderProgramsAvailable;
}

// Sets the flag whether shader programs are available. 
// This is set by the RenderSystem once the extensions are initialised
void OpenGLModule::setShaderProgramsAvailable(bool available)
{
    _shaderProgramsAvailable = available;
}

void OpenGLModule::registerGLCanvas(wxutil::GLWidget* widget)
{
	std::pair<wxGLWidgets::iterator, bool> result = _wxGLWidgets.insert(widget);

	if (result.second && _wxGLWidgets.size() == 1)
	{
		// First non-duplicated widget registered, take this as context holder
		_wxSharedContext = new wxGLContext(widget);

		// Create a context
		widget->SetCurrent(*_wxSharedContext);
        assertNoErrors();

		_wxContextValid = true;

		sharedContextCreated();
	}
}

void OpenGLModule::unregisterGLCanvas(wxutil::GLWidget* widget)
{
	wxGLWidgets::iterator found = _wxGLWidgets.find(widget);

	assert(found != _wxGLWidgets.end());

	if (found != _wxGLWidgets.end())
	{
		if (_wxGLWidgets.size() == 1)
		{
			// This is the last active GL widget
			_wxContextValid = false;

			sharedContextDestroyed();

			delete _wxSharedContext;
			_wxSharedContext = NULL;
		}

		_wxGLWidgets.erase(found);
	}
}

bool OpenGLModule::wxContextValid() const
{
	return _wxContextValid;
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
	rMessage() << "OpenGL::initialiseModule called.\n";
}

// Define the static OpenGLModule module
module::StaticModule<OpenGLModule> openGLModule;
