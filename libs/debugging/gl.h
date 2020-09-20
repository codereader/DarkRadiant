#pragma once

#ifdef _DEBUG
#include "igl.h"
#include "string/convert.h"
#endif

namespace debug
{

/// \brief Asserts that there no OpenGL errors have occurred since the last call to glGetError.
inline void assertNoGlErrors()
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

    for (; error != GL_NO_ERROR; error = glGetError())
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
    GlobalErrorHandler()("OpenGL Error", allErrString);
#endif
}

}
