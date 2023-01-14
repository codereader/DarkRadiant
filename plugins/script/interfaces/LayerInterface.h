#pragma once

#include <pybind11/pybind11.h>

#include "iscript.h"
#include "iscriptinterface.h"
#include "ilayer.h"

#include "SceneGraphInterface.h"

namespace script
{

class LayerVisitor
{
public:
	virtual ~LayerVisitor() {}
	virtual void visit(int layerID, const std::string& layerName) = 0;
};

// Wrap around the LayerVisitor interface
class LayerVisitorWrapper :
	public LayerVisitor
{
public:
	void visit(int layerID, const std::string& layerName) override
	{
		// Wrap this method to python
		PYBIND11_OVERLOAD_PURE(
			void,			/* Return type */
			LayerVisitor,	/* Parent class */
			visit,			/* Name of function in C++ (must match Python name) */
			layerID,		/* Argument(s) */
			layerName
		);
	}
};

class LayerInterface :
	public IScriptInterface
{
public:
	int createLayer(const std::string& name);
	int createLayer(const std::string& name, int layerID);
	void deleteLayer(const std::string& name);
	void foreachLayer(LayerVisitor& visitor);
	int getLayerID(const std::string &name);
	std::string getLayerName(int layerID);
	bool layerExists(int layerID);
	bool renameLayer(int layerID, const std::string& newLayerName);
	int getFirstVisibleLayer();
	int getActiveLayer();
	void setActiveLayer(int layerID);
	bool layerIsVisible(const std::string& layerName);
	bool layerIsVisible(int layerID);
	void setLayerVisibility(const std::string& layerName, bool visible);
	void setLayerVisibility(int layerID, bool visible);
	void addSelectionToLayer(const std::string& layerName);
	void addSelectionToLayer(int layerID);
	void moveSelectionToLayer(const std::string& layerName);
	void moveSelectionToLayer(int layerID);
	void removeSelectionFromLayer(const std::string& layerName);
	void removeSelectionFromLayer(int layerID);
	void setSelected(int layerID, bool selected);
    int getParentLayer(int layerId);
    void setParentLayer(int childLayerId, int parentLayerId);

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
