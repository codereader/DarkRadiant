#include "BrushTokenImporter.h"

#include "Brush.h"
#include "Face.h"
#include "string/string.h"
#include "shaderlib.h"

Doom3FaceTokenImporter::Doom3FaceTokenImporter(Face& face) : 
	_face(face)
{}
  
bool Doom3FaceTokenImporter::importTokens(parser::DefTokeniser& tokeniser) {
	importFacePlaneTokens(tokeniser);
	importTexDefTokens(tokeniser);
	importFaceShaderTokens(tokeniser);
	importContentsFlagsTokens(tokeniser);

	_face.getTexdef().m_projectionInitialised = true;
	_face.getTexdef().m_scaleApplied = true;

	return true;
}

void Doom3FaceTokenImporter::importFacePlaneTokens(parser::DefTokeniser& tokeniser) {
	// Note: Do not expect an initial "(" since this is already consumed by the
	// while loop in BrushTokenImporter::importTokens().
	FacePlane& facePlane = _face.getPlane();

	// Construct a plane and parse its values
	Plane3 plane;

	plane.a = strToDouble(tokeniser.nextToken());
	plane.b = strToDouble(tokeniser.nextToken());
	plane.c = strToDouble(tokeniser.nextToken());
	plane.d = strToDouble(tokeniser.nextToken());

	plane.d = -plane.d;

	tokeniser.assertNextToken(")");

	facePlane.setDoom3Plane(plane);
}

void Doom3FaceTokenImporter::importTexDefTokens(parser::DefTokeniser& tokeniser) {
	FaceTexdef& texdef = _face.getTexdef();
	
	tokeniser.assertNextToken("(");

	tokeniser.assertNextToken("(");
	texdef.m_projection.m_brushprimit_texdef.coords[0][0] = strToDouble(tokeniser.nextToken());
	texdef.m_projection.m_brushprimit_texdef.coords[0][1] = strToDouble(tokeniser.nextToken());
	texdef.m_projection.m_brushprimit_texdef.coords[0][2] = strToDouble(tokeniser.nextToken());
	tokeniser.assertNextToken(")");

	tokeniser.assertNextToken("(");
	texdef.m_projection.m_brushprimit_texdef.coords[1][0] = strToDouble(tokeniser.nextToken());
	texdef.m_projection.m_brushprimit_texdef.coords[1][1] = strToDouble(tokeniser.nextToken());
	texdef.m_projection.m_brushprimit_texdef.coords[1][2] = strToDouble(tokeniser.nextToken());
	tokeniser.assertNextToken(")");

	tokeniser.assertNextToken(")");
}

void Doom3FaceTokenImporter::importFaceShaderTokens(parser::DefTokeniser& tokeniser) {
	FaceShader& faceShader = _face.getFaceShader();
	
    std::string shader = tokeniser.nextToken();

    if (shader == "_default") {
        shader = texdef_name_default();
    }
    
    faceShader.setMaterialName(shader);
}

void Doom3FaceTokenImporter::importContentsFlagsTokens(parser::DefTokeniser& tok) {
	FaceShader& faceShader = _face.getFaceShader();
	
	// parse the optional contents/flags/value
	faceShader.m_flags.m_contentFlags = strToInt(tok.nextToken());
	faceShader.m_flags.m_surfaceFlags = strToInt(tok.nextToken());
	faceShader.m_flags.m_value = strToInt(tok.nextToken());
}

/** ========================================================================
 * 
 *                          BrushTokenImporter 
 * 
 ** ======================================================================*/

BrushTokenImporter::BrushTokenImporter(Brush& brush) : 
	_brush(brush)
{}

bool BrushTokenImporter::importTokens(parser::DefTokeniser& tokeniser) {
    tokeniser.assertNextToken("{");

    while (1) {
        std::string token = tokeniser.nextToken();
        
        // Token should be either a "(" (start of face) or "}" (end of
        // brush
        if (token == "}") {
            break;
        }
        else if (token == "(") { // FACE

            // Add a new Face to the brush and get a reference to it
            _brush.push_back(FacePtr(new Face(_brush, &_brush)));
            Face& face = *_brush.back();

            Doom3FaceTokenImporter importer(face);
            importer.importTokens(tokeniser);

            face.planeChanged();
        }
        else {
            throw std::runtime_error(
            	"BrushTokenImporter: invalid token '" + token + "'"
            );
        }
    }

    // Final outer "}"
    tokeniser.assertNextToken("}");
    
    _brush.planeChanged();
    _brush.shaderChanged();

    return true;
}
