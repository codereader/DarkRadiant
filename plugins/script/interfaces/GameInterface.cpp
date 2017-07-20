#include "GameInterface.h"

#include <pybind11/pybind11.h>

namespace script 
{

ScriptGame::ScriptGame(const game::IGamePtr& game) :
	_game(game)
{}

std::string ScriptGame::getKeyValue(const std::string& key) const
{
	return (_game != NULL) ? _game->getKeyValue(key) : "";
}

// -----------------------------------------------

std::string GameInterface::getUserEnginePath()
{
	return GlobalGameManager().getUserEnginePath();
}

std::string GameInterface::getModPath()
{
	return GlobalGameManager().getModPath();
}

std::string GameInterface::getModBasePath()
{
	return GlobalGameManager().getModBasePath();
}

std::string GameInterface::getFSGame()
{
	return GlobalGameManager().getFSGame();
}

std::string GameInterface::getFSGameBase()
{
	return GlobalGameManager().getFSGameBase();
}

ScriptGame GameInterface::currentGame() 
{
	return ScriptGame(GlobalGameManager().currentGame());
}

GameInterface::PathList GameInterface::getVFSSearchPaths()
{
	game::IGameManager::PathList paths = GlobalGameManager().getVFSSearchPaths();

	PathList pathVector;
	pathVector.assign(paths.begin(), paths.end()); // copy the list

	return pathVector;
}

// IScriptInterface implementation
void GameInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Add the Game object declaration
	py::class_<ScriptGame> game(scope, "Game");
	game.def(py::init<const game::IGamePtr&>());
	game.def("getKeyValue", &ScriptGame::getKeyValue);

	// Add the module declaration to the given python namespace
	py::class_<GameInterface> gameManager(scope, "GameManager");

	gameManager.def("getUserEnginePath", &GameInterface::getUserEnginePath);
	gameManager.def("getModPath", &GameInterface::getModPath);
	gameManager.def("getModBasePath", &GameInterface::getModBasePath);
	gameManager.def("getFSGame", &GameInterface::getFSGame);
	gameManager.def("getFSGameBase", &GameInterface::getFSGameBase);
	gameManager.def("currentGame", &GameInterface::currentGame);
	gameManager.def("getVFSSearchPaths", &GameInterface::getVFSSearchPaths);

	// Now point the Python variable "GlobalGameManager" to this instance
	globals["GlobalGameManager"] = this;
}

} // namespace script
