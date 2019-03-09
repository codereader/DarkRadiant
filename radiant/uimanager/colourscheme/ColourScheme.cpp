#include "ColourScheme.h"

#include "itextstream.h"

namespace ui
{

ColourScheme::ColourScheme() 
{}

/*	ColourScheme Constructor
 *  Builds the colourscheme structure and passes the found <colour> tags to the ColourItem constructor
 *  All the found <colour> items are stored in a vector of ColourItems
 */
ColourScheme::ColourScheme(const xml::Node& schemeNode)
{
	_readOnly = (schemeNode.getAttributeValue("readonly") == "1");

	// Select all <colour> nodes from the tree
	xml::NodeList colourNodes = schemeNode.getNamedChildren("colour");

	if (colourNodes.empty())
	{
		rMessage() << "ColourScheme: No scheme items found." << std::endl;
		return;
	}

	// Assign the name of this scheme
	_name = schemeNode.getAttributeValue("name");

	// Cycle through all found colour tags and add them to this scheme
	for (const xml::Node& colourNode : colourNodes)
	{
		std::string colourName = colourNode.getAttributeValue("name");

		_colours[colourName] = ColourItem(colourNode);
	}
}

ColourItemMap& ColourScheme::getColourMap()
{
	return _colours;
}

ColourItem& ColourScheme::getColour(const std::string& colourName)
{
	ColourItemMap::iterator it = _colours.find(colourName);

	if (it != _colours.end())
	{
		return it->second;
	}

	rMessage() << "ColourScheme: Colour " << colourName << " doesn't exist!" << std::endl;

	return _emptyColour;
}

const std::string& ColourScheme::getName() const
{
	return _name;
}

bool ColourScheme::isReadOnly() const
{
	return _readOnly;
}

void ColourScheme::setReadOnly(bool isReadOnly)
{
	_readOnly = isReadOnly;
}

void ColourScheme::mergeMissingItemsFromScheme(const ColourScheme& other)
{
	for (const ColourItemMap::value_type& otherPair : other._colours)
	{
		// Insert any ColourItems from the other mapping into this scheme
		if (_colours.find(otherPair.first) == _colours.end())
		{
			_colours.insert(otherPair);
		}
	}
}

} // namespace ui
