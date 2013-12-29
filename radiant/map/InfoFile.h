#pragma once

#include <map>
#include "ilayer.h"
#include "parser/DefTokeniser.h"

namespace map
{

class InfoFile
{
public:
	// Tokens / Constants
	// The version of the map info file
	static const int MAP_INFO_VERSION = 2;

	// InfoFile tokens --------------------------------------------------
	static const char* const HEADER_SEQUENCE;
	static const char* const NODE_TO_LAYER_MAPPING;
	static const char* const LAYER;
	static const char* const LAYERS;
	static const char* const NODE;
	static const char* const SELECTION_SETS;
	static const char* const SELECTION_SET;

	typedef std::map<int, std::string> LayerNameMap;

private:
	// The actual DefTokeniser to split the infoStream into pieces
	parser::BasicDefTokeniser<std::istream> _tok;

	// The list of layernames
	LayerNameMap _layerNames;

	typedef std::vector<scene::LayerList> LayerLists;
	LayerLists _layerMappings;

	// The standard list (node is part of layer 0)
	scene::LayerList _standardLayerList;

	// TRUE if the map info file was found to be valid
	bool _isValid;

	// The internal "iterator" into the NodeToLayerMapping vector
	LayerLists::const_iterator _layerMappingIterator;

public:
	// Pass the input stream to the constructor
	InfoFile(std::istream& infoStream);

	// Parse the entire file
	void parse();

	// Get the parsed Layer list
	const LayerNameMap& getLayerNames() const;

	// Returns the next layer mapping. The internal iterator is increased by this call.
	// This allows the client code to treat this class like a LayerList input stream.
	const scene::LayerList& getNextLayerMapping();

	// Returns the number of parsed layer mappings
	std::size_t getLayerMappingCount() const;

private:
	void parseInfoFileBody();

	// Parses the Layers section
	void parseLayerNames();

	void parseNodeToLayerMapping();
};

} // namespace map
