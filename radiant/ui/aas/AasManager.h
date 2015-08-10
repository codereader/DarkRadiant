#pragma once

#include "iaasfile.h"
#include "imodule.h"

namespace map
{

class AasManager : 
    public RegisterableModule
{
public:


    // RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
};

}

map::AasManager& GlobalAasManager();
