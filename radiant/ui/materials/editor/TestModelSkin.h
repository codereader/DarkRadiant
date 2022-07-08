#pragma once

#include "modelskin.h"

namespace ui
{

// Temporary skin implementation used to assign the given material 
class TestModelSkin final
{
private:
    MaterialPtr _material;

    const char* const NAME_PREFIX = "__internal_test_skin__";
    std::string _nameSuffix;

    decl::ISkin::Ptr _skin;

public:
    const char* const TEST_MODEL_MATERIAL = "material";

    TestModelSkin(const std::string& name) :
        _nameSuffix(name),
        _skin(std::static_pointer_cast<decl::ISkin>(
            GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Skin, getSkinName())))
    {}

    std::string getSkinName() const
    {
        return NAME_PREFIX + _nameSuffix;
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

        decl::DeclarationBlockSyntax syntax;

        syntax.modName = "None";
        syntax.typeName = "skin";
        syntax.name = getSkinName();

        syntax.contents = _material ?
            fmt::format("{0}\t{1}", TEST_MODEL_MATERIAL, _material->getName()) : "";

        _skin->setBlockSyntax(syntax);
    }
};

}
