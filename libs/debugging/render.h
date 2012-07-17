#pragma once

#include <ostream>

#include "render/Colour4.h"

namespace debug
{
    namespace detail
    {
        inline bool boolFromGLBool(GLboolean b)
        {
            return b == GL_TRUE;
        }
    }

    /// Streamable object to insert render state flags
    struct StateFlagsInserter
    {
        int flags;
        StateFlagsInserter(int f): flags(f) { }
    };

    inline std::ostream& operator<<(std::ostream& os,
                                    const StateFlagsInserter& s)
    {
        #define OUTPUT_RENDERFLAG(x) if (s.flags & (x)) { os << "|" << #x; }
        OUTPUT_RENDERFLAG(RENDER_LINESTIPPLE);
        OUTPUT_RENDERFLAG(RENDER_POLYGONSTIPPLE);
        OUTPUT_RENDERFLAG(RENDER_ALPHATEST);
        OUTPUT_RENDERFLAG(RENDER_DEPTHTEST);
        OUTPUT_RENDERFLAG(RENDER_DEPTHWRITE);
        OUTPUT_RENDERFLAG(RENDER_MASKCOLOUR);
        OUTPUT_RENDERFLAG(RENDER_CULLFACE);
        OUTPUT_RENDERFLAG(RENDER_SCALED);
        OUTPUT_RENDERFLAG(RENDER_SMOOTH);
        OUTPUT_RENDERFLAG(RENDER_LIGHTING);
        OUTPUT_RENDERFLAG(RENDER_BLEND);
        OUTPUT_RENDERFLAG(RENDER_OFFSETLINE);
        OUTPUT_RENDERFLAG(RENDER_FILL);
        OUTPUT_RENDERFLAG(RENDER_VERTEX_COLOUR);
        OUTPUT_RENDERFLAG(RENDER_TEXTURE_2D);
        OUTPUT_RENDERFLAG(RENDER_TEXTURE_CUBEMAP);
        OUTPUT_RENDERFLAG(RENDER_BUMP);
        OUTPUT_RENDERFLAG(RENDER_PROGRAM);
        OUTPUT_RENDERFLAG(RENDER_OVERRIDE);
        return os;
    }

    /// Streamable object to insert glColorMask value
    class ColorMaskInserter { };

    inline std::ostream& operator<<(std::ostream& os,
                                    const ColorMaskInserter& i)
    {
        using namespace detail;

        GLboolean vals[4];
        glGetBooleanv(GL_COLOR_WRITEMASK, &vals[0]);

        os << "{ R = " << boolFromGLBool(vals[0]) 
           << ", G = " << boolFromGLBool(vals[1])
           << ", B = " << boolFromGLBool(vals[2]) 
           << ", A = " << boolFromGLBool(vals[3]) << " }";
        return os;
    }

    /// Streamable object to insert glDepthMask value
    class DepthMaskInserter { };

    inline std::ostream& operator<<(std::ostream& os,
                                    const DepthMaskInserter& i)
    {
        GLboolean mask;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);

        os << detail::boolFromGLBool(mask);
        return os;
    }

    /// Get the current GL_COLOR as a Colour4 for debugging
    inline Colour4 getGLColor()
    {
        Colour4 result;
        glGetDoublev(GL_CURRENT_COLOR, &result[0]);
        return result;
    }

    /// Get a GL integer for debugging
    inline int getGLInt(GLenum name)
    {
        int result;
        glGetIntegerv(name, &result);
        return result;
    }

    /// Get a GL boolean for debugging
    inline bool getGLBool(GLenum name)
    {
        GLboolean result;
        glGetBooleanv(name, &result);
        return result == GL_TRUE;
    }
}
