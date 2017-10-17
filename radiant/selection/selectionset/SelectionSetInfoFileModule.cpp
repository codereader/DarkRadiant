#include "SelectionSetInfoFileModule.h"

#include <limits>
#include "itextstream.h"
#include "string/convert.h"
#include "string/replace.h"
#include "parser/DefTokeniser.h"

namespace selection
{

namespace
{
	const char* const SELECTION_SETS = "SelectionSets";
	const char* const SELECTION_SET = "SelectionSet";
	std::size_t EMPTY_PRIMITVE_NUM = std::numeric_limits<std::size_t>::max();
}

std::string SelectionSetInfoFileModule::getName()
{
	return "Selection Set Mapping";
}

void SelectionSetInfoFileModule::onInfoFileSaveStart()
{
	_exportInfo.clear();

	// Visit all selection sets and assemble the info into the structures
	GlobalSelectionSetManager().foreachSelectionSet([&](const ISelectionSetPtr& set)
	{
		// Get all nodes of this selection set and store them for later use
		_exportInfo.push_back(SelectionSetExportInfo());

		_exportInfo.back().set = set;
		_exportInfo.back().nodes = set->getNodes();
	});
}

void SelectionSetInfoFileModule::onSavePrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum)
{
	// Determine the item index for the selection set index mapping
	std::for_each(_exportInfo.begin(), _exportInfo.end(), [&](SelectionSetExportInfo& info)
	{
		if (info.nodes.find(node) != info.nodes.end())
		{
			info.nodeIndices.insert(map::NodeIndexPair(entityNum, primitiveNum));
		}
	});
}

void SelectionSetInfoFileModule::onSaveEntity(const scene::INodePtr& node, std::size_t entityNum)
{
	// Determine the item index for the selection set index mapping
	std::for_each(_exportInfo.begin(), _exportInfo.end(), [&](SelectionSetExportInfo& info)
	{
		if (info.nodes.find(node) != info.nodes.end())
		{
			info.nodeIndices.insert(map::NodeIndexPair(entityNum, EMPTY_PRIMITVE_NUM));
		}
	});
}

void SelectionSetInfoFileModule::writeBlocks(std::ostream& stream)
{
	// Selection Set output
	stream << "\t" << SELECTION_SETS << std::endl;

	stream << "\t{" << std::endl;

	std::size_t selectionSetCount = 0;

	std::for_each(_exportInfo.begin(), _exportInfo.end(), [&](SelectionSetExportInfo& info)
	{
		std::string indices = "";

		std::for_each(info.nodeIndices.begin(), info.nodeIndices.end(),
			[&](const map::NodeIndexPair& pair)
		{
			if (pair.second == EMPTY_PRIMITVE_NUM)
			{
				// only entity number
				indices += "( " + string::to_string(pair.first) + " ) ";
			}
			else
			{
				// entity & primitive number
				indices += "( " + string::to_string(pair.first) + " " + string::to_string(pair.second) + " ) ";
			}
		});

		// Make sure to escape the quotes of the set name, use the XML quote entity
		stream << "\t\t" << SELECTION_SET << " " << selectionSetCount++
			<< " { \"" << string::replace_all_copy(info.set->getName(), "\"", "&quot;") << "\" } "
			<< " { " << indices << " } "
			<< std::endl;
	});

	stream << "\t}" << std::endl;

	rMessage() << _exportInfo.size() << " selection sets exported." << std::endl;
}

void SelectionSetInfoFileModule::onInfoFileSaveFinished()
{
	_exportInfo.clear();
}

void SelectionSetInfoFileModule::onInfoFileLoadStart()
{
	_importInfo.clear();
}

bool SelectionSetInfoFileModule::canParseBlock(const std::string& blockName)
{
	return blockName == SELECTION_SETS;
}

void SelectionSetInfoFileModule::parseBlock(const std::string& blockName, parser::DefTokeniser& tok)
{
	if (blockName != SELECTION_SETS) return;

	// SelectionSet 2 { "Stairs" }  { (0 4076) (0 4077) (0 4078) (0 4079) (0 4309) (2) } 

	// The opening brace
	tok.assertNextToken("{");

	while (tok.hasMoreTokens())
	{
		std::string token = tok.nextToken();

		if (token == SELECTION_SET)
		{
			// Create a new SelectionSet info structure
			_importInfo.push_back(SelectionSetImportInfo());

			std::size_t selectionSetIndex = string::convert<std::size_t>(tok.nextToken());

			rMessage() << "Parsing Selection Set #" << selectionSetIndex << std::endl;

			tok.assertNextToken("{");

			// Parse the name, replacing the &quot; placeholder with a proper quote
			_importInfo.back().name = string::replace_all_copy(tok.nextToken(), "&quot;", "\"");

			tok.assertNextToken("}");

			tok.assertNextToken("{");

			while (tok.hasMoreTokens())
			{
				std::string nextToken = tok.nextToken();

				if (nextToken == "}") break;

				// If it's not a closing curly brace, it must be an opening parenthesis
				if (nextToken != "(")
				{
					throw parser::ParseException("InfoFile: Assertion failed: Required \"("
						"\", found \"" + nextToken + "\"");
				}

				// Expect one or two numbers now
				std::size_t entityNum = string::convert<std::size_t>(tok.nextToken());

				nextToken = tok.nextToken();

				if (nextToken == ")")
				{
					// Just the entity number, no primitive number
					_importInfo.back().nodeIndices.insert(
						map::NodeIndexPair(entityNum, EMPTY_PRIMITVE_NUM));
				}
				else
				{
					// Primitive number is provided as well
					std::size_t primitiveNum = string::convert<std::size_t>(nextToken);

					// No more than 2 numbers are supported, so assume a closing parenthesis now
					tok.assertNextToken(")");

					_importInfo.back().nodeIndices.insert(
						map::NodeIndexPair(entityNum, primitiveNum));
				}
			}
		}

		if (token == "}") break;
	}
}

void SelectionSetInfoFileModule::applyInfoToScene(const scene::IMapRootNodePtr& root, const map::NodeIndexMap& nodeMap)
{
	// Remove all selection sets, there shouldn't be any left at this point
	GlobalSelectionSetManager().deleteAllSelectionSets();

	// Re-construct the selection sets
	std::for_each(_importInfo.begin(), _importInfo.end(), [&](const SelectionSetImportInfo& info)
	{
		ISelectionSetPtr set = GlobalSelectionSetManager().createSelectionSet(info.name);

		std::size_t failedNodes = 0;

		std::for_each(info.nodeIndices.begin(), info.nodeIndices.end(),
			[&](const map::NodeIndexPair& indexPair)
		{
			map::NodeIndexMap::const_iterator i = nodeMap.find(indexPair);

			if (i != nodeMap.end())
			{
				set->addNode(i->second);
			}
			else
			{
				failedNodes++;
			}
		});

		if (failedNodes > 0)
		{
			rWarning() << "Couldn't resolve " << failedNodes << " nodes in selection set " << set->getName() << std::endl;
		}
	});
}

void SelectionSetInfoFileModule::onInfoFileLoadFinished()
{
	_importInfo.clear();
}

}
