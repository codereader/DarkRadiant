#pragma once

#include <sstream>
#include "iselectiongroup.h"
#include "imapinfofile.h"

namespace selection
{

/**
 * Info file module importing/exporting the selection group info
 * to the .darkradiant file of persistence between mapping sessions.
 */
class SelectionGroupInfoFileModule :
	public map::IMapInfoFileModule
{
private:
	struct SelectionGroupImportInfo
	{
		// Group ID
		std::size_t id;

		// The name of this group
		std::string name;
	};

	// Parsed selection group information
	std::vector<SelectionGroupImportInfo> _groupInfo;
	
	// Parsed node mapping
	typedef std::map<map::NodeIndexPair, IGroupSelectable::GroupIds> NodeMapping;
	NodeMapping _nodeMapping;

	std::stringstream _output;

	std::size_t _nodeInfoCount;

public:
	std::string getName() override;

	void onInfoFileSaveStart() override;
	void onSavePrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum) override;
	void onSaveEntity(const scene::INodePtr& node, std::size_t entityNum) override;
	void writeBlocks(std::ostream& stream) override;
	void onInfoFileSaveFinished() override;

	void onInfoFileLoadStart() override;
	bool canParseBlock(const std::string& blockName) override;
	void parseBlock(const std::string& blockName, parser::DefTokeniser& tok) override;
	void applyInfoToScene(const scene::IMapRootNodePtr& root, const map::NodeIndexMap& nodeMap) override;
	void onInfoFileLoadFinished() override;

private:
	void saveNode(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum);
	void parseSelectionGroups(parser::DefTokeniser& tok);
	void parseNodeMappings(parser::DefTokeniser& tok);
};

}
