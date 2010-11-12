#include "GameInterface.h"

#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace script {

ScriptGame::ScriptGame(const game::IGamePtr& game) :
	_game(game)
{}

std::string ScriptGame::getKeyValue(const std::string& key) const
{
	return (_game != NULL) ? _game->getKeyValue(key) : "";
}


// -----------------------------------------------

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

ScriptGame GameInterface::currentGame() {
	return ScriptGame(GlobalGameManager().currentGame());
}

GameInterface::PathList GameInterface::getVFSSearchPaths() {
	game::IGameManager::PathList paths = GlobalGameManager().getVFSSearchPaths();

	PathList pathVector;
	pathVector.assign(paths.begin(), paths.end()); // copy the list

	return pathVector;
}

// IScriptInterface implementation
void GameInterface::registerInterface(boost::python::object& nspace)
{
	// Add the Game object declaration
	nspace["Game"] = boost::python::class_<ScriptGame>("Game", boost::python::init<const game::IGamePtr&>())
		.def("getKeyValue", &ScriptGame::getKeyValue)
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalGameManager"] = boost::python::class_<GameInterface>("GlobalGameManager")
		.def("getModPath", &GameInterface::getModPath)
		.def("getModBasePath", &GameInterface::getModBasePath)
		.def("getFSGame", &GameInterface::getFSGame)
		.def("getFSGameBase", &GameInterface::getFSGameBase)
		.def("currentGame", &GameInterface::currentGame)
		.def("getVFSSearchPaths", &GameInterface::getVFSSearchPaths)
	;

	// Now point the Python variable "GlobalGameManager" to this instance
	nspace["GlobalGameManager"] = boost::python::ptr(this);
}

} // namespace script
