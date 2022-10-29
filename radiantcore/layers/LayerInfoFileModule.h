#pragma once

#include "imapinfofile.h"
#include <sstream>

namespace scene
{

class LayerInfoFileModule :
	public map::IMapInfoFileModule
{
private:
	// Number of node-to-layer mappings written
	std::size_t _layerInfoCount;

	// Buffer to hold our output
	std::stringstream _output;
	std::stringstream _layerNameBuffer;
	std::stringstream _layerHierarchyBuffer;

	// The list of layernames
    std::map<int, std::string> _layerNames;

    std::map<int, int> _layerParentIds;
    int _activeLayerId;
    std::vector<int> _hiddenLayerIds;

	typedef std::vector<scene::LayerList> LayerLists;
	LayerLists _layerMappings;

	// The standard list (node is part of layer 0)
	scene::LayerList _standardLayerList;

public:
	LayerInfoFileModule();

	std::string getName() override;

	void onInfoFileSaveStart() override;
	void onBeginSaveMap(const scene::IMapRootNodePtr& root) override;
	void onFinishSaveMap(const scene::IMapRootNodePtr& root) override;
	void onSavePrimitive(const INodePtr& node, std::size_t entityNum, std::size_t primitiveNum) override;
	void onSaveEntity(const INodePtr& node, std::size_t entityNum) override;
	void writeBlocks(std::ostream& stream) override;
	void onInfoFileSaveFinished() override;

	void onInfoFileLoadStart() override;
	bool canParseBlock(const std::string& blockName) override;
	void parseBlock(const std::string& blockName, parser::DefTokeniser& tok) override;
	void applyInfoToScene(const IMapRootNodePtr& root, const map::NodeIndexMap& nodeMap) override;
	void onInfoFileLoadFinished() override;

private:
	void saveNode(const INodePtr& node);
	void clear();

	void parseLayerNames(parser::DefTokeniser& tok);
	void parseNodeToLayerMapping(parser::DefTokeniser& tok);
	void parseLayerHierarchy(parser::DefTokeniser& tok);
	void parseLayerProperties(parser::DefTokeniser& tok);
};

}
