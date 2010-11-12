#include "SkinInterface.h"

#include "modelskin.h"

namespace script {

ScriptModelSkin ModelSkinCacheInterface::capture(const std::string& name)
{
	return ScriptModelSkin(GlobalModelSkinCache().capture(name));
}

StringList ModelSkinCacheInterface::getSkinsForModel(const std::string& model)
{
	return GlobalModelSkinCache().getSkinsForModel(model);
}

StringList ModelSkinCacheInterface::getAllSkins()
{
	return GlobalModelSkinCache().getAllSkins();
}

void ModelSkinCacheInterface::refresh()
{
	GlobalModelSkinCache().refresh();
}

void ModelSkinCacheInterface::registerInterface(boost::python::object& nspace)
{
	// Declare the model skin
	nspace["ScriptModelSkin"] = boost::python::class_<ScriptModelSkin>(
		"ScriptModelSkin", boost::python::init<ModelSkin&>())
		.def("getName", &ScriptModelSkin::getName)
		.def("getRemap", &ScriptModelSkin::getRemap)
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalModelSkinCache"] = boost::python::class_<ModelSkinCacheInterface>("GlobalModelSkinCache")
		.def("getAllSkins", &ModelSkinCacheInterface::getAllSkins)
		.def("capture", &ModelSkinCacheInterface::capture)
		.def("getSkinsForModel", &ModelSkinCacheInterface::getSkinsForModel)
		.def("refresh", &ModelSkinCacheInterface::refresh)
	;

	// Now point the Python variable "GlobalModelSkinCache" to this instance
	nspace["GlobalModelSkinCache"] = boost::python::ptr(this);
}

} // namespace script
