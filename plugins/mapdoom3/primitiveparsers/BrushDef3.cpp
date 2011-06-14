#include "BrushDef3.h"

#include "imap.h"
#include "ibrush.h"
#include "parser/DefTokeniser.h"
#include "math/Matrix4.h"
#include "shaderlib.h"
#include "i18n.h"
#include <boost/format.hpp>

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

// greebo: switch off optimisations for this section - the symptom is that brushes don't get a 
// valid d value assigned after the first call to addFace() - the callback triggers a series
// of calls in the DarkRadiant main module (up to the Texture Tool), and after return the plane
// gets wrong values assigned
#if _MSC_VER >= 1600
#pragma optimize( "", off )
#endif

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

			plane.normal().x() = strToFloat(tok.nextToken());
			plane.normal().y() = strToFloat(tok.nextToken());
			plane.normal().z() = strToFloat(tok.nextToken());
			plane.dist() = -strToFloat(tok.nextToken()); // negate d

			tok.assertNextToken(")");

			// Parse TexDef
			Matrix4 texdef;
			tok.assertNextToken("(");

			tok.assertNextToken("(");
			texdef.xx() = strToFloat(tok.nextToken());
			texdef.yx() = strToFloat(tok.nextToken());
			texdef.tx() = strToFloat(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken("(");
			texdef.xy() = strToFloat(tok.nextToken());
			texdef.yy() = strToFloat(tok.nextToken());
			texdef.ty() = strToFloat(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken(")");

			// Parse Shader
			std::string shader = tok.nextToken();

			// Parse Contents Flags (and ignore them)
			tok.skipTokens(3);

			// Finally, add the new face to the brush
			/*IFace& face = */brush.addFace(plane, texdef, shader);
		}
		else {
			std::string text = (boost::format(_("BrushDef3Parser: invalid token '%s'")) % token).str();
			throw parser::ParseException(text);
		}
	}

	// Final outer "}"
	tok.assertNextToken("}");

	return node;
}

scene::INodePtr BrushDef3ParserQuake4::parse(parser::DefTokeniser& tok) const
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

			plane.normal().x() = strToFloat(tok.nextToken());
			plane.normal().y() = strToFloat(tok.nextToken());
			plane.normal().z() = strToFloat(tok.nextToken());
			plane.dist() = -strToFloat(tok.nextToken()); // negate d

			tok.assertNextToken(")");

			// Parse TexDef
			Matrix4 texdef;
			tok.assertNextToken("(");

			tok.assertNextToken("(");
			texdef.xx() = strToFloat(tok.nextToken());
			texdef.yx() = strToFloat(tok.nextToken());
			texdef.tx() = strToFloat(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken("(");
			texdef.xy() = strToFloat(tok.nextToken());
			texdef.yy() = strToFloat(tok.nextToken());
			texdef.ty() = strToFloat(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken(")");

			// Parse Shader
			std::string shader = tok.nextToken();

			// Finally, add the new face to the brush
			/*IFace& face = */brush.addFace(plane, texdef, shader);
		}
		else {
			std::string text = (boost::format(_("BrushDef3ParserQuake4: invalid token '%s'")) % token).str();
			throw parser::ParseException(text);
		}
	}

	// Final outer "}"
	tok.assertNextToken("}");

	return node;
}

#if _MSC_VER >= 1600
#pragma optimize( "", on )
#endif

} // namespace map
