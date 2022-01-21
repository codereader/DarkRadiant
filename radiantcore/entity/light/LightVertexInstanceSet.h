#pragma once

#include "iselection.h"
#include "../VertexInstance.h"

namespace entity
{

// Set of values defining a projected light
template<typename T> struct Projected
{
    T target;
    T up;
    T right;
    T start;
    T end;
};

// The set of editable vertices of a single light entity
struct LightVertexInstanceSet
{
    // The (draggable) light center instance
    VertexInstance center;

    // Projected light vertices
    VertexInstance target;
    VertexInstanceRelative right;
    VertexInstanceRelative up;
    VertexInstance start;
    VertexInstance end;

    // The number of vertex member in this struct
    constexpr static std::size_t NumVertices = 6;

    LightVertexInstanceSet(Vector3& center, Projected<Vector3>& projected, 
                           const SelectionChangedSlot& selectionChanged) :
        center(center, selectionChanged),
        target(projected.target, selectionChanged),
        right(projected.right, projected.target, selectionChanged),
        up(projected.up, projected.target, selectionChanged),
        start(projected.start, selectionChanged),
        end(projected.end, selectionChanged)
    {}
};

}
