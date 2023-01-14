#pragma once

#include "iscript.h"
#include "iscriptinterface.h"

#include "modelskin.h"

namespace script 
{

// Wrapper class to represent a ModelSkin object in Python
class ScriptModelSkin
{
	decl::ISkin::Ptr _skin;
public:
	ScriptModelSkin(const decl::ISkin::Ptr& skin) :
		_skin(skin)
	{}

	std::string getName()
	{
		return _skin ? _skin->getDeclName() : std::string();
	}

	std::string getRemap(const std::string& name)
	{
		return _skin ? _skin->getRemap(name) : std::string();
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
