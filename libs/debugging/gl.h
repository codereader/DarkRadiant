#pragma once

#include "igl.h"
#include "string/convert.h"

namespace debug
{

/**
 * \brief Check and print GL errors
 *
 * Reads GL errors and dumps them to the internal console, returning the most
 * recent error to calling code.
 *
 * \param context
 * Optional caller-defined context string which will be printed along with any
 * errors. This can help identify what code was doing at the point where GL
 * errors were detected.
 *
 * \return The last GL error, or 0 if there is no error
 */
inline GLenum checkGLErrors(const std::string& context = {})
{
    // Return if no error
    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
    {
        return GL_NO_ERROR;
    }

    // Build list of all GL errors
    std::string allErrString;
    int maxErrors = 10;
    GLenum lastError = error;
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

        lastError = error;
    }

    // Show the error message return last error
    rError() << "OpenGL Error(s)"
             << (context.empty() ? "" : " [" + context + "]") << ":\n"
             << allErrString << "\n";
    return lastError;
}

/// Check GL errors only in debug mode
inline void assertNoGlErrors(const std::string& context = {})
{
#if !defined(NDEBUG)
    checkGLErrors(context);
#endif
}

}
