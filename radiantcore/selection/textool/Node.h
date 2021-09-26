#pragma once

#include <vector>
#include "itexturetoolmodel.h"
#include <sigc++/functors/mem_fun.h>
#include "itexturetoolmodel.h"
#include "ObservedSelectable.h"
#include "SelectableVertex.h"

namespace textool
{

class Node :
    public virtual INode,
    public virtual IComponentSelectable
{
private:
    selection::ObservedSelectable _selectable;

protected:
    std::vector<SelectableVertex> _vertices;

public:
    Node() :
        _selectable(sigc::mem_fun(*this, &Node::onSelectionStatusChanged))
    {}

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

    virtual std::size_t getNumSelectedComponents() const override
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
            test.TestPoint(Vector3(vertex.getTexcoord().x(), vertex.getTexcoord().y(), 0), intersection);

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

            bounds.includePoint({ vertex.getTexcoord().x(), vertex.getTexcoord().y(), 0 });
        }

        return bounds;
    }

    virtual void expandComponentSelectionToRelated() override
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
            glVertex3d(vertex.getTexcoord().x(), vertex.getTexcoord().y(), vertex.isSelected() ? 0.1f : 0);
        }

        glEnd();
        glDisable(GL_DEPTH_TEST);
    }

private:
    void onSelectionStatusChanged(const ISelectable& selectable)
    {
        GlobalTextureToolSelectionSystem().onNodeSelectionChanged(*this);
    }
};

}
