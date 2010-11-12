#ifndef _MODELSKINCACHE_INTERFACE_H_
#define _MODELSKINCACHE_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

#include "modelskin.h"

namespace script {

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
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<ModelSkinCacheInterface> ModelSkinCacheInterfacePtr;

} // namespace script

#endif /* _MODELSKINCACHE_INTERFACE_H_ */
