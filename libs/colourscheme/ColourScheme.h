#ifndef COLOURSCHEME_H_
#define COLOURSCHEME_H_

#include <string>
#include <map>
#include "generic/vector.h"
#include "xmlutil/Node.h"
#include "gdk/gdkcolor.h"

namespace ui {

/*	The ColourItem represents a single colour
 *  This can be constructed out of a xml::node containing the colour information
 * 
 *  Can be cast into a GdkColor* and a std::string
 */
class ColourItem {
	
	private:
		// The name of this colouritem
		std::string	_name;
		
		// The internal representations of this colour
		Vector3	_colour;
		GdkColor _gdkColor;
		
	public:
		// Constructors
		ColourItem();
		ColourItem(xml::Node& colourNode);
		
		operator Vector3 ();
		operator GdkColor* ();
		operator std::string ();
		bool operator== (const ColourItem& other) const;
		
		std::string getName() const {
			return _name;
		}
		
		Vector3 getColour() const {
			return _colour;
		}
		
		// Update the internal colour objects of this class
		// expects colour values between 0...65535 (as GdkColor does)
		void setColour(const unsigned int& red, 
					   const unsigned int& green, 
					   const unsigned int& blue);
		
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
