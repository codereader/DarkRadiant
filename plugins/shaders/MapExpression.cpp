#include "MapExpression.h"

#include "parser/DefTokeniser.h"
#include <boost/algorithm/string/case_conv.hpp>

namespace shaders
{

// Construct a MapExpression from the tokeniser
MapExpression::MapExpression(parser::DefTokeniser& tok)
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
	else { 
		// Leaf node. This could be a texture or a float value, which we will
		// store as a string in either case
		_nodeType = LEAF;
		_value = token;
	}
}

}
