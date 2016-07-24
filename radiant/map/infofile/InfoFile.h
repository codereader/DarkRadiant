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

	// The internal placeholder number for "no primitive number"
	static std::size_t EMPTY_PRIMITVE_NUM;

	typedef std::map<int, std::string> LayerNameMap;

	struct SelectionSetImportInfo
	{
		// The name of this set
		std::string name;

		typedef std::pair<std::size_t, std::size_t> IndexPair;

		// The node indices, which will be resolved to nodes after import
		std::set<IndexPair> nodeIndices;
	};

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

	// Parsed selection set information
	std::vector<SelectionSetImportInfo> _selectionSetInfo;

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

	// Returns the number of selection sets
	std::size_t getSelectionSetCount() const;

	// Traversal function for the parsed selection sets
	void foreachSelectionSetInfo(const std::function<void(const SelectionSetImportInfo&)>& functor);

private:
	void parseInfoFileBody();

	// Parses the Layers section
	void parseLayerNames();

	// SelectionSet information parser
	void parseSelectionSetInfo();

	void parseNodeToLayerMapping();
};

} // namespace map
