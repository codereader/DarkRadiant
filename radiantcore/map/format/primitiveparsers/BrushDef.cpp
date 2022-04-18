// greebo: Specialise the float casting function
#define SPECIALISE_STR_TO_FLOAT

#include "BrushDef.h"

#include "../Quake3Utils.h"
#include "string/convert.h"
#include "imap.h"
#include "ibrush.h"
#include "parser/DefTokeniser.h"
#include "math/Matrix4.h"
#include "math/Plane3.h"
#include "math/Vector3.h"
#include "shaderlib.h"
#include "texturelib.h"
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
// Example Primitive:

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

			// Parse Shader, brushDef has an implicit "textures/" not written to the map
			std::string shader = GlobalTexturePrefix_get() + tok.nextToken();

			// Parse Flags (usually each brush has all faces detail or all faces structural)
			IBrush::DetailFlag flag = static_cast<IBrush::DetailFlag>(
				string::convert<std::size_t>(tok.nextToken(), IBrush::Structural));
			brush.setDetailFlag(flag);

			// Ignore the other two flags
			tok.skipTokens(2);

			// Finally, add the new face to the brush
			brush.addFace(plane, texdef, shader);
		}
		else
		{
			std::string text = fmt::format(_("BrushDefParser: invalid token '{0}'"), token);
			throw parser::ParseException(text);
		}
	}

	// Final outer "}"
	tok.assertNextToken("}");

    // Cleanup redundant face planes
    brush.removeRedundantFaces();

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

			// Parse texdef (shift rotation scale)
            ShiftScaleRotation ssr;

            ssr.shift[0] = string::to_float(tok.nextToken());
            ssr.shift[1] = string::to_float(tok.nextToken());

            ssr.rotate = string::to_float(tok.nextToken());

            ssr.scale[0] = string::to_float(tok.nextToken());
            ssr.scale[1] = string::to_float(tok.nextToken());

            if (ssr.scale[0] == 0)
            {
                ssr.scale[0] = 0.5;
            }

            if (ssr.scale[1] == 0)
            {
                ssr.scale[1] = 0.5;
            }

            auto texdef = calculateTextureMatrix(shader, plane.normal(), ssr);

			// Parse Flags (usually each brush has all faces detail or all faces structural)
			auto flag = static_cast<IBrush::DetailFlag>(
				string::convert<std::size_t>(tok.nextToken(), IBrush::Structural));
			brush.setDetailFlag(flag);

			// Ignore the other two flags
			tok.skipTokens(2);

			// Finally, add the new face to the brush
			brush.addFace(plane, texdef, shader);
		}
		else
		{
			std::string text = fmt::format(_("BrushDefParser: invalid token '{0}'"), token);
			throw parser::ParseException(text);
		}
	}

    // Cleanup redundant face planes
    brush.removeRedundantFaces();

	return node;
}

Matrix3 LegacyBrushDefParser::calculateTextureMatrix(const std::string& shader, const Vector3& normal, const ShiftScaleRotation& ssr)
{
	float imageWidth = 128;
	float imageHeight = 128;

	auto texture = GlobalMaterialManager().getMaterial(shader)->getEditorImage();

	if (texture)
    {
        imageWidth = static_cast<float>(texture->getWidth());
        imageHeight = static_cast<float>(texture->getHeight());
	}

	if (imageWidth == 0 || imageHeight == 0)
    {
		rError() << "LegacyBrushDefParser: Failed to load image: " << shader << std::endl;
	}

    // Call the Q3 texture matrix calculation code as used in GtkRadiant
    auto transform = quake3::calculateTextureMatrix(normal, ssr, imageWidth, imageHeight);

    // DarkRadiant's texture emission algorithm is applying a base transform before
    // projecting the vertices to UV space - this is something performed in the
    // idTech4 dmap compiler too, so we have to keep that behaviour.
    // To compensate that effect we're applying an inverse base transformation matrix
    // to this texture transform so we get the same visuals as in Q3.
    // This fix will only work effectively for axis-aligned faces, everything else
    // will not be stored in the 6 floats forming DR's TextureProjections.
    auto axisBase = getBasisTransformForNormal(normal);
    
    // The axis base matrix is orthonormal, so we can invert it by transposing
    transform.multiplyBy(axisBase.getTransposed());

	return getTextureMatrixFromMatrix4(transform);
}

} // namespace map
