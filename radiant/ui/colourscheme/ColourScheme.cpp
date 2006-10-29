#include "ColourScheme.h"

#include "stringio.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"
#include <sstream>

namespace ui {
	
	namespace {
		const unsigned int FULL_INTENSITY = 65535;
	}

/*	Constructor
 *  Builds a new ColourItem object out of the information found in the colourNode XML node
 */
ColourItem::ColourItem(xml::Node& colourNode) {
	// Parse the "value" attribute for the vector
	std::string value = colourNode.getAttributeValue("value");
	if (string_parse_vector3( value.c_str(), _colour)) {
		//globalOutputStream() << "ColourItem " << _name.c_str() << " with value " << _colour << " found.\n";
		_gdkColor.red 	= (unsigned int) (FULL_INTENSITY*_colour[0]);
		_gdkColor.green	= (unsigned int) (FULL_INTENSITY*_colour[1]);
		_gdkColor.blue 	= (unsigned int) (FULL_INTENSITY*_colour[2]);
	}
	else {
		globalOutputStream() << "ColourSchemeManager: Invalid colour data found in ColourItem: " << value.c_str() << "\n";
		string_parse_vector3( "0 0 0", _colour);
	}
}

// Constructor without arguments
ColourItem::ColourItem():
  _colour("0 0 0") 
{
	_gdkColor.red 	= 0;
	_gdkColor.green	= 0;
	_gdkColor.blue 	= 0;
}

bool ColourItem::operator== (const ColourItem& other) const {
	return (other._colour[0] == _colour[0] && 
	        other._colour[1] == _colour[1] && 
	        other._colour[2] == _colour[2]); 
}

/*	Casting operator for use as Vector3 
 */
ColourItem::operator Vector3 () {
	return _colour;
}

/*	Casting operator for use in GtkColorButtons etc. 
 */
ColourItem::operator GdkColor* () {
	return &_gdkColor;
}

/*	Casting operator, e.g. for saving the colour into the registry 
 */
ColourItem::operator std::string () {
	return std::string(_colour);
}

/*	Updates the colour information contained in this class including the GdkColor item
 */
void ColourItem::setColour(const unsigned int& red, const unsigned int& green, const unsigned int& blue) {
	// Update the GdkColor...
	_gdkColor.red = red;
	_gdkColor.green = green;
	_gdkColor.blue = blue;
	
	// ...and the internal Vector3
	_colour[0] = float(red) / FULL_INTENSITY;
	_colour[1] = float(green) / FULL_INTENSITY;
	_colour[2] = float(blue) / FULL_INTENSITY;
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
