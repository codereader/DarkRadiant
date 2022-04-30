#pragma once

#include "itexturetoolmodel.h"
#include "ObservedSelectable.h"

namespace textool
{

class SelectableVertex :
    public selection::ObservedSelectable
{
private:
    Vector3& _vertex;
    Vector2& _texcoord;

public:
    SelectableVertex(Vector3& vertex, Vector2& texcoord) :
        ObservedSelectable(std::bind(&SelectableVertex::onSelectionStatusChanged, this, std::placeholders::_1)),
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

private:
    void onSelectionStatusChanged(const ISelectable& selectable)
    {
        GlobalTextureToolSelectionSystem().onComponentSelectionChanged(*this);
    }
};

}
