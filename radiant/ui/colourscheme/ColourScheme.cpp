#include "ColourScheme.h"

#include "stream/textstream.h"

namespace ui {
	

/*	Constructor
 *  Builds a new ColourItem object out of the information found in the colourNode XML node
 */
ColourItem::ColourItem(xml::Node& colourNode)
: _colour(colourNode.getAttributeValue("value")) // initialise Vector3 from string
{
}

bool ColourItem::operator== (const ColourItem& other) const {
	return (other._colour[0] == _colour[0] && 
	        other._colour[1] == _colour[1] && 
	        other._colour[2] == _colour[2]); 
}

bool ColourItem::operator!= (const ColourItem& other) const {
	return (other._colour[0] != _colour[0] || 
	        other._colour[1] != _colour[1] || 
	        other._colour[2] != _colour[2]); 
}

/*	Casting operator for use as Vector3 
 */
ColourItem::operator Vector3 () {
	return _colour;
}

/*	Casting operator, e.g. for saving the colour into the registry 
 */
ColourItem::operator std::string () {
	return std::string(_colour);
}

/*	Updates the colour information contained in this class
 */
void ColourItem::setColour(const float& red, const float& green, const float& blue) {
	_colour[0] = red;
	_colour[1] = green;
	_colour[2] = blue;
}

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

/*	Checks whether the specified colour exists in this scheme
 */
bool ColourScheme::colourExists(const std::string& colourName) {
	ColourItemMap::iterator it = _colours.find(colourName);
   	return (it != _colours.end());
}

/* Returns the specified colour object
 */
ColourItem& ColourScheme::getColour(const std::string& colourName) {
	if (colourExists(colourName)) {
		return _colours[colourName];
	}
	else {
		globalOutputStream() << "ColourScheme: Colour " << colourName.c_str() << " doesn't exist!\n";
	}
}

} // namespace ui
