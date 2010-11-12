#include "BrushDef.h"

#include "imap.h"
#include "ibrush.h"
#include "parser/DefTokeniser.h"
#include "math/matrix.h"
#include "shaderlib.h"
#include "i18n.h"
#include <boost/format.hpp>

namespace map
{

const std::string& BrushDefParser::getKeyword() const
{
	static std::string _keyword("brushDef");
	return _keyword;
}

/*
// Example Primitive
{
brushDef
{
( -1216 -464 232 ) ( -1088 -464 232 ) ( -1088 -80 120 ) ( ( 0.031250 0 14 ) ( -0.000009 0.031250 4.471550 ) ) common/caulk 134217728 4 0
( -1088 -464 248 ) ( -1216 -464 248 ) ( -1216 -80 136 ) ( ( 0 -0.031373 -0.147059 ) ( 0.007812 0 0.049020 ) ) common/caulk 134217728 0 0
( -1088 -560 120 ) ( -1088 -560 136 ) ( -1088 -80 136 ) ( ( 0.031250 0 16.500000 ) ( 0 0.031250 0.250000 ) ) common/caulk 134217728 4 0
( -1088 -80 136 ) ( -1216 -80 136 ) ( -1216 -80 8 ) ( ( 0.031250 0 2 ) ( 0 0.031250 0.250000 ) ) common/caulk 134217728 4 0
( -1216 -400 136 ) ( -1216 -400 120 ) ( -1216 -80 120 ) ( ( 0.031250 0 -16.500000 ) ( 0 0.031250 0.250000 ) ) common/caulk 134217728 4 0
( -1088 -464 232 ) ( -1216 -464 232 ) ( -1216 -464 248 ) ( ( 0.031250 0 -2 ) ( 0 0.031250 0.250000 ) ) common/caulk 134217728 4 0
}
}
*/
scene::INodePtr BrushDefParser::parse(parser::DefTokeniser& tok) const
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
			// Parse three 3D points to construct a plane
			Vector3 p1(strToDouble(tok.nextToken()), strToDouble(tok.nextToken()), strToDouble(tok.nextToken()));
			tok.assertNextToken(")");
			tok.assertNextToken("(");

			Vector3 p2(strToDouble(tok.nextToken()), strToDouble(tok.nextToken()), strToDouble(tok.nextToken()));
			tok.assertNextToken(")");
			tok.assertNextToken("(");

			Vector3 p3(strToDouble(tok.nextToken()), strToDouble(tok.nextToken()), strToDouble(tok.nextToken()));
			tok.assertNextToken(")");

			// Construct the plane from the three points
			Plane3 plane(p1, p2, p3);

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

			// Parse Shader, brushDef has an implicit "textures/" not written to the map
			std::string shader = "textures/" + tok.nextToken();

			// Parse Contents Flags (and ignore them)
			tok.skipTokens(3);

			// Finally, add the new face to the brush
			IFace& face = brush.addFace(plane, texdef, shader);
		}
		else
		{
			std::string text = (boost::format(_("BrushDefParser: invalid token '%s'")) % token).str();
			throw parser::ParseException(text);
		}
	}

	// Final outer "}"
	tok.assertNextToken("}");

	return node;
}

} // namespace map
