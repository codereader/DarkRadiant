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

            auto texdef = getTexDef(shader, plane.normal(), shiftS, shiftT, rotation, scaleS, scaleT);

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

	return node;
}

// Code ported from GtkRadiant to calculate the texture projection matrix as done in Q3
// without the ComputeAxisBase preprocessing that is happening in idTech4 before applying texcoords
// https://github.com/TTimo/GtkRadiant/blob/a1ae77798f434bf8fb31a7d91cd137d1ce554e33/radiant/brush.cpp#L85
Vector3 baseaxis[18] =
{
    {0,0,1}, {1,0,0}, {0,-1,0},     // floor
    {0,0,-1}, {1,0,0}, {0,-1,0},    // ceiling
    {1,0,0}, {0,1,0}, {0,0,-1},     // west wall
    {-1,0,0}, {0,1,0}, {0,0,-1},    // east wall
    {0,1,0}, {1,0,0}, {0,0,-1},     // south wall
    {0,-1,0}, {1,0,0}, {0,0,-1}     // north wall
};

void TextureAxisFromPlane(const Vector3& normal, Vector3& xv, Vector3& yv)
{
    Vector3::ElementType best = 0;
    int bestaxis = 0;

    for (int i = 0; i < 6; i++)
    {
        auto dot = normal.dot(baseaxis[i * 3]);

        if (dot > best)
        {
            best = dot;
            bestaxis = i;
        }
    }

    xv = baseaxis[bestaxis * 3 + 1];
    yv = baseaxis[bestaxis * 3 + 2];
}

// Ported from GtkRadiant:
// https://github.com/TTimo/GtkRadiant/blob/a1ae77798f434bf8fb31a7d91cd137d1ce554e33/radiant/brush.cpp#L331
inline void getTextureVectorsForFace(const Vector3& normal, float shiftS, float shiftT, float rotation, 
    float scaleS, float scaleT, float texWidth, float texHeight, float STfromXYZ[2][4])
{
    Vector3 pvecs[2];
    int sv, tv;
    Vector3::ElementType ang, sinv, cosv;
    Vector3::ElementType ns, nt;
    int i, j;
    float scale[2] = { scaleS, scaleT };

    memset(STfromXYZ, 0, 8 * sizeof(float));

    if (!scale[0]) {
        scale[0] = 2;
    }
    if (!scale[1]) {
        scale[1] = 2;
    }

    // get natural texture axis
    TextureAxisFromPlane(normal, pvecs[0], pvecs[1]);

    // rotate axis
    if (rotation == 0) {
        sinv = 0; cosv = 1;
    }
    else if (rotation == 90) {
        sinv = 1; cosv = 0;
    }
    else if (rotation == 180) {
        sinv = 0; cosv = -1;
    }
    else if (rotation == 270) {
        sinv = -1; cosv = 0;
    }
    else
    {
        ang = rotation / 180 * math::PI;
        sinv = sin(ang);
        cosv = cos(ang);
    }

    if (pvecs[0][0]) {
        sv = 0;
    }
    else if (pvecs[0][1]) {
        sv = 1;
    }
    else {
        sv = 2;
    }

    if (pvecs[1][0]) {
        tv = 0;
    }
    else if (pvecs[1][1]) {
        tv = 1;
    }
    else {
        tv = 2;
    }

    for (i = 0; i < 2; i++) {
        ns = cosv * pvecs[i][sv] - sinv * pvecs[i][tv];
        nt = sinv * pvecs[i][sv] + cosv * pvecs[i][tv];
        STfromXYZ[i][sv] = static_cast<float>(ns);
        STfromXYZ[i][tv] = static_cast<float>(nt);
    }

    // scale
    for (i = 0; i < 2; i++)
        for (j = 0; j < 3; j++)
            STfromXYZ[i][j] = STfromXYZ[i][j] / scale[i];

    // shift
    STfromXYZ[0][3] = shiftS;
    STfromXYZ[1][3] = shiftT;

    for (j = 0; j < 4; j++)
    {
        STfromXYZ[0][j] /= texWidth;
        STfromXYZ[1][j] /= texHeight;
    }
}

Matrix4 LegacyBrushDefParser::getTexDef(const std::string& shader, const Vector3& normal, float shiftS, float shiftT, float rotation, float scaleS, float scaleT)
{
	float image_width = 0;
	float image_height = 0;

	auto texture = GlobalMaterialManager().getMaterial(shader)->getEditorImage();

	if (texture)
    {
		image_width = static_cast<float>(texture->getWidth());
		image_height = static_cast<float>(texture->getHeight());
	}

	if (image_width == 0 || image_height == 0)
    {
		rError() << "LegacyBrushDefParser: Failed to load image: " << shader << std::endl;
		image_width = 128;
		image_height = 128;
	}

    auto transform = Matrix4::getIdentity();

#if 1
    // Take the matrix calculation used in GtkRadiant ("Face_TextureVectors in brush.cpp")
    float STfromXYZ[2][4];
    getTextureVectorsForFace(normal, shiftS, shiftT, rotation, scaleS, scaleT, image_width, image_height, STfromXYZ);

    transform.xx() = STfromXYZ[0][0];
    transform.yx() = STfromXYZ[0][1];
    transform.zx() = STfromXYZ[0][2];

    transform.xy() = STfromXYZ[1][0];
    transform.yy() = STfromXYZ[1][1];
    transform.zy() = STfromXYZ[1][2];

    transform.tx() = STfromXYZ[0][3];
    transform.ty() = STfromXYZ[1][3];

#else
	// transform to texdef shift/scale/rotate
    double inverse_scale[2];
	inverse_scale[0] = 1 / (scaleS * image_width);
	inverse_scale[1] = -1 / (scaleT * image_height);

	transform[12] = shiftS / image_width;
	transform[13] = -shiftT / image_height;

	double c = cos(degrees_to_radians(-rotation));
	double s = sin(degrees_to_radians(-rotation));

	transform[0] = c * inverse_scale[0];
	transform[1] = s * inverse_scale[1];
	transform[4] = -s * inverse_scale[0];
	transform[5] = c * inverse_scale[1];
#endif

    // DarkRadiant's texture emission algorithm is applying a base transform before
    // projecting the vertices to UV space - something that is done in the D3 game engine too,
    // so we have to keep that behaviour.
    // To compensate that effect we're applying an inverse base transformation matrix
    // to this texture transform so we get the same visuals as in Q3.
    auto axisBase = getBasisTransformForNormal(normal);
    
    // The axis base matrix is orthonormal, so we can invert it by transposing
    transform.multiplyBy(axisBase.getTransposed());

	return transform;
}

} // namespace map
