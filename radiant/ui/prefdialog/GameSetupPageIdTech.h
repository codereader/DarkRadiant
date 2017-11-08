#pragma once

#include "GameSetupPage.h"
#include "registry/buffer.h"

class wxTextCtrl;
namespace wxutil { class PathEntry; }

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

	wxTextCtrl* _fsGameEntry;
	wxTextCtrl* _fsGameBaseEntry;
	wxutil::PathEntry* _enginePathEntry;

	std::string _enginePath;
	std::string _modBasePath;
	std::string _modPath;

public:
	GameSetupPageIdTech(wxWindow* parent);

	static const char* TYPE();

	const char* getType() override;
	void validateSettings() override;

	std::string getEnginePath() override;
	std::string getModBasePath() override;
	std::string getModPath() override;

private:
	void constructPaths();

	wxTextCtrl* createEntry(const std::string& registryKey);
	wxutil::PathEntry* createPathEntry(const std::string& registryKey);
};

}
