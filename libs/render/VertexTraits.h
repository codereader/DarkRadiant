#pragma once

namespace render
{

/**
 * \brief
 * Traits class for vertex data
 *
 * Specialisations of this class provide the VBO-related code with a mechanism
 * to determine what vertex properties a particular class provides (e.g.
 * normal, colour, texcoord etc), and to obtain the byte offsets of each
 * property within the class.
 */
template<typename V> class VertexTraits
{
public:

    /// Typedef of the underlying vertex type
    typedef V VertexType;
};

}
