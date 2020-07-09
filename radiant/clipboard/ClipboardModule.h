#pragma once

#include "iclipboard.h"

namespace ui
{

class ClipboardModule :
	public radiant::IClipboard
{
public:
	std::string getString() override;
	void setString(const std::string& str) override;

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
};

}
