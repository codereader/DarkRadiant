// greebo: Specialise the boost::lexical_cast<float>() function
#define SPECIALISE_STR_TO_FLOAT

#include "BrushDef.h"

#include "string/convert.h"
#include "imap.h"
#include "ibrush.h"
#include "parser/DefTokeniser.h"
#include "math/Matrix4.h"
#include "math/Plane3.h"
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
			double x = string::to_float(tok.nextToken());
			double y = string::to_float(tok.nextToken());
			double z = string::to_float(tok.nextToken());
			Vector3 p1(x, y, z);

			tok.assertNextToken(")");
			tok.assertNextToken("(");

			x = string::to_float(tok.nextToken());
			y = string::to_float(tok.nextToken());
			z = string::to_float(tok.nextToken());
			Vector3 p2(x, y, z);

			tok.assertNextToken(")");
			tok.assertNextToken("(");

			x = string::to_float(tok.nextToken());
			y = string::to_float(tok.nextToken());
			z = string::to_float(tok.nextToken());
			Vector3 p3(x, y, z);

			tok.assertNextToken(")");

			// Construct the plane from the three points
			Plane3 plane(p3, p2, p1);

			// Parse TexDef
			Matrix4 texdef;
			tok.assertNextToken("(");

			tok.assertNextToken("(");
			texdef.xx() = string::to_float(tok.nextToken());
			texdef.yx() = string::to_float(tok.nextToken());
			texdef.tx() = string::to_float(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken("(");
			texdef.xy() = string::to_float(tok.nextToken());
			texdef.yy() = string::to_float(tok.nextToken());
			texdef.ty() = string::to_float(tok.nextToken());
			tok.assertNextToken(")");

			tok.assertNextToken(")");

			// Parse Shader, brushDef has an implicit "textures/" not written to the map
			std::string shader = GlobalTexturePrefix_get() + tok.nextToken();

			// Parse Flags (usually each brush has all faces detail or all faces structural)
			IBrush::DetailFlag flag = static_cast<IBrush::DetailFlag>(
				string::convert<std::size_t>(tok.nextToken(), IBrush::Structural));
			brush.setDetailFlag(flag);

			// Ignore the other two flags
			tok.skipTokens(2);

			// Finally, add the new face to the brush
			/*IFace& face = */brush.addFace(plane, texdef, shader);
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
