#include "ColourScheme.h"

#include "stream/textstream.h"

namespace ui {
	
/*	ColourScheme Constructor
 *  Builds the colourscheme structure and passes the found <colour> tags to the ColourItem constructor
 *  All the found <colour> items are stored in a vector of ColourItems
 */
ColourScheme::ColourScheme(xml::Node& schemeNode) {
	_readOnly = (schemeNode.getAttributeValue("readonly") == "1");
	
	// Select all <colour> nodes from the tree
	xml::NodeList colourNodes = schemeNode.getNamedChildren("colour");
	
	if (colourNodes.size() > 0) {
		// Assign the name of this scheme
		_name = schemeNode.getAttributeValue("name");
		
		// Cycle through all found colour tags and add them to this scheme
		for (unsigned int i = 0; i < colourNodes.size(); i++) {
			std::string colourName = colourNodes[i].getAttributeValue("name");
			_colours[colourName] = ColourItem(colourNodes[i]);
		}
		
	}
	else {
		globalOutputStream() << "ColourScheme: No scheme items found.\n";
	}
}

/* Returns the specified colour object
 */
ColourItem& ColourScheme::getColour(const std::string& colourName) {
	ColourItemMap::iterator it = _colours.find(colourName);
	if (it != _colours.end()) {
		return it->second;
	}
	else {
		globalOutputStream() << "ColourScheme: Colour " << colourName.c_str() << " doesn't exist!\n";
		return _emptyColour;
	}
}

} // namespace ui
