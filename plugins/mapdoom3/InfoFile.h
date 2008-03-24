#ifndef INFO_FILE_H_
#define INFO_FILE_H_

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

	// TRUE if the map info fail was found to be valid
	bool _isValid;

public:
	// Pass the input stream to the constructor
	InfoFile(std::istream& infoStream);

	// Parse the entire file
	void parse();

	const LayerNameList& getLayerNames() const;

private:
	void parseInfoFileBody();

	// Parses the Layers section
	void parseLayerNames();

	void parseNodeToLayerMapping();
};

} // namespace map

#endif /* INFO_FILE_H_ */
