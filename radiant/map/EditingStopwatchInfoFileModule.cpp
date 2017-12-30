#include "EditingStopwatchInfoFileModule.h"

#include "itextstream.h"
#include "EditingStopwatch.h"
#include "parser/DefTokeniser.h"
#include "string/convert.h"

namespace map
{

namespace
{
	const char* const MAP_EDIT_TIMINGS = "MapEditTimings";
	const char* const TOTAL_SECONDS_EDITED = "TotalSecondsEdited";
}

std::string EditingStopwatchInfoFileModule::getName()
{
	return "Map Edit Stopwatch";
}

void EditingStopwatchInfoFileModule::onInfoFileSaveStart()
{}

void EditingStopwatchInfoFileModule::onSavePrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum)
{}

void EditingStopwatchInfoFileModule::onSaveEntity(const scene::INodePtr& node, std::size_t entityNum)
{}

void EditingStopwatchInfoFileModule::writeBlocks(std::ostream& stream)
{
	// Block output
	stream << "\t" << MAP_EDIT_TIMINGS << std::endl;

	stream << "\t{" << std::endl;

	unsigned long secondsEdited = EditingStopwatch::GetInstanceInternal().getTotalSecondsEdited();

	// TotalSecondsEdited { 4 }
	stream << "\t\t" << TOTAL_SECONDS_EDITED << " { " << secondsEdited  << " }" << std::endl;

	stream << "\t}" << std::endl;

	rMessage() << "Map Edit Timings written." << std::endl;
}

void EditingStopwatchInfoFileModule::onInfoFileSaveFinished()
{}

void EditingStopwatchInfoFileModule::onInfoFileLoadStart()
{}

bool EditingStopwatchInfoFileModule::canParseBlock(const std::string& blockName)
{
	return blockName == MAP_EDIT_TIMINGS;
}

void EditingStopwatchInfoFileModule::parseBlock(const std::string& blockName, parser::DefTokeniser& tok)
{
	// The opening brace
	tok.assertNextToken("{");

	int blockLevel = 1;

	while (tok.hasMoreTokens() && blockLevel > 0)
	{
		std::string token = tok.nextToken();

		if (token == TOTAL_SECONDS_EDITED)
		{
			// TotalSecondsEdited { 4 }
			tok.assertNextToken("{");

			// Parse the name, replacing the &quot; placeholder with a proper quote
			unsigned long secondsEdited = string::convert<unsigned long>(tok.nextToken());

			tok.assertNextToken("}");

			rMessage() << "[InfoFile]: Parsed map editing time." << std::endl;

			// Apply the parsed value
			EditingStopwatch::GetInstanceInternal().setTotalSecondsEdited(secondsEdited);
		}
		else if (token == "{")
		{
			blockLevel++;
		}
		else if (token == "}")
		{
			blockLevel--;
		}
	}
}

void EditingStopwatchInfoFileModule::applyInfoToScene(const scene::IMapRootNodePtr & root, const NodeIndexMap & nodeMap)
{}

void EditingStopwatchInfoFileModule::onInfoFileLoadFinished()
{}

}
