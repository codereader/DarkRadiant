#pragma once

#include <vector>
#include "render/Colour4.h"
#include "math/AABB.h"
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
    Node();

    virtual void setSelected(bool select) override;
    virtual bool isSelected() const override;

    virtual bool hasSelectedComponents() const override;
    virtual std::size_t getNumSelectedComponents() const override;
    virtual void clearComponentSelection() override;
    virtual void testSelectComponents(Selector& selector, SelectionTest& test) override;
    virtual AABB getSelectedComponentBounds() override;
    virtual void expandComponentSelectionToRelated() override;

protected:
    virtual void renderComponents();
    virtual Colour4 getSurfaceColour(SelectionMode mode);

private:
    void onSelectionStatusChanged(const ISelectable& selectable);
};

}
