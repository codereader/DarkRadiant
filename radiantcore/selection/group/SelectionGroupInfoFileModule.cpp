#include "SelectionGroupInfoFileModule.h"

#include <limits>
#include "iselectiongroup.h"
#include "ientity.h"
#include "string/convert.h"
#include "string/replace.h"
#include "parser/DefTokeniser.h"

#include "scenelib.h"
#include "debugging/ScenegraphUtils.h"
#include "SelectionGroupManager.h"

namespace selection
{

namespace
{
	const char* const SELECTION_GROUPS = "SelectionGroups";
	const char* const SELECTION_GROUP = "SelectionGroup";
	const char* const NODE_MAPPING = "SelectionGroupNodeMapping";
	const char* const NODE = "Node";
	std::size_t EMPTY_PRIMITVE_NUM = std::numeric_limits<std::size_t>::max();
}

std::string SelectionGroupInfoFileModule::getName()
{
	return "Selection Groups";
}

void SelectionGroupInfoFileModule::clear()
{
	_groupInfo.clear();
	_nodeMapping.clear();

	_output.str(std::string());
	_output.clear();
	_selectionGroupBuffer.str(std::string());
	_selectionGroupBuffer.clear();
	_nodeInfoCount = 0;
}

void SelectionGroupInfoFileModule::onInfoFileSaveStart()
{
	clear();
}

void SelectionGroupInfoFileModule::onBeginSaveMap(const scene::IMapRootNodePtr& root)
{
	// Selection Group output
	_selectionGroupBuffer << "\t" << SELECTION_GROUPS << std::endl;

	_selectionGroupBuffer << "\t{" << std::endl;

	// SelectionGroup 0 { Name of this group }

	std::size_t selectionGroupCount = 0;

	root->getSelectionGroupManager().foreachSelectionGroup([&](ISelectionGroup& group)
	{
		// Ignore empty groups
		if (group.size() == 0) return;

		// Make sure to escape the quotes of the set name, use the XML quote entity
		_selectionGroupBuffer << "\t\t" << SELECTION_GROUP << " " << group.getId()
			<< " { \"" << string::replace_all_copy(group.getName(), "\"", "&quot;") << "\" }"
			<< std::endl;

		selectionGroupCount++;
	});

	_selectionGroupBuffer << "\t}" << std::endl;

	rMessage() << selectionGroupCount << " selection groups collected." << std::endl;
}

void SelectionGroupInfoFileModule::onFinishSaveMap(const scene::IMapRootNodePtr& root)
{}

void SelectionGroupInfoFileModule::onSavePrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum)
{
	saveNode(node, entityNum, primitiveNum);
}

void SelectionGroupInfoFileModule::onSaveEntity(const scene::INodePtr& node, std::size_t entityNum)
{
	saveNode(node, entityNum, EMPTY_PRIMITVE_NUM);
}

void SelectionGroupInfoFileModule::saveNode(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum)
{
	// Don't export the group settings for models and particles, as they are not there
	// at map load/parse time - these shouldn't even be passed in here
	assert(Node_isEntity(node) || Node_isPrimitive(node));

	std::shared_ptr<IGroupSelectable> selectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

	if (!selectable) return;

	const IGroupSelectable::GroupIds& ids = selectable->getGroupIds();

	// Ignore nodes that are not part of any group
	if (ids.empty()) return;

	// Node { ( EntityNum [PrimitiveNum] ) ( GroupId1 GroupId2 ... )
	// e.g.
	// Node { ( 3 78 ) ( 1 2 8 ) }

	// Open a Node block
	_output << "\t\t" << NODE << " { ";

	// Write the node coordinates
	_output << "( " << entityNum;

	if (primitiveNum != EMPTY_PRIMITVE_NUM)
	{
		_output << " " << primitiveNum;
	}
	
	_output << " )";

	_output << " ( ";

	// Write a space-separated list of group IDs
	for (const IGroupSelectable::GroupIds::value_type& i : ids)
	{
		_output << i << " ";
	}

	_output << ") ";

	// Close the Node block
	_output << "}";

	// Write additional node info, for easier debugging in case of issues
	_output << " // " << getNodeInfo(node);

	_output << std::endl;

	_nodeInfoCount++;
}

void SelectionGroupInfoFileModule::writeBlocks(std::ostream& stream)
{
	// Selection Group output
	stream << _selectionGroupBuffer.str();

	// Write the NodeToLayerMapping block
	stream << "\t" << NODE_MAPPING << std::endl;
	stream << "\t{" << std::endl;

	// Write the output buffer to the stream
	stream << _output.str();

	// Closing braces of NodeToLayerMapping block
	stream << "\t}" << std::endl;

	rMessage() << _nodeInfoCount << " selection group member mappings written." << std::endl;
}

void SelectionGroupInfoFileModule::onInfoFileSaveFinished()
{
	clear();
}

void SelectionGroupInfoFileModule::onInfoFileLoadStart()
{
	clear();
}

