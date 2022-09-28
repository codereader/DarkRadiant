#include "LayerInterface.h"

#include <pybind11/pybind11.h>

namespace script
{

namespace
{
inline scene::ILayerManager& getMapLayerManager()
{
	if (!GlobalMapModule().getRoot())
	{
		throw std::runtime_error("No map loaded.");
	}

	return GlobalMapModule().getRoot()->getLayerManager();
}

}

int LayerInterface::createLayer(const std::string &name)
{
	try
	{
		return getMapLayerManager().createLayer(name);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return -1;
	}
}

int LayerInterface::createLayer(const std::string &name, int layerID)
{
	try
	{
		return getMapLayerManager().createLayer(name, layerID);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return -1;
	}
}

void LayerInterface::deleteLayer(const std::string &name)
{
	try
	{
		getMapLayerManager().deleteLayer(name);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::foreachLayer(LayerVisitor &visitor)
{
	try
	{
		getMapLayerManager().foreachLayer([&](int layerId, const std::string& layerName)
		{
			visitor.visit(layerId, layerName);
		});
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

int LayerInterface::getLayerID(const std::string& name)
{
	try
	{
		return getMapLayerManager().getLayerID(name);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return -1;
	}
}

std::string LayerInterface::getLayerName(int layerID)
{
	try
	{
		return getMapLayerManager().getLayerName(layerID);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return "";
	}
}

bool LayerInterface::layerExists(int layerID)
{
	try
	{
		return getMapLayerManager().layerExists(layerID);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return false;
	}
}

bool LayerInterface::renameLayer(int layerID, const std::string &newLayerName)
{
	try
	{
		return getMapLayerManager().renameLayer(layerID, newLayerName);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return false;
	}
}

int LayerInterface::getFirstVisibleLayer()
{
	try
	{
		return getMapLayerManager().getFirstVisibleLayer();
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return -1;
	}
}

int LayerInterface::getActiveLayer()
{
	try
	{
		return getMapLayerManager().getActiveLayer();
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return -1;
	}
}

void LayerInterface::setActiveLayer(int layerID)
{
	try
	{
		getMapLayerManager().setActiveLayer(layerID);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

bool LayerInterface::layerIsVisible(const std::string &layerName)
{
	try
	{
		return getMapLayerManager().layerIsVisible(getLayerID(layerName));
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return false;
	}
}

bool LayerInterface::layerIsVisible(int layerID)
{
	try
	{
		return getMapLayerManager().layerIsVisible(layerID);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
		return false;
	}
}

void LayerInterface::setLayerVisibility(const std::string &layerName, bool visible)
{
	try
	{
		getMapLayerManager().setLayerVisibility(getLayerID(layerName), visible);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::setLayerVisibility(int layerID, bool visible)
{
	try
	{
		getMapLayerManager().setLayerVisibility(layerID, visible);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::addSelectionToLayer(const std::string &layerName)
{
	try
	{
		getMapLayerManager().addSelectionToLayer(getLayerID(layerName));
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::addSelectionToLayer(int layerID)
{
	try
	{
		getMapLayerManager().addSelectionToLayer(layerID);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::moveSelectionToLayer(const std::string &layerName)
{
	try
	{
		getMapLayerManager().moveSelectionToLayer(getLayerID(layerName));
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::moveSelectionToLayer(int layerID)
{
	try
	{
		getMapLayerManager().moveSelectionToLayer(layerID);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::removeSelectionFromLayer(const std::string &layerName)
{
	try
	{
		getMapLayerManager().removeSelectionFromLayer(getLayerID(layerName));
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::removeSelectionFromLayer(int layerID)
{
	try
	{
		getMapLayerManager().removeSelectionFromLayer(layerID);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::setSelected(int layerID, bool selected)
{
	try
	{
		getMapLayerManager().setSelected(layerID, selected);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

int LayerInterface::getParentLayer(int layerId)
{
	try
	{
		return getMapLayerManager().getParentLayer(layerId);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
        return -1;
	}
}

void LayerInterface::setParentLayer(int childlayerId, int parentLayerId)
{
	try
	{
		getMapLayerManager().setParentLayer(childlayerId, parentLayerId);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void LayerInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Expose the LayerVisitor interface
	py::class_<LayerVisitor, LayerVisitorWrapper> visitor(scope, "LayerVisitor");

	visitor.def(py::init<>());
	visitor.def("visit", &LayerVisitor::visit);

	// Add the module declaration to the given python namespace
	py::class_<LayerInterface> layerManager(scope, "LayerManager");

	layerManager.def("createLayer", py::overload_cast<const std::string&>(&LayerInterface::createLayer));
	layerManager.def("createLayer", py::overload_cast<const std::string&, int>(&LayerInterface::createLayer));
	layerManager.def("deleteLayer", &LayerInterface::deleteLayer);
	layerManager.def("foreachLayer", &LayerInterface::foreachLayer);
	layerManager.def("getLayerID", &LayerInterface::getLayerID);
	layerManager.def("getLayerName", &LayerInterface::getLayerName);
	layerManager.def("layerExists", &LayerInterface::layerExists);
	layerManager.def("renameLayer", &LayerInterface::renameLayer);
	layerManager.def("getFirstVisibleLayer", &LayerInterface::getFirstVisibleLayer);
	layerManager.def("getActiveLayer", &LayerInterface::getActiveLayer);
	layerManager.def("setActiveLayer", &LayerInterface::setActiveLayer);
	layerManager.def("layerIsVisible", py::overload_cast<const std::string&>(&LayerInterface::layerIsVisible));
	layerManager.def("layerIsVisible", py::overload_cast<int>(&LayerInterface::layerIsVisible));
	layerManager.def("setLayerVisibility", py::overload_cast<const std::string&, bool>(&LayerInterface::setLayerVisibility));
	layerManager.def("setLayerVisibility", py::overload_cast<int, bool>(&LayerInterface::setLayerVisibility));
	layerManager.def("addSelectionToLayer", py::overload_cast<const std::string&>(&LayerInterface::addSelectionToLayer));
	layerManager.def("addSelectionToLayer", py::overload_cast<int>(&LayerInterface::addSelectionToLayer));
	layerManager.def("moveSelectionToLayer", py::overload_cast<const std::string&>(&LayerInterface::moveSelectionToLayer));
	layerManager.def("moveSelectionToLayer", py::overload_cast<int>(&LayerInterface::moveSelectionToLayer));
	layerManager.def("removeSelectionFromLayer", py::overload_cast<const std::string&>(&LayerInterface::removeSelectionFromLayer));
	layerManager.def("removeSelectionFromLayer", py::overload_cast<int>(&LayerInterface::removeSelectionFromLayer));
	layerManager.def("setSelected", &LayerInterface::setSelected);
	layerManager.def("getParentLayer", &LayerInterface::getParentLayer);
	layerManager.def("setParentLayer", &LayerInterface::setParentLayer);

	// Now point the Python variable "GlobalLayerManager" to this instance
	globals["GlobalLayerManager"] = this;
}


}
