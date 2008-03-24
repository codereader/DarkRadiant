#ifndef INFO_FILE_H_
#define INFO_FILE_H_

#include <vector>
#include "ilayer.h"
#include "parser/DefTokeniser.h"

namespace map {

class InfoFile {
public:
	typedef std::vector<std::string> LayerNameList;

private:
	// The actual DefTokeniser to split the infoStream into pieces
	parser::BasicDefTokeniser<std::istream> _tok;

	// The list of layernames
	LayerNameList _layerNames;

	typedef std::vector<scene::LayerList> LayerLists;
	LayerLists _layerMappings;

	// The standard list (node is part of layer 0)
	scene::LayerList _standardLayerList;

	// TRUE if the map info fail was found to be valid
	bool _isValid;

	// The internal "iterator" into the NodeToLayerMapping vector
	LayerLists::const_iterator _layerMappingIterator;

public:
	// Pass the input stream to the constructor
	InfoFile(std::istream& infoStream);

	~InfoFile();

	// Parse the entire file
	void parse();

	// Get the parsed Layer list
	const LayerNameList& getLayerNames() const;

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

#endif /* INFO_FILE_H_ */
