#include "OpenGLModule.h"

#include "irender.h"
#include "itextstream.h"
#include "imainframe.h"
#include "debugging/debugging.h"
#include "module/StaticModule.h"

#include <string/convert.h>
#include "wxutil/GLWidget.h"
#include "wxutil/dialog/MessageBox.h"

#include <stdexcept>
#include <FTGL/ftgl.h>

OpenGLModule::OpenGLModule() :
	_unknownError("Unknown error."),
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
	int maxErrors = 100;

    for ( ; error != GL_NO_ERROR; error = glGetError())
    {
        const char* strErr = reinterpret_cast<const char*>(
            gluErrorString(error)
        );
        allErrString += string::to_string(error);
        allErrString += " (" + std::string(strErr) + ") ";

		if (--maxErrors <= 0)
		{
			allErrString += "---> Maximum number of GL errors reached, maybe there is a problem with the GL context?";
			break;
		}
	}

    // Show the error message and terminate
	wxutil::Messagebox::ShowFatalError(allErrString);
#endif
}

void OpenGLModule::sharedContextCreated()
{
	// Initialise the font before firing the extension initialised signal
	_font.reset(new wxutil::GLFont(wxutil::GLFont::FONT_SANS, 12));
}

void OpenGLModule::sharedContextDestroyed()
{
	_font.reset();
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
