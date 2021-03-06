#pragma once

#include "modelskin.h"

namespace ui
{

// Temporary skin implementation used to assign the given material 
class TestModelSkin final :
    public ModelSkin
{
private:
    MaterialPtr _material;

    const char* const NAME_PREFIX = "__internal_test_skin__";
    std::string _nameSuffix;

public:
    const char* const TEST_MODEL_MATERIAL = "material";

    TestModelSkin(const std::string& name) :
        _nameSuffix(name)
    {}

    std::string getName() const override
    {
        return NAME_PREFIX + _nameSuffix;
    }

    std::string getRemap(const std::string& name) const override
    {
        if (_material && name == TEST_MODEL_MATERIAL)
        {
            return _material->getName();
        }

        return std::string();
    }

    std::string getSkinFileName() const override
    {
        return "__defined_by_darkradiant.skin";
    }

    bool isEmpty() const
    {
        return !_material;
    }

    void clear()
    {
        _material.reset();
    }

    void setRemapMaterial(const MaterialPtr& material)
    {
        _material = material;
    }
};

}
