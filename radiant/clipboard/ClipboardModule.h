#pragma once

#include "iclipboard.h"

class wxActivateEvent;

namespace ui
{

class ClipboardModule :
	public radiant::IClipboard
{
private:
    sigc::signal<void> _sigContentsChanged;
    std::string _contentHash;

public:
	std::string getString() override;
	void setString(const std::string& str) override;
    virtual sigc::signal<void>& signal_clipboardContentChanged() override;

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
    void onAppActivated(wxActivateEvent& ev);
};

}
