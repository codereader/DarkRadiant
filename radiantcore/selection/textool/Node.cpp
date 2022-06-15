#include "Node.h"

#include "math/Vector2.h"
#include "math/Matrix4.h"
#include "itexturetoolcolours.h"
#include <sigc++/functors/mem_fun.h>
#include <GL/glew.h>

namespace textool
{

Node::Node() :
    _selectable(std::bind(&Node::onSelectionStatusChanged, this, std::placeholders::_1))
{}

void Node::setSelected(bool select)
{
    _selectable.setSelected(select);
}

bool Node::isSelected() const
{
    return _selectable.isSelected();
}

bool Node::hasSelectedComponents() const
{
    for (auto& vertex : _vertices)
    {
        if (vertex.isSelected())
        {
            return true;
        }
    }

    return false;
}

std::size_t Node::getNumSelectedComponents() const
{
    std::size_t count = 0;

    for (const auto& vertex : _vertices)
    {
        if (vertex.isSelected())
        {
            ++count;
        }
    }

    return count;
}

void Node::clearComponentSelection()
{
    for (auto& vertex : _vertices)
    {
        vertex.setSelected(false);
    }
}

void Node::testSelectComponents(Selector& selector, SelectionTest& test)
{
    test.BeginMesh(Matrix4::getIdentity(), true);

    for (auto& vertex : _vertices)
    {
        SelectionIntersection intersection;
        test.TestPoint(Vector3(vertex.getTexcoord().x(), vertex.getTexcoord().y(), 0), intersection);

        if (intersection.isValid())
        {
            selector.addWithNullIntersection(vertex);
        }
    }
}

AABB Node::getSelectedComponentBounds()
{
    AABB bounds;

    for (const auto& vertex : _vertices)
    {
        if (!vertex.isSelected()) continue;

        bounds.includePoint({ vertex.getTexcoord().x(), vertex.getTexcoord().y(), 0 });
    }

    return bounds;
}

void Node::expandComponentSelectionToRelated()
{
    if (!hasSelectedComponents())
    {
        return;
    }

    for (auto& vertex : _vertices)
    {
        vertex.setSelected(true);
    }
}

Colour4 Node::getSurfaceColour(SelectionMode mode)
{
    if (mode == SelectionMode::Surface && isSelected())
    {
        return GlobalTextureToolColourSchemeManager().getColour(SchemeElement::SelectedSurface);
    }
    else if (mode == SelectionMode::Vertex)
    {
        return GlobalTextureToolColourSchemeManager().getColour(SchemeElement::SurfaceInComponentMode);
    }
    else
    {
        return GlobalTextureToolColourSchemeManager().getColour(SchemeElement::SurfaceInSurfaceMode);
    }
}

void Node::renderComponents()
{
    glPointSize(5);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glBegin(GL_POINTS);

    auto deselectedColour = GlobalTextureToolColourSchemeManager().getColour(SchemeElement::Vertex);
    auto selectedColour = GlobalTextureToolColourSchemeManager().getColour(SchemeElement::SelectedVertex);

    for (const auto& vertex : _vertices)
    {
        if (vertex.isSelected())
        {
            glColor4fv(selectedColour);
        }
        else
        {
            glColor4fv(deselectedColour);
        }

        // Move the selected vertices a bit up in the Z area
        glVertex3d(vertex.getTexcoord().x(), vertex.getTexcoord().y(), vertex.isSelected() ? 0.1f : 0);
    }

    glEnd();
    glDisable(GL_DEPTH_TEST);
}

void Node::onSelectionStatusChanged(const ISelectable& selectable)
{
    GlobalTextureToolSelectionSystem().onNodeSelectionChanged(*this);
}

}
