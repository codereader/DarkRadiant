/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_BRUSHTOKENS_H)
#define INCLUDED_BRUSHTOKENS_H

#include "imap.h"
#include "string/string.h"
#include "shaderlib.h"
#include "brush/Face.h"
#include "brush/Brush.h"
#include "parser/DefTokeniser.h"

inline void FaceShader_importContentsFlagsValue(FaceShader& faceShader, 
                                                parser::DefTokeniser& tok)
{
    // parse the optional contents/flags/value
    faceShader.m_flags.m_contentFlags = strToInt(tok.nextToken());
    faceShader.m_flags.m_surfaceFlags = strToInt(tok.nextToken());
    faceShader.m_flags.m_value = strToInt(tok.nextToken());
}

inline void FaceTexdef_BP_importTokens(FaceTexdef& texdef, 
                                       parser::DefTokeniser& tokeniser)
{
    tokeniser.assertNextToken("(");  

    tokeniser.assertNextToken("(");
    texdef.m_projection.m_brushprimit_texdef.coords[0][0] = 
        strToDouble(tokeniser.nextToken());
    texdef.m_projection.m_brushprimit_texdef.coords[0][1] = 
        strToDouble(tokeniser.nextToken());
    texdef.m_projection.m_brushprimit_texdef.coords[0][2] = 
        strToDouble(tokeniser.nextToken());
    tokeniser.assertNextToken(")");  

    tokeniser.assertNextToken("(");  
    texdef.m_projection.m_brushprimit_texdef.coords[1][0] = 
        strToDouble(tokeniser.nextToken());
    texdef.m_projection.m_brushprimit_texdef.coords[1][1] = 
        strToDouble(tokeniser.nextToken());
    texdef.m_projection.m_brushprimit_texdef.coords[1][2] = 
        strToDouble(tokeniser.nextToken());
    tokeniser.assertNextToken(")");  

    tokeniser.assertNextToken(")");  
}

inline void FacePlane_Doom3_importTokens(FacePlane& facePlane, 
                                         parser::DefTokeniser& tokeniser)
{
    // Note: Do not expect an initial "(" since this is already consumed by the
    // while loop in BrushTokenImporter::importTokens().
    
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

inline void FaceShader_Doom3_importTokens(FaceShader& faceShader, 
                                          parser::DefTokeniser& tokeniser)
{
    std::string shader = tokeniser.nextToken();

    if (shader == "_default") {
        shader = texdef_name_default();
    }
    
    faceShader.setShader(shader);
}

class Doom3FaceTokenImporter
{
  Face& m_face;
public:
  Doom3FaceTokenImporter(Face& face) : m_face(face)
  {
  }
  
  bool importTokens(parser::DefTokeniser& tokeniser)
  {
    FacePlane_Doom3_importTokens(m_face.getPlane(), tokeniser);
    FaceTexdef_BP_importTokens(m_face.getTexdef(), tokeniser);
    FaceShader_Doom3_importTokens(m_face.getShader(), tokeniser);
    FaceShader_importContentsFlagsValue(m_face.getShader(), tokeniser);

    m_face.getTexdef().m_projectionInitialised = true;
    m_face.getTexdef().m_scaleApplied = true;

    return true;
  }
};

/* Export the plane specification for a Doom 3 brushface.
 */
inline void FacePlane_Doom3_exportTokens(const FacePlane& facePlane, std::ostream& os)
{
	os << "( ";
	os << facePlane.getDoom3Plane().a << " ";
	os << facePlane.getDoom3Plane().b << " ";
	os << facePlane.getDoom3Plane().c << " ";
	os << -facePlane.getDoom3Plane().d << " ";
	os << ") ";
}

/* Export the texture coordinate information from a Doom 3 brushface.
 */
inline void FaceTexdef_BP_exportTokens(const FaceTexdef& faceTexdef, std::ostream& os)
{
	os << "( ";

	os << "( ";
    for(int i = 0; i < 3; i++)
    {
      os << faceTexdef.m_projection.m_brushprimit_texdef.coords[0][i] << " ";
    }
	os << ") ";

	os << "( ";
    for(int i = 0; i < 3; i++)
    {
      os << faceTexdef.m_projection.m_brushprimit_texdef.coords[1][i] << " ";
    }
	os << ") ";

	os << ") ";
}

/* Export shader surface flags.
 */

inline void FaceShader_ContentsFlagsValue_exportTokens(const FaceShader& faceShader, std::ostream& os)
{
	os << faceShader.m_flags.m_contentFlags << " ";
	os << faceShader.m_flags.m_surfaceFlags << " ";
	os << faceShader.m_flags.m_value;
}

/* Export Doom 3 face shader information.
 */

inline void FaceShader_Doom3_exportTokens(const FaceShader& faceShader, std::ostream& os)
{
	if(string_empty(shader_get_textureName(faceShader.getShader().c_str()))) {
		os << "\"_default\" ";
	}
	else {
    	os << "\"" << faceShader.getShader().c_str() << "\" ";
	}
}

/** Token exporter for Doom 3 brush faces.
 */

class Doom3FaceTokenExporter
{
  const Face& m_face;
public:
  Doom3FaceTokenExporter(const Face& face) : m_face(face)
  {
  }

	// Export tokens to the provided stream
	void exportTokens(std::ostream& os) const {
	    FacePlane_Doom3_exportTokens(m_face.getPlane(), os);
	    FaceTexdef_BP_exportTokens(m_face.getTexdef(), os);
	    FaceShader_Doom3_exportTokens(m_face.getShader(), os);
	    FaceShader_ContentsFlagsValue_exportTokens(m_face.getShader(), os);
	    os << "\n";
	}
};

class BrushTokenImporter : 
	public MapImporter
{
  Brush& m_brush;

public:
  BrushTokenImporter(Brush& brush) : m_brush(brush)
  {
  }
  
    /**
     * Required token import method.
     */
    bool importTokens(parser::DefTokeniser& tokeniser)
    {
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
                m_brush.push_back(FacePtr(new Face(&m_brush)));
                Face& face = *m_brush.back();

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
        
        m_brush.planeChanged();
        m_brush.shaderChanged();
    
        return true;
    }
};

/* Token exporter for Doom 3 brushes.
 */

class BrushTokenExporter : public MapExporter
{
  const Brush& m_brush;

public:
  BrushTokenExporter(const Brush& brush) : m_brush(brush)
  {
  }

	// Required export function
	void exportTokens(std::ostream& os) const {
	    m_brush.evaluateBRep(); // ensure b-rep is up-to-date, so that non-contributing faces can be identified.

	    if(!m_brush.hasContributingFaces())
	    {
	      return;
	    }

		// Brush decl header
		os << "{\n";
		os << "brushDef3\n";
		os << "{\n";

		// Iterate over each brush face, exporting the tokens from all contributing
		// faces
	    for(Brush::const_iterator i = m_brush.begin(); i != m_brush.end(); ++i) {
			const Face& face = *(*i);

	      	if(face.contributes()) {
		      	Doom3FaceTokenExporter exporter(face);
		        exporter.exportTokens(os);
	    	}
		}

		// Close brush contents and header
		os << "}\n}\n";
	}
};

    
#endif