bool SelectionGroupInfoFileModule::canParseBlock(const std::string& blockName)
{
	return blockName == SELECTION_GROUPS || blockName == NODE_MAPPING;
}

void SelectionGroupInfoFileModule::parseBlock(const std::string& blockName, parser::DefTokeniser& tok)
{
	assert(canParseBlock(blockName));

	if (blockName == SELECTION_GROUPS)
	{
		parseSelectionGroups(tok);
	}
	else if (blockName == NODE_MAPPING)
	{
		parseNodeMappings(tok);
	}
}

void SelectionGroupInfoFileModule::parseSelectionGroups(parser::DefTokeniser& tok)
{
	// The opening brace
	tok.assertNextToken("{");

	while (tok.hasMoreTokens())
	{
		std::string token = tok.nextToken();

		if (token == SELECTION_GROUP)
		{
			// SelectionGroup 0 { Name of this group }

			// Get the ID
			std::string idStr = tok.nextToken();
			std::size_t id = string::convert<std::size_t>(idStr);

			tok.assertNextToken("{");

			// Parse the name, replacing the &quot; placeholder with a proper quote
			std::string name = string::replace_all_copy(tok.nextToken(), "&quot;", "\"");

			tok.assertNextToken("}");

			_groupInfo.push_back(SelectionGroupImportInfo());
			_groupInfo.back().id = id;
			_groupInfo.back().name = name;

			rDebug() << "[InfoFile]: Parsed group #" << id << " with name \"" << name << "\"" << std::endl;

			continue;
		}

		if (token == "}")
		{
			break;
		}
	}
}

void SelectionGroupInfoFileModule::parseNodeMappings(parser::DefTokeniser& tok)
{
	// The opening brace
	tok.assertNextToken("{");

	while (tok.hasMoreTokens())
	{
		std::string token = tok.nextToken();

		if (token == NODE)
		{
			// Node { ( 3 78 ) ( 1 2 8 ) }

			// Get the node coordinates
			tok.assertNextToken("{");

			tok.assertNextToken("(");

			// Entity number is always there
			std::size_t entityNum = string::convert<std::size_t>(tok.nextToken());
			std::size_t primitiveNum = EMPTY_PRIMITVE_NUM;

			token = tok.nextToken();

			// If we hit the closing parenthesis, we don't have a primitive number
			if (token != ")")
			{
				// We have a primitive number
				primitiveNum = string::convert<std::size_t>(token);
				tok.assertNextToken(")");
			}

			// Initialise the node mapping with an empty list
			NodeMapping::iterator mapped = _nodeMapping.insert(NodeMapping::value_type(
				map::NodeIndexPair(entityNum, primitiveNum),
				IGroupSelectable::GroupIds())).first;

			tok.assertNextToken("(");

			// Parse the group IDs until we hit the closing parenthesis
			for (token = tok.nextToken(); token != ")"; token = tok.nextToken())
			{
				std::size_t groupId = string::convert<std::size_t>(token);

				mapped->second.push_back(groupId);
			}

			// Node closed
			tok.assertNextToken("}");

			continue;
		}

		if (token == "}")
		{
			break;
		}
	}
}

void SelectionGroupInfoFileModule::applyInfoToScene(const scene::IMapRootNodePtr& root, const map::NodeIndexMap& nodeMap)
{
	// Remove all selection sets, there shouldn't be any left at this point
	root->getSelectionGroupManager().deleteAllSelectionGroups();

	typedef std::map<std::size_t, ISelectionGroupPtr> GroupMap;
	GroupMap groups;

	// Re-construct the groups first
	for (const SelectionGroupImportInfo& info : _groupInfo)
	{
		try
		{
			// Create the group by ID
			ISelectionGroupPtr group = root->getSelectionGroupManager().createSelectionGroup(info.id);
			group->setName(info.name);

			// Store for later retrieval
			groups[info.id] = group;
		}
		catch (std::runtime_error& ex)
		{
			rError() << ex.what() << std::endl;
		}
	}

	// Assign the nodes, as found in the mapping, keeping the group ID order intact
	std::size_t failedNodes = 0;

	for (const NodeMapping::value_type& mapping : _nodeMapping)
	{
		map::NodeIndexMap::const_iterator foundNode = nodeMap.find(mapping.first);

		if (foundNode != nodeMap.end())
		{
			// Assign this node to its groups, following the order
			for (const IGroupSelectable::GroupIds::value_type& id : mapping.second)
			{
				// Get the group and assign the node
				GroupMap::iterator found = groups.find(id);

				if (found == groups.end())
				{
					rWarning() << "Invalid group ID " << id << " encountered for node (" << 
						mapping.first.first << "," << mapping.first.second << ")" << std::endl;
					continue;
				}

				found->second->addNode(foundNode->second);
			}
		}
		else
		{
			failedNodes++;
		}
	}
		
	if (failedNodes > 0)
	{
		rWarning() << "Couldn't resolve " << failedNodes << " nodes in group mapping " << std::endl;
	}
}

void SelectionGroupInfoFileModule::onInfoFileLoadFinished()
{
	clear();
}

}
