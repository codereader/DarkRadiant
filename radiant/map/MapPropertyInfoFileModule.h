#pragma once

#include "imapinfofile.h"
#include "KeyValueStore.h"

namespace map
{

/**
* Info File Module to persist the map property bag to the
* .darkradiant map info file.
*/
class MapPropertyInfoFileModule :
	public IMapInfoFileModule
{
private:
	KeyValueStore _store;

public:
	std::string getName() override;

	void onInfoFileSaveStart() override;
	void onBeginSaveMap(const scene::IMapRootNodePtr& root) override;
	void onFinishSaveMap(const scene::IMapRootNodePtr& root) override;
	void onSavePrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum) override;
	void onSaveEntity(const scene::INodePtr& node, std::size_t entityNum) override;
	void writeBlocks(std::ostream& stream) override;
	void onInfoFileSaveFinished() override;

	void onInfoFileLoadStart() override;
	bool canParseBlock(const std::string& blockName) override;
	void parseBlock(const std::string& blockName, parser::DefTokeniser& tok) override;
	void applyInfoToScene(const scene::IMapRootNodePtr& root, const NodeIndexMap& nodeMap) override;
	void onInfoFileLoadFinished() override;
};

}
