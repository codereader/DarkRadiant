#pragma once

#include "iscript.h"

#include "igame.h"
#include <vector>

namespace script 
{

class ScriptGame
{
	game::IGamePtr _game;
public:
	ScriptGame(const game::IGamePtr& game);

	std::string getKeyValue(const std::string& key) const;
};

class GameInterface :
	public IScriptInterface
{
public:
	// Redefine the path list to be of type std::vector
	typedef std::vector<std::string> PathList;

	std::string getUserEnginePath();
	std::string getModPath();
	std::string getModBasePath();
	std::string getFSGame();
	std::string getFSGameBase();
	ScriptGame currentGame();
	PathList getVFSSearchPaths();

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
