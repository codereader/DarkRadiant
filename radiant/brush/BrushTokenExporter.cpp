#include "BrushTokenExporter.h"

#include "Brush.h"
#include "Face.h"
#include "string/string.h"
#include "shaderlib.h"

namespace {
	// Writes a double to the given stream and checks for NaN and infinity
	inline void writeDoubleSafe(const double& d, std::ostream& os) {
		if (isValid(d)) {
			os << d;
		}
		else {
			// Is infinity or NaN, write 0
			os << "0";
		}
	} 
}

Doom3FaceTokenExporter::Doom3FaceTokenExporter(const Face& face) : 
	_face(face)
{}

// Export tokens to the provided stream
void Doom3FaceTokenExporter::exportTokens(std::ostream& os) const {
	exportFacePlaneTokens(os);
	exportTexDefTokens(os);
	exportFaceShaderTokens(os);
	exportContentsFlagsTokens(os);
    os << "\n";
}

void Doom3FaceTokenExporter::exportFacePlaneTokens(std::ostream& os) const {
	const FacePlane& facePlane = _face.getPlane();
	
	os << "( ";
	writeDoubleSafe(facePlane.getDoom3Plane().a, os);
	os << " ";
	writeDoubleSafe(facePlane.getDoom3Plane().b, os);
	os << " ";
	writeDoubleSafe(facePlane.getDoom3Plane().c, os);
	os << " ";
	writeDoubleSafe(-facePlane.getDoom3Plane().d, os);
	os << " ";
	os << ") ";
}

void Doom3FaceTokenExporter::exportTexDefTokens(std::ostream& os) const {
	const FaceTexdef& faceTexdef = _face.getTexdef();
	os << "( ";

	os << "( ";
    for (int i = 0; i < 3; i++) {
    	writeDoubleSafe(faceTexdef.m_projection.m_brushprimit_texdef.coords[0][i], os); 
    	os << " ";
	}
	os << ") ";

	os << "( ";
	for (int i = 0; i < 3; i++) {
		writeDoubleSafe(faceTexdef.m_projection.m_brushprimit_texdef.coords[1][i], os);
		os << " ";
	}
	os << ") ";

	os << ") ";
}

void Doom3FaceTokenExporter::exportFaceShaderTokens(std::ostream& os) const {
	const FaceShader& faceShader = _face.getShader();
	
	if (string_empty(shader_get_textureName(faceShader.getShader().c_str()))) {
		os << "\"_default\" ";
	}
	else {
		os << "\"" << faceShader.getShader() << "\" ";
	}
}

void Doom3FaceTokenExporter::exportContentsFlagsTokens(std::ostream& os) const {
	const FaceShader& faceShader = _face.getShader();
	
	os << faceShader.m_flags.m_contentFlags << " ";
	os << faceShader.m_flags.m_surfaceFlags << " ";
	os << faceShader.m_flags.m_value;
}

/** ========================================================================
 * 
 *                          BrushTokenExporter 
 * 
 ** ======================================================================*/

BrushTokenExporter::BrushTokenExporter(const Brush& brush) : 
	_brush(brush)
{}

// Required export function
void BrushTokenExporter::exportTokens(std::ostream& os) const {
	_brush.evaluateBRep(); // ensure b-rep is up-to-date, so that non-contributing faces can be identified.

	if (!_brush.hasContributingFaces()) {
		return;
	}

	// Brush decl header
	os << "{\n";
	os << "brushDef3\n";
	os << "{\n";

	// Iterate over each brush face, exporting the tokens from all contributing
	// faces
	for(Brush::const_iterator i = _brush.begin(); i != _brush.end(); ++i) {
		const Face& face = *(*i);

		if (face.contributes()) {
			Doom3FaceTokenExporter exporter(face);
			exporter.exportTokens(os);
		}
	}

	// Close brush contents and header
	os << "}\n}\n";
}
