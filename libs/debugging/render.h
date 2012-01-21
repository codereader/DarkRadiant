#pragma once

#include <ostream>

#include "render/Colour4.h"

namespace debug
{
    namespace detail
    {
        // Stream insertion helper class
        struct StateFlagsInserter
        {
            int flags;
            StateFlagsInserter(int f): flags(f) { }
        };

        // Stream insertion function
        inline std::ostream& operator<<(std::ostream& os,
                                        const StateFlagsInserter& s)
        {
            #define OUTPUT_RENDERFLAG(x) if (s.flags & (x)) { os << "|" << #x; }
            OUTPUT_RENDERFLAG(RENDER_LINESTIPPLE);
            OUTPUT_RENDERFLAG(RENDER_LINESMOOTH);
            OUTPUT_RENDERFLAG(RENDER_POLYGONSTIPPLE);
            OUTPUT_RENDERFLAG(RENDER_POLYGONSMOOTH);
            OUTPUT_RENDERFLAG(RENDER_ALPHATEST);
            OUTPUT_RENDERFLAG(RENDER_DEPTHTEST);
            OUTPUT_RENDERFLAG(RENDER_DEPTHWRITE);
            OUTPUT_RENDERFLAG(RENDER_COLOURWRITE);
            OUTPUT_RENDERFLAG(RENDER_CULLFACE);
            OUTPUT_RENDERFLAG(RENDER_SCALED);
            OUTPUT_RENDERFLAG(RENDER_SMOOTH);
            OUTPUT_RENDERFLAG(RENDER_LIGHTING);
            OUTPUT_RENDERFLAG(RENDER_BLEND);
            OUTPUT_RENDERFLAG(RENDER_OFFSETLINE);
            OUTPUT_RENDERFLAG(RENDER_FILL);
            OUTPUT_RENDERFLAG(RENDER_COLOURARRAY);
            OUTPUT_RENDERFLAG(RENDER_VERTEX_COLOUR);
            OUTPUT_RENDERFLAG(RENDER_TEXTURE_2D);
            OUTPUT_RENDERFLAG(RENDER_TEXTURE_CUBEMAP);
            OUTPUT_RENDERFLAG(RENDER_BUMP);
            OUTPUT_RENDERFLAG(RENDER_PROGRAM);
            OUTPUT_RENDERFLAG(RENDER_OVERRIDE);
            return os;
        }
    }

    /// Convert a renderstateflags int to a streamable object for debugging
    inline detail::StateFlagsInserter printStateFlags(int flags)
    {
        return detail::StateFlagsInserter(flags);
    }

    /// Get the current GL_COLOR as a Colour4 for debugging
    inline Colour4 getGLColor()
    {
        Colour4 result;
        glGetFloatv(GL_CURRENT_COLOR, &result[0]);
        return result;
    }
}
