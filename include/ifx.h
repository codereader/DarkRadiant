#pragma once

#include "ideclmanager.h"

namespace fx
{

class IFxAction
{
public:
    using Ptr = std::shared_ptr<IFxAction>;

    virtual ~IFxAction() {}


};

class IFxDeclaration :
    public decl::IDeclaration
{
public:
    using Ptr = std::shared_ptr<IFxDeclaration>;

    // Returns the number of actions in this FX declaration
    virtual std::size_t getNumActions() = 0;

    // Returns the n-th action (based on the given 0-based index)
    virtual IFxAction::Ptr getAction(std::size_t index) = 0;
};

}
