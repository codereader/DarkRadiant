#pragma once

#include <map>
#include "icolourscheme.h"
#include "string/convert.h"
#include "xmlutil/Node.h"

namespace colours
{

/* The ColourItem represents a single colour. This ia a simple derivative of
 * Vector3 which provides an additional constructor to extract the colour information
 * from an XML node.
 */

class ColourItem : 
	public IColourItem
{
private:
    Vector3 _colour;

public:
    /** Default constructor. Creates a black colour.
     */
    ColourItem() : 
        _colour(0, 0, 0)
    {}

    // Copy ctor
    ColourItem(const IColourItem& other) :
        _colour(other.getColour())
    {}

    ColourItem& operator=(const IColourItem& other)
    {
        _colour = other.getColour();
        return *this;
    }

    /** Construct a ColourItem from an XML Node.
     */
    ColourItem(const xml::Node& colourNode) : 
        _colour(string::convert<Vector3>(colourNode.getAttributeValue("value")))
    {}

    const Vector3& getColour() const override
    {
        return _colour;
    }

    Vector3& getColour() override
    {
        return _colour;
    }

};

typedef std::map<std::string, ColourItem> ColourItemMap;

/*  A colourscheme is basically a collection of ColourItems
 */
class ColourScheme :
    public IColourScheme
{
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
	ColourScheme();

    // Constructs a ColourScheme from a given xml::node
    ColourScheme(const xml::Node& schemeNode);

    void foreachColour(const std::function<void(const std::string& name, IColourItem& colour)>& functor) override;
    void foreachColour(const std::function<void(const std::string& name, const IColourItem& colour)>& functor) const override;

    // Returns the requested colour object
    ColourItem& getColour(const std::string& colourName) override;

    // returns the name of this colour scheme
	const std::string& getName() const override;

    // returns true if the scheme is read-only
	bool isReadOnly() const override;

    // set the read-only status of this scheme
	void setReadOnly(bool isReadOnly) override;

	// Tries to add any missing items from the given scheme into this one
	void mergeMissingItemsFromScheme(const IColourScheme& other) override;
};

} // namespace
