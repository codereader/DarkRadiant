#include "Node.h"

#include <sigc++/functors/mem_fun.h>

namespace textool
{

Node::Node() :
    _selectable(sigc::mem_fun(*this, &Node::onSelectionStatusChanged))
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
            Selector_add(selector, vertex);
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

void Node::renderComponents()
{
    glPointSize(5);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glBegin(GL_POINTS);

    for (const auto& vertex : _vertices)
    {
        if (vertex.isSelected())
        {
            glColor3f(1, 0.5f, 0);
        }
        else
        {
            glColor3f(0.8f, 0.8f, 0.8f);
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
