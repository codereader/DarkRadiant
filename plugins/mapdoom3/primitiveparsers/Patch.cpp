// greebo: Specialise the boost::lexical_cast<float>() function
#define SPECIALISE_STR_TO_FLOAT

#include "Patch.h"

#include "string/convert.h"
#include "parser/DefTokeniser.h"

namespace map
{

void PatchParser::parseMatrix(parser::DefTokeniser& tok, IPatch& patch) const
{
	tok.assertNextToken("(");

	// For each row
	for (std::size_t c = 0; c < patch.getWidth(); c++)
	{
		tok.assertNextToken("(");

		// For each column
		for (std::size_t r=0; r < patch.getHeight(); r++)
		{
			tok.assertNextToken("(");

			// Parse vertex coordinates
			patch.ctrlAt(r, c).vertex[0] = string::to_float(tok.nextToken());
			patch.ctrlAt(r, c).vertex[1] = string::to_float(tok.nextToken());
			patch.ctrlAt(r, c).vertex[2] = string::to_float(tok.nextToken());

			// Parse texture coordinates
			patch.ctrlAt(r, c).texcoord[0] = string::to_float(tok.nextToken());
			patch.ctrlAt(r, c).texcoord[1] = string::to_float(tok.nextToken());

			tok.assertNextToken(")");
		}

		tok.assertNextToken(")");
	}

	tok.assertNextToken(")");
}

}
