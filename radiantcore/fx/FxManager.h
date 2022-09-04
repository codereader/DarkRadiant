#pragma once

#include "imodule.h"

namespace fx
{

class FxManager final :
    public RegisterableModule
{
public:
    // RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
};

}
