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
#include "stringio.h"
#include "stream/stringstream.h"
#include "shaderlib.h"
#include "brush/Face.h"
#include "brush/BrushClass.h"

inline bool FaceShader_importContentsFlagsValue(FaceShader& faceShader, Tokeniser& tokeniser)
{
  // parse the optional contents/flags/value
  RETURN_FALSE_IF_FAIL(Tokeniser_getInteger(tokeniser, faceShader.m_flags.m_contentFlags));
  RETURN_FALSE_IF_FAIL(Tokeniser_getInteger(tokeniser, faceShader.m_flags.m_surfaceFlags));
  RETURN_FALSE_IF_FAIL(Tokeniser_getInteger(tokeniser, faceShader.m_flags.m_value));
  return true;
}

inline bool FaceTexdef_importTokens(FaceTexdef& texdef, Tokeniser& tokeniser)
{
  // parse texdef
  RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_texdef._shift[0]));
  RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_texdef._shift[1]));
  RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_texdef._rotate));
  RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_texdef._scale[0]));
  RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_texdef._scale[1]));

  ASSERT_MESSAGE(texdef.m_projection.m_texdef.isSane(), "FaceTexdef_importTokens: bad texdef");
  return true;
}

inline bool FaceTexdef_BP_importTokens(FaceTexdef& texdef, Tokeniser& tokeniser)
{
  // parse alternate texdef
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "("));
  {
    RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "("));
    RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_brushprimit_texdef.coords[0][0]));
    RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_brushprimit_texdef.coords[0][1]));
    RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_brushprimit_texdef.coords[0][2]));
    RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, ")"));
  }
  {
    RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "("));
    RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_brushprimit_texdef.coords[1][0]));
    RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_brushprimit_texdef.coords[1][1]));
    RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, texdef.m_projection.m_brushprimit_texdef.coords[1][2]));
    RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, ")"));
  }
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, ")"));
  return true;
}

inline bool FacePlane_importTokens(FacePlane& facePlane, Tokeniser& tokeniser)
{
  // parse planepts
  for(std::size_t i = 0; i<3; i++)
  {
    RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "("));
    for(std::size_t j = 0; j < 3; ++j)
    {
      RETURN_FALSE_IF_FAIL(Tokeniser_getDouble(tokeniser, facePlane.planePoints()[i][j]));
    }
    RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, ")"));
  }
  facePlane.MakePlane();
  return true;
}

inline bool FacePlane_Doom3_importTokens(FacePlane& facePlane, Tokeniser& tokeniser)
{
  Plane3 plane;
  // parse plane equation
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "("));
  RETURN_FALSE_IF_FAIL(Tokeniser_getDouble(tokeniser, plane.a));
  RETURN_FALSE_IF_FAIL(Tokeniser_getDouble(tokeniser, plane.b));
  RETURN_FALSE_IF_FAIL(Tokeniser_getDouble(tokeniser, plane.c));
  RETURN_FALSE_IF_FAIL(Tokeniser_getDouble(tokeniser, plane.d));
  plane.d = -plane.d;
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, ")"));

  facePlane.setDoom3Plane(plane);
  return true;
}

inline bool FaceShader_Doom3_importTokens(FaceShader& faceShader, Tokeniser& tokeniser)
{
  const char *shader = tokeniser.getToken();
  if(shader == 0)
  {
    Tokeniser_unexpectedError(tokeniser, shader, "#shader-name");
    return false;
  }
  if(string_equal(shader, "_default"))
  {
    shader = texdef_name_default();
  }
  faceShader.setShader(shader);
  return true;
}

inline bool FaceShader_importTokens(FaceShader& faceShader, Tokeniser& tokeniser)
{
  const char* texture = tokeniser.getToken();
  if(texture == 0)
  {
    Tokeniser_unexpectedError(tokeniser, texture, "#texture-name");
    return false;
  }
  if(string_equal(texture, "NULL"))
  {
    faceShader.setShader(texdef_name_default());
  }
  else
  {
    StringOutputStream shader(string_length(GlobalTexturePrefix_get()) + string_length(texture));
    shader << GlobalTexturePrefix_get() << texture;
    faceShader.setShader(shader.c_str());
  }
  return true;
}




class Doom3FaceTokenImporter
{
  Face& m_face;
public:
  Doom3FaceTokenImporter(Face& face) : m_face(face)
  {
  }
  bool importTokens(Tokeniser& tokeniser)
  {
    RETURN_FALSE_IF_FAIL(FacePlane_Doom3_importTokens(m_face.getPlane(), tokeniser));
    RETURN_FALSE_IF_FAIL(FaceTexdef_BP_importTokens(m_face.getTexdef(), tokeniser));
    RETURN_FALSE_IF_FAIL(FaceShader_Doom3_importTokens(m_face.getShader(), tokeniser));
    RETURN_FALSE_IF_FAIL(FaceShader_importContentsFlagsValue(m_face.getShader(), tokeniser));

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

class BrushTokenImporter : public MapImporter
{
  Brush& m_brush;

public:
  BrushTokenImporter(Brush& brush) : m_brush(brush)
  {
  }
  bool importTokens(Tokeniser& tokeniser)
  {
    tokeniser.nextLine();
    RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "{"));
    while(1)
    {
      // check for end of brush
      tokeniser.nextLine();
      const char* token = tokeniser.getToken();
      if(string_equal(token, "}"))
      {
        break;
      }

      tokeniser.ungetToken();

      m_brush.push_back(FaceSmartPointer(new Face(&m_brush)));

      //!todo BP support
      tokeniser.nextLine();

      Face& face = *m_brush.back();

      Doom3FaceTokenImporter importer(face);
      RETURN_FALSE_IF_FAIL(importer.importTokens(tokeniser));
      face.planeChanged();
    }

    tokeniser.nextLine();
    RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "}"));

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
