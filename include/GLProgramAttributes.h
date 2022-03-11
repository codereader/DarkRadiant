#pragma once

namespace render
{

/**
 * Enumeration for vertex attributes to be bound to a GLProgram, to avoid using
 * magic numbers.
 */
struct GLProgramAttribute
{
    enum Index
    {
        Position = 0,
        TexCoord = 8,
        Tangent = 9,
        Bitangent = 10,
        Normal = 11,
        Colour = 12,
    };
};

}
