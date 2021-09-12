#pragma once

#include "imanipulator.h"

namespace selection
{

class TextureToolRotateManipulator :
    public IManipulator
{
private:
    std::size_t _id;
    bool _isSelected;

public:
    virtual std::size_t getId() const override;
    virtual void setId(std::size_t id) override;
    virtual Type getType() const override;

    virtual Component* getActiveComponent() override;

    virtual void setSelected(bool select) override;
    virtual bool isSelected() const override;
    virtual void testSelect(SelectionTest& test, const Matrix4& pivot2world) override;
};

}
