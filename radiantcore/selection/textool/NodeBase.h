#pragma once

#include <vector>
#include "itexturetoolmodel.h"
#include "../BasicSelectable.h"
#include "SelectableVertex.h"

namespace textool
{

class NodeBase :
    public virtual INode,
    public virtual IComponentSelectable
{
private:
    selection::BasicSelectable _selectable;

protected:
    std::vector<SelectableVertex> _vertices;

public:
    virtual void setSelected(bool select) override
    {
        _selectable.setSelected(select);
    }

    virtual bool isSelected() const override
    {
        return _selectable.isSelected();
    }

    virtual bool hasSelectedComponents() const override
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

    virtual void clearComponentSelection() override
    {
        for (auto& vertex : _vertices)
        {
            vertex.setSelected(false);
        }
    }

    virtual void testSelectComponents(Selector& selector, SelectionTest& test) override
    {
        test.BeginMesh(Matrix4::getIdentity(), true);

        for (auto& vertex : _vertices)
        {
            SelectionIntersection intersection;
            test.TestPoint(Vector3(vertex.getVertex().x(), vertex.getVertex().y(), 0), intersection);

            if (intersection.isValid())
            {
                Selector_add(selector, vertex);
            }
        }
    }

    virtual AABB getSelectedComponentBounds() override
    {
        AABB bounds;

        for (const auto& vertex : _vertices)
        {
            if (!vertex.isSelected()) continue;

            bounds.includePoint({ vertex.getVertex().x(), vertex.getVertex().y(), 0 });
        }

        return bounds;
    }

protected:
    virtual void renderComponents()
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
            glVertex3d(vertex.getVertex().x(), vertex.getVertex().y(), vertex.isSelected() ? 0.1f : 0);
        }

        glEnd();
        glDisable(GL_DEPTH_TEST);
    }
};

}
