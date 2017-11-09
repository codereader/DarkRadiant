#include "GameSetupPage.h"

#include "itextstream.h"
#include "GameSetupPageIdTech.h"
#include <functional>

namespace ui
{

GameSetupPage::GameSetupPage(wxWindow* parent, const game::IGamePtr& game) :
	wxPanel(parent, wxID_ANY),
	_game(game)
{}

// Static instance
GameSetupPage::GameSetupPages GameSetupPage::_registeredPages;

void GameSetupPage::EnsureDefaultPages()
{
	if (_registeredPages.empty())
	{
		_registeredPages[GameSetupPageIdTech::TYPE()] = [](wxWindow* parent, const game::IGamePtr& game)
		{ 
			return new GameSetupPageIdTech(parent, game);
		};
	}
}

GameSetupPage* GameSetupPage::CreatePageForGame(const game::IGamePtr& game, wxWindow* parent)
{
	EnsureDefaultPages();

	// Check the game setup dialog type, default to idTech generic
	std::string type = GameSetupPageIdTech::TYPE();

	xml::NodeList nodes = game->getLocalXPath("/gameSetup/dialog");

	if (!nodes.empty())
	{
		std::string value = nodes[0].getAttributeValue("type");

		if (!value.empty())
		{
			type = value;
		}
	}

	GameSetupPages::const_iterator found = _registeredPages.find(type);

	if (found != _registeredPages.end())
	{
		return found->second(parent, game);
	}
	
	rWarning() << "No Game Setup Page associated to type " << type << 
		", will fall back to a generic idTech setup page." << std::endl;

	return new GameSetupPageIdTech(parent, game);
}

}
