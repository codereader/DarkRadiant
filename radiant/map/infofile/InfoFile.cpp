#include "InfoFile.h"

#include "itextstream.h"
#include "imapinfofile.h"
#include "string/convert.h"

#include "i18n.h"
#include "string/split.h"

namespace map
{

const char* const InfoFile::HEADER_SEQUENCE = "DarkRadiant Map Information File Version";

// Pass the input stream to the constructor
InfoFile::InfoFile(std::istream& infoStream, const scene::IMapRootNodePtr& root, const NodeIndexMap& nodeMap) :
	_tok(infoStream),
	_isValid(true),
	_root(root),
	_nodeMap(nodeMap)
{}

void InfoFile::parse()
{
	// Initialise the modules
	GlobalMapInfoFileManager().foreachModule([](IMapInfoFileModule& module)
	{
		module.onInfoFileLoadStart();
	});

	// Parse the Header
	parseInfoFileHeader();

	// Parse the blocks
	parseInfoFileBody();

	// Apply the parsed info to the scene
	GlobalMapInfoFileManager().foreachModule([&](IMapInfoFileModule& module)
	{
		module.applyInfoToScene(_root, _nodeMap);
	});

	// De-initialise the modules
	GlobalMapInfoFileManager().foreachModule([&](IMapInfoFileModule& module)
	{
		module.onInfoFileLoadFinished();
	});
}

void InfoFile::parseInfoFileHeader()
{
	try
	{
		std::vector<std::string> parts;
		string::split(parts, HEADER_SEQUENCE, " ");

		// Parse the string "DarkRadiant Map Information File Version"
		for (std::size_t i = 0; i < parts.size(); i++)
		{
			_tok.assertNextToken(parts[i]);
		}

		float version = std::stof(_tok.nextToken());

		if (version != MAP_INFO_VERSION)
		{
			_isValid = false;
			throw parser::ParseException(_("Map Info File Version invalid"));
		}
	}
	catch (parser::ParseException& e)
	{
		rError() << "[InfoFile] Unable to parse info file header: " << e.what() << std::endl;
		_isValid = false;
		return;
	}
	catch (std::invalid_argument& e)
	{
		rError() << "[InfoFile] Unable to parse info file version: " << e.what() << std::endl;
		_isValid = false;
		return;
	}
}

void InfoFile::parseInfoFileBody()
{
	// The opening brace of the master block
	_tok.assertNextToken("{");

	while (_tok.hasMoreTokens())
	{
		std::string token = _tok.nextToken();

		bool blockParsed = false;

		// Send each block to the modules that are able to load it
		GlobalMapInfoFileManager().foreachModule([&](IMapInfoFileModule& module)
		{
			if (!blockParsed && module.canParseBlock(token))
			{
				module.parseBlock(token, _tok);
				blockParsed = true;
			}
		});

		if (blockParsed)
		{
			continue; // block was processed by a module
		}

		if (token == "}")
		{
			break;
		}

		// Unknown token, try to ignore that block
		rWarning() << "Unknown keyword " << token << " encountered, will try to ignore this block." << std::endl;

		// We can only ignore a block if there is a block beginning curly brace
		_tok.assertNextToken("{");

		// Ignore the block
		int depth = 1;

		while (_tok.hasMoreTokens() && depth > 0)
		{
			std::string token2 = _tok.nextToken();

			if (token2 == "{") 
			{
				depth++;
			}
			else if (token2 == "}") 
			{
				depth--;
			}
		}
	}
}

} // namespace map
