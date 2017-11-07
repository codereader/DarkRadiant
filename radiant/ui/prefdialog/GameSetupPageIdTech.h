#pragma once

#include "GameSetupPage.h"
#include "registry/buffer.h"

namespace ui
{

/**
* A Setup Page for generic idTech-based games like
* Doom3, Quake3, Quake4, etc.
*/
class GameSetupPageIdTech :
	public GameSetupPage
{
private:
	// We're holding back any registry write operations until the user clicks OK
	registry::Buffer _registryBuffer;

	// A signal chain all registry key-bound widgets are connected with
	// when emitted, the widgets reload the values from the registry.
	sigc::signal<void> _resetValuesSignal;

public:
	GameSetupPageIdTech(wxWindow* parent);

	static const char* TYPE();

	const char* getType() override;

	void validateSettings() override;
	void saveSettings() override;

private:
	wxWindow* createEntry(const std::string& registryKey);
};

}
