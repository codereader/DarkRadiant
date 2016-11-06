#pragma once

#include "imapinfofile.h"
#include "iselectionset.h"

namespace selection
{

/**
 * Info file module importing/exporting the selection set mapping 
 * to the .darkradiant file of persistence between mapping sessions.
 */
class SelectionSetInfoFileModule :
	public map::IMapInfoFileModule
{
private:
	struct SelectionSetImportInfo
	{
		// The name of this set
		std::string name;

		// The node indices, which will be resolved to nodes after import
		std::set<map::NodeIndexPair> nodeIndices;
	};

	// Parsed selection set information
	std::vector<SelectionSetImportInfo> _importInfo;

	struct SelectionSetExportInfo
	{
		// The set we're working with
		selection::ISelectionSetPtr set;

		// The nodes in this set
		std::set<scene::INodePtr> nodes;

		// The node indices, which will be resolved during traversal
		std::set<map::NodeIndexPair> nodeIndices;
	};

	// SelectionSet-related
	typedef std::vector<SelectionSetExportInfo> SelectionSetInfo;
	SelectionSetInfo _exportInfo;

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
};

}
