#pragma once

#include "selection/BasicSelectable.h"

namespace textool
{

class SelectableVertex :
    public selection::BasicSelectable
{
private:
    Vector2& _texcoord;

public:
    SelectableVertex(Vector2& texcoord) :
        _texcoord(texcoord)
    {}

    const Vector2& getVertex() const
    {
        return _texcoord;
    }

    Vector2& getVertex()
    {
        return _texcoord;
    }
};

}
