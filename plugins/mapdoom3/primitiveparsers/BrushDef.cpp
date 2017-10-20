// greebo: Specialise the float casting function
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
#include <fmt/format.h>

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
			std::string text = fmt::format(_("BrushDefParser: invalid token '{0}'"), token);
			throw parser::ParseException(text);
		}
	}

	// Final outer "}"
	tok.assertNextToken("}");

	return node;
}

// Legacy brushDef format
const std::string& LegacyBrushDefParser::getKeyword() const
{
	static std::string _keyword("("); // The Q3 map reader will just encounter this opening brace
	return _keyword;
}

/*
Example Primitive:

// brush 0
{
( 2224 3952 0 ) ( 1424 3952 0 ) ( 1424 3808 0 ) common/caulk 192 192 0 0.125000 0.125000 0 4 0
( 1424 3808 8 ) ( 1424 3952 8 ) ( 2224 3952 8 ) common/caulk 0 0 0 0.500000 0.500000 0 0 0
( 1416 3776 8 ) ( 2216 3776 8 ) ( 2216 3776 0 ) common/caulk 192 64 0 0.125000 0.125000 0 4 0
( 2944 3808 8 ) ( 2944 3952 8 ) ( 2944 3952 0 ) common/caulk -192 64 0 0.125000 0.125000 0 4 0
( 2224 3968 8 ) ( 1424 3968 8 ) ( 1424 3968 0 ) common/caulk 192 64 0 0.125000 0.125000 0 4 0
( 1340 3952 8 ) ( 1340 3808 8 ) ( 1340 3808 0 ) common/caulk -192 64 0 0.125000 0.125000 0 4 0
}

*/
scene::INodePtr LegacyBrushDefParser::parse(parser::DefTokeniser& tok) const
{
	// Create a new brush
	scene::INodePtr node = GlobalBrushCreator().createBrush();

	// Cast the node, this must succeed
	IBrushNodePtr brushNode = std::dynamic_pointer_cast<IBrushNode>(node);
	assert(brushNode != NULL);

	IBrush& brush = brushNode->getIBrush();

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

            // Parse Shader, brushDef has an implicit "textures/" not written to the map
			std::string shader = GlobalTexturePrefix_get() + tok.nextToken();

			// Parse texture (shift rotation scale)
            float shiftS = string::to_float(tok.nextToken());
            float shiftT = string::to_float(tok.nextToken());

            float rotation = string::to_float(tok.nextToken());

            float scaleS = string::to_float(tok.nextToken());
            float scaleT = string::to_float(tok.nextToken());

            Matrix4 texdef = getTexDef(shiftS, shiftT, rotation, scaleS, scaleT);

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
			std::string text = fmt::format(_("BrushDefParser: invalid token '{0}'"), token);
			throw parser::ParseException(text);
		}
	}

	return node;
}

Matrix4 LegacyBrushDefParser::getTexDef(float shiftS, float shiftT, float rotation, float scaleS, float scaleT)
{
	Matrix4 transform;
	double inverse_scale[2];

    // I don't care about the correct texture scale, let's just load the matrix
    static float DUMMY_WIDTH = 128;
    static float DUMMY_HEIGHT = 128;

	// transform to texdef shift/scale/rotate
	inverse_scale[0] = 1 / (scaleS * DUMMY_WIDTH);
	inverse_scale[1] = 1 / (scaleT * -DUMMY_HEIGHT);
	transform[12] = shiftS / DUMMY_WIDTH;
	transform[13] = -shiftT / -DUMMY_HEIGHT;

	double c = cos(degrees_to_radians(-rotation));
	double s = sin(degrees_to_radians(-rotation));

	transform[0] = c * inverse_scale[0];
	transform[1] = s * inverse_scale[1];
	transform[4] = -s * inverse_scale[0];
	transform[5] = c * inverse_scale[1];
	transform[2] = transform[3] = transform[6] = transform[7] = transform[8] = transform[9] = transform[11] = transform[14] = 0;
	transform[10] = transform[15] = 1;

	return transform;
}

} // namespace map
