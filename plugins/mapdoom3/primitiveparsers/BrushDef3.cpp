#include "BrushDef3.h"

#include "imap.h"
#include "ibrush.h"
#include "parser/DefTokeniser.h"
#include "math/matrix.h"
#include "shaderlib.h"

namespace map
{

const std::string& BrushDef3Parser::getKeyword() const
{
	static std::string _keyword("brushDef3");
	return _keyword;
}

/* 
// Example Primitive
{
brushDef3
{
( 0 0 1 -604 ) ( ( 0.015625 0 255.9375 ) ( 0 0.015625 0 ) ) "textures/darkmod/stone/brick/blocks_brown" 0 0 0
( 0 1 0 -1528 ) ( ( 0.015625 0 0 ) ( 0 0.015625 1 ) ) "textures/darkmod/stone/brick/blocks_brown" 0 0 0
( 1 0 0 -1312 ) ( ( 0.015625 0 255.9375 ) ( 0 0.015625 1 ) ) "textures/darkmod/stone/brick/blocks_brown" 0 0 0
( 0 0 -1 -0 ) ( ( 0.015625 0 255.9375 ) ( 0 0.015625 0 ) ) "textures/darkmod/stone/brick/blocks_brown" 0 0 0
( -1 0 0 -1264 ) ( ( 0.015625 0 0.0625 ) ( 0 0.015625 1 ) ) "textures/darkmod/stone/brick/blocks_brown" 0 0 0
( -0 -1 -0 1524 ) ( ( 0.015625 0 0 ) ( 0 0.015625 1 ) ) "textures/darkmod/stone/brick/blocks_brown" 0 0 0
}
}
*/
scene::INodePtr BrushDef3Parser::parse(parser::DefTokeniser& tok) const
{
	// Create a new brush
	scene::INodePtr node = GlobalBrushCreator().createBrush();

	// Cast the node, this must succeed
	IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(node);
	assert(brushNode != NULL);

	IBrush& brush = brushNode->getIBrush();

	tok.assertNextToken("{");

	// Parse face tokens until a closing brace is encountered
	while (1)
	{
		std::string token = tok.nextToken();

		// Token should be either a "(" (start of face) or "}" (end of brush)
		if (token == "}") 
		{
			break; // end of brush
		}
		else if (token == "(") // FACE
		{ 
			// Construct a plane and parse its values
			Plane3 plane;

			plane.a = strToDouble(tok.nextToken());
			plane.b = strToDouble(tok.nextToken());
			plane.c = strToDouble(tok.nextToken());
			plane.d = -strToDouble(tok.nextToken()); // negate d

			tok.assertNextToken(")");

			// Parse TexDef
			Matrix4 texdef;
			tok.assertNextToken("(");

			tok.assertNextToken("(");
			texdef.xx() = strToDouble(tok.nextToken());
			texdef.yx() = strToDouble(tok.nextToken());
			texdef.tx() = strToDouble(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken("(");
			texdef.xy() = strToDouble(tok.nextToken());
			texdef.yy() = strToDouble(tok.nextToken());
			texdef.ty() = strToDouble(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken(")");

			// Parse Shader
			std::string shader = tok.nextToken();

			// Parse Contents Flags (and ignore them)
			tok.nextToken();
			tok.nextToken();
			tok.nextToken();

			// Finally, add the new face to the brush
			IFace& face = brush.addFace(plane, texdef, shader);
		}
		else {
			throw parser::ParseException(
				"BrushTokenImporter: invalid token '" + token + "'"
			);
		}
	}

	// Final outer "}"
	tok.assertNextToken("}");

	return node;
}

} // namespace map
