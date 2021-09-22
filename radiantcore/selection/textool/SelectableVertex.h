#pragma once

#include "selection/BasicSelectable.h"

namespace textool
{

class SelectableVertex :
    public selection::BasicSelectable
{
private:
    Vector3& _vertex;
    Vector2& _texcoord;

public:
    SelectableVertex(Vector3& vertex, Vector2& texcoord) :
        _vertex(vertex),
        _texcoord(texcoord)
    {}

    const Vector3& getVertex() const
    {
        return _vertex;
    }

    Vector3& getVertex()
    {
        return _vertex;
    }

    const Vector2& getTexcoord() const
    {
        return _texcoord;
    }

    Vector2& getTexcoord()
    {
        return _texcoord;
    }
};

}
