#include "Patch.h"

#include "string/string.h"
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
			patch.ctrlAt(r, c).vertex[0] = strToDouble(tok.nextToken());
			patch.ctrlAt(r, c).vertex[1] = strToDouble(tok.nextToken());
			patch.ctrlAt(r, c).vertex[2] = strToDouble(tok.nextToken());

			// Parse texture coordinates
			patch.ctrlAt(r, c).texcoord[0] = strToDouble(tok.nextToken());
			patch.ctrlAt(r, c).texcoord[1] = strToDouble(tok.nextToken());

			tok.assertNextToken(")");
		}

		tok.assertNextToken(")");
	}

	tok.assertNextToken(")");
}

}
