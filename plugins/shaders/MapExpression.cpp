#include "MapExpression.h"

#include "parser/DefTokeniser.h"
#include "os/path.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>

namespace shaders
{

namespace {

/**
 * Replace backslashes with forward slashes and strip of the file extension of
 * the provided token, and store the result in the provided string.
 * 
 * @token
 * The raw texture path.
 */
std::string parseTextureName(const std::string& token)
{
	return os::standardPath(token).substr(0, token.rfind("."));
}

}

// Construct a MapExpression from the tokeniser
MapExpression::MapExpression(parser::DefTokeniser& tok)
: _value("")
{
	// Switch on the first keyword, to determine what kind of expression this
	// is.
	std::string token = boost::algorithm::to_lower_copy(tok.nextToken());
	
	if (token == "addnormals") {
		_nodeType = ADDNORMALS;
		tok.assertNextToken("(");
		_children.push_back(MapExpression(tok));
		tok.assertNextToken(",");
		_children.push_back(MapExpression(tok));
		tok.assertNextToken(")");
	}
	else if (token == "heightmap") {
		_nodeType = HEIGHTMAP;
		tok.assertNextToken("(");
		_children.push_back(MapExpression(tok));
		tok.assertNextToken(",");
		_children.push_back(MapExpression(tok)); // should be a float
		tok.assertNextToken(")");
	}
	else if (token == "makeintensity") {
		_nodeType = MAKEINTENSITY;
		tok.assertNextToken("(");
		_children.push_back(MapExpression(tok));
		tok.assertNextToken(")");
	}
	else { 
		// Leaf node. This could be a texture or a float value, which we will
		// store as a string in either case
		_nodeType = LEAF;
		_value = parseTextureName(token);
	}
}

// Flatten the expression to a texture name
std::string MapExpression::getTextureName() const {
	
	// If this expression has a texture name, return it, otherwise recursively
	// search the child tree for a texture name and return this instead.
	std::string retVal = _value;
	if (_nodeType != LEAF) {
		for (MapExpressionList::const_iterator i = _children.begin();
			 i != _children.end();
			 ++i)
		{
			std::string childName = i->getTextureName();
			if (childName != "")
				retVal = childName;
				break;
		}
	}
	
	return retVal; 
}

}
