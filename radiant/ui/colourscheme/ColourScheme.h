#ifndef COLOURSCHEME_H_
#define COLOURSCHEME_H_

#include <string>
#include <map>
#include "generic/vector.h"
#include "xmlutil/Node.h"

namespace ui {

/*	The ColourItem represents a single colour
 *  This can be constructed out of a xml::node containing the colour information
 * 
 *  Can be cast into a GdkColor* and a std::string
 */
class ColourItem {
	
	private:
		// The internal representations of this colour
		Vector3	_colour;
		
	public:
		// Constructors
		ColourItem() : _colour("0 0 0") {}
		ColourItem(xml::Node& colourNode);
		
		operator Vector3 ();
		operator std::string ();
		bool operator== (const ColourItem& other) const;
		bool operator!= (const ColourItem& other) const;
		
		Vector3 getColour() const {
			return _colour;
		}
		
		// Update the internal colour objects of this class
		void setColour(const float& red, const float& green, const float& blue);
		
		// Destructor
		~ColourItem() {}
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
		
	public:
		// Constructors
		ColourScheme() {};
		// Constructs a ColourScheme from a given xml::node
		ColourScheme(xml::Node& schemeNode);
		
		// Returns the list of ColourItems
		ColourItemMap& getColourMap() {
			return _colours;
		}
		
		// Checks whether this colour exists in this scheme
		bool colourExists(const std::string& colourName);
		
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
