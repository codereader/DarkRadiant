#include "GLProgramFactory.h"
#include "glprogram/ARBBumpProgram.h"
#include "glprogram/ARBDepthFillProgram.h"

#include "os/file.h"
#include "debugging/debugging.h"
#include "container/array.h"
#include "stream/filestream.h"

namespace render
{

// Constructor, populates map with GLProgram instances
GLProgramFactory::GLProgramFactory()
{
	_map.insert(std::make_pair("depthFill", new ARBDepthFillProgram())); 
	_map.insert(std::make_pair("bumpMap", new ARBBumpProgram()));
}

// Return static GLProgramFactory instance
GLProgramFactory& GLProgramFactory::getInstance() {
	static GLProgramFactory _instance;
	return _instance;
}

// Lookup a named program in the singleton instance
GLProgramPtr GLProgramFactory::getProgram(const std::string& name) {
	
	// Reference to static instance's map
	ProgramMap& map = getInstance()._map;
	
	// Lookup the program, if not found throw an exception
	ProgramMap::iterator i = map.find(name);
	if (i != map.end())
		return i->second;
	else
		throw std::runtime_error("GLProgramFactory: failed to find program "
								 + name);
}

// Realise the program factory.
void GLProgramFactory::realise() {
	
	// Get static map
	ProgramMap& map = getInstance()._map;
	
	// Realise each GLProgram in the map
	for (ProgramMap::iterator i = map.begin();
		 i != map.end();
		 ++i)
	{
		i->second->create();
	}
}

// Unrealise the program factory.
void GLProgramFactory::unrealise() {
	
	// Get static map
	ProgramMap& map = getInstance()._map;
	
	// Destroy each GLProgram in the map
	for (ProgramMap::iterator i = map.begin();
		 i != map.end();
		 ++i)
	{
		i->second->destroy();
	}
}

void GLProgramFactory::createARBProgram(const std::string& filename, GLenum type) {
	std::size_t size = file_size(filename.c_str());
	FileInputStream file(filename);
	
    // Throw an exception if the file could not be found
	if (file.failed())
    {
        throw std::runtime_error(
            "GLProgramFactory::createARBProgram() failed to open file: "
            + filename
        );
    }
	
	Array<GLcharARB> buffer(size);
	size = file.read(reinterpret_cast<StreamBase::byte_type*>(buffer.data()), size);

	glProgramStringARB(type, GL_PROGRAM_FORMAT_ASCII_ARB, GLsizei(size), buffer.data());

	if (GL_INVALID_OPERATION == glGetError()) {
		GLint errPos;
		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
		const GLubyte* errString = glGetString(GL_PROGRAM_ERROR_STRING_ARB);

		globalErrorStream() << reinterpret_cast<const char*>(filename.c_str()) << ":"
			<< errPos << "\n"<< reinterpret_cast<const char*>(errString);

		ERROR_MESSAGE("error in gl program");
	}
}

// Get the path of a GL program file
std::string GLProgramFactory::getGLProgramPath(const std::string& progName)
{
    // Determine the root path of the GL programs
#if defined(POSIX) && defined (PKGDATADIR)
    std::string glProgRoot = std::string(PKGDATADIR) + "/";
#else
    std::string glProgRoot = GlobalRegistry().get("user/paths/appPath");
#endif

    // Append the requested filename with the "gl/" directory.
    return glProgRoot + "gl/" + progName;
}

} // namespace render
