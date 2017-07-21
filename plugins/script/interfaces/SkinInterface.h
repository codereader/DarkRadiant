#pragma once

#include "iscript.h"

#include "modelskin.h"

namespace script 
{

// Wrapper class to represent a ModelSkin object in Python
class ScriptModelSkin
{
	ModelSkin& _skin;
public:
	ScriptModelSkin(ModelSkin& skin) :
		_skin(skin)
	{}

	std::string getName()
	{
		return _skin.getName();
	}

	std::string getRemap(const std::string& name)
	{
		return _skin.getRemap(name);
	}
};

/**
 * greebo: This class registers the GlobalSkinCache interface with the
 * scripting system.
 */
class ModelSkinCacheInterface :
	public IScriptInterface
{
public:
	// Mapped methods
	ScriptModelSkin capture(const std::string& name);
	StringList getSkinsForModel(const std::string& model);
	StringList getAllSkins();
	void refresh();

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
