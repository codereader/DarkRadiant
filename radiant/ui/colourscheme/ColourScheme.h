#ifndef COLOURSCHEME_H_
#define COLOURSCHEME_H_

#include <string>
#include <map>
#include "math/Vector3.h"
#include "xmlutil/Node.h"

namespace ui {

/* The ColourItem represents a single colour. This ia a simple derivative of 
 * Vector3 which provides an additional constructor to extract the colour information
 * from an XML node.
 */

class ColourItem 
: public Vector3
{
public:

	/** Default constructor. Creates a black colour.
	 */
	ColourItem() 
	: Vector3(0, 0, 0) 
	{}
	
	/** Construct a ColourItem from an XML Node.
	 */
	ColourItem(xml::Node& colourNode)
	: Vector3(colourNode.getAttributeValue("value"))
	{}	
		
};

typedef std::map<const std::string, ColourItem> ColourItemMap;

/*	A colourscheme is basically a collection of ColourItems 
 */
class ColourScheme {
	
	private:
		// The name of this scheme
		std::string _name;
		
		// The ColourItems Map
		ColourItemMap _colours;
		
		// True if the scheme must not be edited
		bool _readOnly;
		
		/* Empty Colour, this serves as return value for 
		   non-existing, but requested colours */
		ColourItem _emptyColour; 
		
	public:
		// Constructors
		ColourScheme() {};
		// Constructs a ColourScheme from a given xml::node
		ColourScheme(xml::Node& schemeNode);
		
		// Returns the list of ColourItems
		ColourItemMap& getColourMap() {
			return _colours;
		}
		
		// Returns the requested colour object
		ColourItem& getColour(const std::string& colourName);
		
		// returns the name of this colour scheme
		std::string getName() const {
			return _name;
		}
		
		// returns true if the scheme is read-only
		bool isReadOnly() const {
			return _readOnly;
		}
		
		// set the read-only status of this scheme
		void setReadOnly(const bool isReadOnly) {
			_readOnly = isReadOnly;
		}
		
		// Destructor
		~ColourScheme() {};
		
}; // class ColourScheme

} // namespace ui

#endif /*COLOURSCHEME_H_*/
