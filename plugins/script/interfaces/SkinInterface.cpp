#include "SkinInterface.h"

#include <pybind11/pybind11.h>

#include "modelskin.h"

namespace script
{

ScriptModelSkin ModelSkinCacheInterface::capture(const std::string& name)
{
	return ScriptModelSkin(GlobalModelSkinCache().findSkin(name));
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

void ModelSkinCacheInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Declare the model skin
	py::class_<ScriptModelSkin> skin(scope, "ModelSkin");
	skin.def(py::init<const decl::ISkin::Ptr&>());
	skin.def("getName", &ScriptModelSkin::getName);
	skin.def("getRemap", &ScriptModelSkin::getRemap);

	// Add the module declaration to the given python namespace
	py::class_<ModelSkinCacheInterface> cache(scope, "ModelSkinCache");

	cache.def("getAllSkins", &ModelSkinCacheInterface::getAllSkins);
	cache.def("capture", &ModelSkinCacheInterface::capture);
	cache.def("getSkinsForModel", &ModelSkinCacheInterface::getSkinsForModel);
	cache.def("refresh", &ModelSkinCacheInterface::refresh);

	// Now point the Python variable "GlobalModelSkinCache" to this instance
	globals["GlobalModelSkinCache"] = this;
}

} // namespace script
