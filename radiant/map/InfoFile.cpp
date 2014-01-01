#include "InfoFile.h"

#include <limits>
#include "itextstream.h"
#include "string/convert.h"

#include "i18n.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>

namespace map
{

const char* const InfoFile::HEADER_SEQUENCE = "DarkRadiant Map Information File Version";
const char* const InfoFile::NODE_TO_LAYER_MAPPING = "NodeToLayerMapping";
const char* const InfoFile::LAYER = "Layer";
const char* const InfoFile::LAYERS = "Layers";
const char* const InfoFile::NODE = "Node";
const char* const InfoFile::SELECTION_SETS = "SelectionSets";
const char* const InfoFile::SELECTION_SET = "SelectionSet";

std::size_t InfoFile::EMPTY_PRIMITVE_NUM = std::numeric_limits<std::size_t>::max();

// Pass the input stream to the constructor
InfoFile::InfoFile(std::istream& infoStream) :
	_tok(infoStream),
	_isValid(true)
{
	_standardLayerList.insert(0);
}

const InfoFile::LayerNameMap& InfoFile::getLayerNames() const {
	return _layerNames;
}

std::size_t InfoFile::getLayerMappingCount() const {
	return _layerMappings.size();
}

const scene::LayerList& InfoFile::getNextLayerMapping() {
	// Check if we have a valid infofile
	if (!_isValid) {
		return _standardLayerList;
	}

	// Check if the node index is out of bounds
	if (_layerMappingIterator == _layerMappings.end()) {
		return _standardLayerList;
	}

	// Return the current list and increase the iterator afterwards
	return *(_layerMappingIterator++);
}

void InfoFile::parse()
{
	// parse the header
	try {
		std::vector<std::string> parts;
		boost::algorithm::split(parts, HEADER_SEQUENCE, boost::algorithm::is_any_of(" "));

		// Parse the string "DarkRadiant Map Information File Version"
		for (std::size_t i = 0; i < parts.size(); i++) {
			_tok.assertNextToken(parts[i]);
		}

		float version = boost::lexical_cast<float>(_tok.nextToken());

		if (version != MAP_INFO_VERSION) {
			_isValid = false;
			throw parser::ParseException(_("Map Info File Version invalid"));
		}
	}
	catch (parser::ParseException& e) {
        rError()
            << "[InfoFile] Unable to parse info file header: "
			<< e.what() << std::endl;
		_isValid = false;
        return;
    }
    catch (boost::bad_lexical_cast& e) {
        rError()
            << "[InfoFile] Unable to parse info file version: "
			<< e.what() << std::endl;
		_isValid = false;
        return;
    }

	// The opening brace of the master block
	_tok.assertNextToken("{");

	parseInfoFileBody();

	// Set the layer mapping iterator to the beginning
	_layerMappingIterator = _layerMappings.begin();
}

void InfoFile::parseInfoFileBody()
{
	while (_tok.hasMoreTokens())
	{
		std::string token = _tok.nextToken();

		if (token == LAYERS)
		{
			parseLayerNames();
			continue;
		}

		if (token == NODE_TO_LAYER_MAPPING)
		{
			parseNodeToLayerMapping();
			continue;
		}

		if (token == SELECTION_SETS)
		{
			parseSelectionSetInfo();
			continue;
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
			std::string token = _tok.nextToken();

			if (token == "{") 
			{
				depth++;
			}
			else if (token == "}") 
			{
				depth--;
			}
		}
	}
}

void InfoFile::parseLayerNames()
{
	// The opening brace
	_tok.assertNextToken("{");

	while (_tok.hasMoreTokens()) {
		std::string token = _tok.nextToken();

		if (token == LAYER) {
			// Get the ID
			std::string layerIDStr = _tok.nextToken();
			int layerID = string::convert<int>(layerIDStr);

			_tok.assertNextToken("{");

			// Assemble the name
			std::string name;

			token = _tok.nextToken();
			while (token != "}") {
				name += token;
				token = _tok.nextToken();
			}

			rMessage() << "[InfoFile]: Parsed layer #"
				<< layerID << " with name " << name << std::endl;

			_layerNames.insert(LayerNameMap::value_type(layerID, name));

			continue;
		}

		if (token == "}") {
			break;
		}
	}
}

void InfoFile::parseNodeToLayerMapping()
{
	// The opening brace
	_tok.assertNextToken("{");

	while (_tok.hasMoreTokens()) {
		std::string token = _tok.nextToken();

		if (token == NODE) {
			_tok.assertNextToken("{");

			// Create a new LayerList
			_layerMappings.push_back(scene::LayerList());

			while (_tok.hasMoreTokens()) {
				std::string nodeToken = _tok.nextToken();

				if (nodeToken == "}") {
					break;
				}

				// Add the ID to the list
				_layerMappings.back().insert(string::convert<int>(nodeToken));
			}
		}

		if (token == "}") {
			break;
		}
	}
}

void InfoFile::parseSelectionSetInfo()
{
	_selectionSetInfo.clear();

	// SelectionSet 2 { "Stairs" }  { (0 4076) (0 4077) (0 4078) (0 4079) (0 4309) (2) } 

	// The opening brace
	_tok.assertNextToken("{");

	while (_tok.hasMoreTokens())
	{
		std::string token = _tok.nextToken();

		if (token == SELECTION_SET)
		{
			// Create a new SelectionSet info structure
			_selectionSetInfo.push_back(SelectionSetImportInfo());

			std::size_t selectionSetIndex = string::convert<std::size_t>(_tok.nextToken());

			rMessage() << "Parsing Selection Set #" << selectionSetIndex << std::endl;

			_tok.assertNextToken("{");

			// Parse the name, replacing the &quot; placeholder with a proper quote
			_selectionSetInfo.back().name = boost::algorithm::replace_all_copy(_tok.nextToken(), "&quot;", "\"");

			_tok.assertNextToken("}");

			_tok.assertNextToken("{");

			while (_tok.hasMoreTokens())
			{
				std::string nextToken = _tok.nextToken();

				if (nextToken == "}") break;

				// If it's not a closing curly brace, it must be an opening parenthesis
				if (nextToken != "(")
				{
					throw parser::ParseException("InfoFile: Assertion failed: Required \"("
						"\", found \"" + nextToken + "\"");
				}

				// Expect one or two numbers now
				std::size_t entityNum = string::convert<std::size_t>(_tok.nextToken());

				nextToken = _tok.nextToken();

				if (nextToken == ")")
				{
					// Just the entity number, no primitive number
					_selectionSetInfo.back().nodeIndices.insert(
						SelectionSetImportInfo::IndexPair(entityNum, EMPTY_PRIMITVE_NUM));
				}
				else
				{
					// Primitive number is provided as well
					std::size_t primitiveNum = string::convert<std::size_t>(nextToken);

					// No more than 2 numbers are supported, so assume a closing parenthesis now
					_tok.assertNextToken(")");

					_selectionSetInfo.back().nodeIndices.insert(
						SelectionSetImportInfo::IndexPair(entityNum, primitiveNum));
				}
			}
		}

		if (token == "}") break;
	}
}

std::size_t InfoFile::getSelectionSetCount() const
{
	return _selectionSetInfo.size();
}

// Traversal function for the parsed selection sets
void InfoFile::foreachSelectionSetInfo(const std::function<void(const SelectionSetImportInfo&)>& functor)
{
	std::for_each(_selectionSetInfo.begin(), _selectionSetInfo.end(), functor);
}

} // namespace map
