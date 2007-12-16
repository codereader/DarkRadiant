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

#include "renderstate.h"
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

#define DEBUG_RENDER 0

#include "timer.h"

StringOutputStream g_renderer_stats;
std::size_t g_count_prims;
std::size_t g_count_states;
std::size_t g_count_transforms;
Timer g_timer;

inline void count_prim()
{
  ++g_count_prims;
}

inline void count_state()
{
  ++g_count_states;
}

inline void count_transform()
{
  ++g_count_transforms;
}

void Renderer_ResetStats()
{
  g_count_prims = 0;
  g_count_states = 0;
  g_count_transforms = 0;
  g_timer.start();
}

const char* Renderer_GetStats()
{
  g_renderer_stats.clear();
  g_renderer_stats << "prims: " << Unsigned(g_count_prims)
    << " | states: " << Unsigned(g_count_states)
    << " | transforms: " << Unsigned(g_count_transforms)
    << " | msec: " << g_timer.elapsed_msec();
  return g_renderer_stats.c_str();
}


void printShaderLog(GLhandleARB object)
{
  GLint log_length = 0;
  glGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &log_length);
  
  Array<char> log(log_length);
  glGetInfoLogARB(object, log_length, &log_length, log.data());
  
  globalErrorStream() << StringRange(log.begin(), log.begin() + log_length) << "\n";
}

void createShader(GLhandleARB program, const char* filename, GLenum type)
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
}

// Create an ARB GL Program by calling glProgramStringARB with the contents of
// a file.
void createARBProgram(const char* filename, GLenum type)
{
  std::size_t size = file_size(filename);
  FileInputStream file(filename);
  ASSERT_MESSAGE(!file.failed(), "failed to open " << makeQuoted(filename));
  Array<GLcharARB> buffer(size);
  size = file.read(reinterpret_cast<StreamBase::byte_type*>(buffer.data()), size);

  glProgramStringARB(type, GL_PROGRAM_FORMAT_ASCII_ARB, GLsizei(size), buffer.data());

  if(GL_INVALID_OPERATION == glGetError())
  {
    GLint errPos;
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
    const GLubyte* errString = glGetString(GL_PROGRAM_ERROR_STRING_ARB);

    globalErrorStream() << reinterpret_cast<const char*>(filename) << ":" <<  errPos << "\n" << reinterpret_cast<const char*>(errString);

    ERROR_MESSAGE("error in gl program");
  }
}

#define LIGHT_SHADER_DEBUG 0

#define DEBUG_SHADERS 0

namespace render {

// Define the static OpenGLStateLibrary registerable module
module::StaticModule<OpenGLStateMap> openGLStateLibraryModule;

} // namespace render
