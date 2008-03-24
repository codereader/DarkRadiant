#include "InfoFile.h"

#include "stream/textstream.h"
#include "string/string.h"

#include "Doom3MapFormat.h"
#include "Tokens.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>

namespace map {

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

void InfoFile::parse() {
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
			throw parser::ParseException("Map Info File Version invalid");
		}
	}
	catch (parser::ParseException e) {
        globalErrorStream() 
            << "[mapdoom3] Unable to parse info file header: " 
            << e.what() << "\n";
		_isValid = false;
        return;
    }
    catch (boost::bad_lexical_cast e) {
        globalErrorStream() 
            << "[mapdoom3] Unable to parse info file version: " 
            << e.what() << "\n";
		_isValid = false;
        return;
    }

	// The opening brace of the master block
	_tok.assertNextToken("{");
	
	parseInfoFileBody();

	// Set the layer mapping iterator to the beginning
	_layerMappingIterator = _layerMappings.begin();
}

void InfoFile::parseInfoFileBody() {
	while (_tok.hasMoreTokens()) {
		std::string token = _tok.nextToken();

		if (token == LAYERS) {
			parseLayerNames();
			continue;
		}

		if (token == NODE_TO_LAYER_MAPPING) {
			parseNodeToLayerMapping();
			continue;
		}

		if (token == "}") {
			break;
		}
	}
}

void InfoFile::parseLayerNames() {
	// The opening brace
	_tok.assertNextToken("{");
	
	while (_tok.hasMoreTokens()) {
		std::string token = _tok.nextToken();

		if (token == LAYER) {
			// Get the ID
			std::string layerIDStr = _tok.nextToken();
			int layerID = strToInt(layerIDStr);

			_tok.assertNextToken("{");

			// Assemble the name
			std::string name;

			token = _tok.nextToken();
			while (token != "}") {
				name += token;
				token = _tok.nextToken();
			}

			globalOutputStream() << "[InfoFile]: Parsed layer #" 
				<< layerID << " with name " << name.c_str() << "\n";

			_layerNames.insert(LayerNameMap::value_type(layerID, name));

			continue;
		}

		if (token == "}") {
			break;
		}
	}
}

void InfoFile::parseNodeToLayerMapping() {
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
				_layerMappings.back().insert(strToInt(nodeToken));
			}
		}

		if (token == "}") {
			break;
		}
	}
}

} // namespace map
