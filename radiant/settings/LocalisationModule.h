#pragma once

#include "imodule.h"

namespace settings
{

// Localisation module, taking care of the Language Preferences page
class LocalisationModule :
	public RegisterableModule
{
public:
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;
};

}
