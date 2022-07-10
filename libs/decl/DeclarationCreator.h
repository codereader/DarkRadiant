#pragma once

#include "ideclmanager.h"

namespace decl
{

// Helper template implementing the IDeclarationCreator interface
// handing out the type info and new instances of the given DeclarationType.
template<typename DeclarationType>
class DeclarationCreator :
    public IDeclarationCreator
{
private:
    static_assert(std::is_base_of_v<IDeclaration, DeclarationType>,
        "DeclarationType type must inherit from IDeclaration");

    Type _type;

public:
    DeclarationCreator(Type type) :
        _type(type)
    {}

    Type getDeclType() const override
    {
        return _type;
    }

    IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        return std::make_shared<DeclarationType>(name);
    }
};

}
