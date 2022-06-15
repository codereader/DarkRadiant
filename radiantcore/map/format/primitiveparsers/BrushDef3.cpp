// greebo: Specialise the float casting function
#define SPECIALISE_STR_TO_FLOAT

#include "BrushDef3.h"
#include "string/convert.h"
#include "imap.h"
#include "ibrush.h"
#include "parser/DefTokeniser.h"
#include "math/Matrix4.h"
#include "math/Plane3.h"
#include "shaderlib.h"
#include "i18n.h"
#include <fmt/format.h>

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
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(node);
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

			plane.normal().x() = string::to_float(tok.nextToken());
			plane.normal().y() = string::to_float(tok.nextToken());
			plane.normal().z() = string::to_float(tok.nextToken());
			plane.dist() = -string::to_float(tok.nextToken()); // negate d

			tok.assertNextToken(")");

			// Parse TexDef
			Matrix3 texdef;
			tok.assertNextToken("(");

			tok.assertNextToken("(");
			texdef.xx() = string::to_float(tok.nextToken());
			texdef.yx() = string::to_float(tok.nextToken());
			texdef.zx() = string::to_float(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken("(");
			texdef.xy() = string::to_float(tok.nextToken());
			texdef.yy() = string::to_float(tok.nextToken());
			texdef.zy() = string::to_float(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken(")");

			// Parse Shader
			std::string shader = tok.nextToken();

			// Parse Flags (usually each brush has all faces detail or all faces structural)
			IBrush::DetailFlag flag = static_cast<IBrush::DetailFlag>(
				string::convert<std::size_t>(tok.nextToken(), IBrush::Structural));
			brush.setDetailFlag(flag);

			// Ignore the other two flags
			tok.skipTokens(2);

			// Finally, add the new face to the brush
			brush.addFace(plane, texdef, shader);
		}
		else {
			std::string text = fmt::format(_("BrushDef3Parser: invalid token '{0}'"), token);
			throw parser::ParseException(text);
		}
	}

	// Final outer "}"
	tok.assertNextToken("}");

    // Cleanup redundant face planes
    brush.removeRedundantFaces();

	return node;
}

scene::INodePtr BrushDef3ParserQuake4::parse(parser::DefTokeniser& tok) const
{
	// Create a new brush
	scene::INodePtr node = GlobalBrushCreator().createBrush();

	// Cast the node, this must succeed
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(node);
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

			plane.normal().x() = string::to_float(tok.nextToken());
			plane.normal().y() = string::to_float(tok.nextToken());
			plane.normal().z() = string::to_float(tok.nextToken());
			plane.dist() = -string::to_float(tok.nextToken()); // negate d

			tok.assertNextToken(")");

			// Parse TexDef
			Matrix3 texdef;
			tok.assertNextToken("(");

			tok.assertNextToken("(");
			texdef.xx() = string::to_float(tok.nextToken());
			texdef.yx() = string::to_float(tok.nextToken());
			texdef.zx() = string::to_float(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken("(");
			texdef.xy() = string::to_float(tok.nextToken());
			texdef.yy() = string::to_float(tok.nextToken());
			texdef.zy() = string::to_float(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken(")");

			// Parse Shader
			std::string shader = tok.nextToken();

			// Finally, add the new face to the brush
			brush.addFace(plane, texdef, shader);
		}
		else {
			std::string text = fmt::format(_("BrushDef3ParserQuake4: invalid token '{0}'"), token);
			throw parser::ParseException(text);
		}
	}

	// Final outer "}"
	tok.assertNextToken("}");

    // Cleanup redundant face planes
    brush.removeRedundantFaces();

	return node;
}

#if _MSC_VER >= 1600
#pragma optimize( "", on )
#endif

} // namespace map
