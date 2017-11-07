#include "GameSetupPage.h"

#include "itextstream.h"
#include "GameSetupPageIdTech.h"
#include <functional>

namespace ui
{

GameSetupPage::GameSetupPage(wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{}

// Static instance
GameSetupPage::GameSetupPages GameSetupPage::_registeredPages;

void GameSetupPage::EnsureDefaultPages()
{
	if (_registeredPages.empty())
	{
		_registeredPages[GameSetupPageIdTech::TYPE()] = [](wxWindow* parent)
		{ 
			return new GameSetupPageIdTech(parent);
		};
	}
}

GameSetupPage* GameSetupPage::CreatePageForType(const std::string& type, wxWindow* parent)
{
	EnsureDefaultPages();

	GameSetupPages::const_iterator found = _registeredPages.find(type);

	if (found != _registeredPages.end())
	{
		return found->second(parent);
	}
	
	rWarning() << "No Game Setup Page associated to type " << type << 
		", will fall back to a generic idTech setup page." << std::endl;

	return new GameSetupPageIdTech(parent);
}

}
