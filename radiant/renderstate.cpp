/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "render/backend/OpenGLStateBucket.h"
#include "render/backend/OpenGLStateBucketAdd.h"
#include "render/backend/OpenGLShader.h"
#include "render/backend/OpenGLStateMap.h"
#include "render/backend/GLProgramFactory.h"

#include "debugging/debugging.h"
#include "warnings.h"

#include "imodule.h"
#include "ishaders.h"
#include "irender.h"
#include "igl.h"
#include "iglrender.h"
#include "renderable.h"
#include "iradiant.h"

#include <set>
#include <vector>
#include <list>
#include <map>
#include <sstream>

#include "math/matrix.h"
#include "math/aabb.h"
#include "generic/callback.h"
#include "texturelib.h"
#include "string/string.h"
#include "container/array.h"
#include "generic/reference.h"
#include "moduleobservers.h"
#include "stream/filestream.h"
#include "stream/stringstream.h"
#include "os/file.h"
#include "modulesystem/StaticModule.h"

#include "render/LinearLightList.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/weak_ptr.hpp>

/*void printShaderLog(GLhandleARB object)
{
  GLint log_length = 0;
  glGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &log_length);
  
  Array<char> log(log_length);
  glGetInfoLogARB(object, log_length, &log_length, log.data());
  
  globalErrorStream() << StringRange(log.begin(), log.begin() + log_length) << "\n";
}*/

/*void createShader(GLhandleARB program, const char* filename, GLenum type)
{
  GLhandleARB shader = glCreateShaderObjectARB(type);
  GlobalOpenGL_debugAssertNoErrors();

  // load shader
  {
    std::size_t size = file_size(filename);
    FileInputStream file(filename);
    ASSERT_MESSAGE(!file.failed(), "failed to open " << makeQuoted(filename));
    Array<GLcharARB> buffer(size);
    size = file.read(reinterpret_cast<StreamBase::byte_type*>(buffer.data()), size);

    const GLcharARB* string = buffer.data();
    GLint length = GLint(size);
    glShaderSourceARB(shader, 1, &string, &length);
  }

  // compile shader
  {
    glCompileShaderARB(shader);
    
    GLint compiled = 0;
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);

    if(!compiled)
    {
      printShaderLog(shader);
    }

    ASSERT_MESSAGE(compiled, "shader compile failed: " << makeQuoted(filename));
  }
  
  // attach shader
  glAttachObjectARB(program, shader);
  
  glDeleteObjectARB(shader);

  GlobalOpenGL_debugAssertNoErrors();
}*/

#define LIGHT_SHADER_DEBUG 0

#define DEBUG_SHADERS 0

namespace render {

// Define the static OpenGLStateLibrary registerable module
module::StaticModule<OpenGLStateMap> openGLStateLibraryModule;

} // namespace render
