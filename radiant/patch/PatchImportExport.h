#ifndef PATCHIMPORTEXPORT_H_
#define PATCHIMPORTEXPORT_H_

#include "Patch.h"	// The main Patch class

#include "imap.h"
#include "shaderlib.h"
#include "iscriplib.h"
#include "stringio.h"
#include "string/string.h"
#include "stream/stringstream.h"

/* greebo: These are the import/export functions for Patches from and to map files. A PatchDoom3 for example
 * makes use of the PatchDoom3TokenImporter to load a patch from the map file.
 * 
 * This section looks like it could do with some reorganising.
 */

inline bool Patch_importHeader(Patch& patch, Tokeniser& tokeniser) {
  tokeniser.nextLine();
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "{"));
  return true;
}

inline bool Patch_importShader(Patch& patch, Tokeniser& tokeniser) {
  // parse shader name
  tokeniser.nextLine();
  const char* texture = tokeniser.getToken();
  if(texture == 0)
  {
    Tokeniser_unexpectedError(tokeniser, texture, "#texture-name");
    return false;
  }
  if(string_equal(texture, "NULL"))
  {
    patch.SetShader(texdef_name_default());
  }
  else
  {
    StringOutputStream shader(string_length(GlobalTexturePrefix_get()) + string_length(texture));
    shader << GlobalTexturePrefix_get() << texture;
    patch.SetShader(shader.c_str());
  }
  return true;
}

inline bool PatchDoom3_importShader(Patch& patch, Tokeniser& tokeniser)
{
  // parse shader name
  tokeniser.nextLine();
  const char *shader = tokeniser.getToken();
  if(shader == 0)
  {
    Tokeniser_unexpectedError(tokeniser, shader, "#shader-name");
    return false;
  }
  if(string_equal(shader, "_default"))
  {
    shader = texdef_name_default().c_str();
  }
  patch.SetShader(shader);
  return true;
}

inline bool Patch_importParams(Patch& patch, Tokeniser& tokeniser)
{
  tokeniser.nextLine();
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "("));

  // parse matrix dimensions
  {
    std::size_t c, r;
    RETURN_FALSE_IF_FAIL(Tokeniser_getSize(tokeniser, c));
    RETURN_FALSE_IF_FAIL(Tokeniser_getSize(tokeniser, r));

    patch.setDims(c, r);
  }

  if(patch.m_patchDef3)
  {
    RETURN_FALSE_IF_FAIL(Tokeniser_getSize(tokeniser, patch.m_subdivisions_x));
    RETURN_FALSE_IF_FAIL(Tokeniser_getSize(tokeniser, patch.m_subdivisions_y));
  }

  // ignore contents/flags/value
  int tmp;
  RETURN_FALSE_IF_FAIL(Tokeniser_getInteger(tokeniser, tmp));
  RETURN_FALSE_IF_FAIL(Tokeniser_getInteger(tokeniser, tmp));
  RETURN_FALSE_IF_FAIL(Tokeniser_getInteger(tokeniser, tmp));

  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, ")"));
  return true;
}

inline bool Patch_importMatrix(Patch& patch, Tokeniser& tokeniser)
{
  // parse matrix
  tokeniser.nextLine();
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "("));
  {
    for(std::size_t c=0; c<patch.getWidth(); c++)
    {
      tokeniser.nextLine();
      RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "("));
      for(std::size_t r=0; r<patch.getHeight(); r++)
      {
        RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "("));
    
        RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, patch.ctrlAt(r,c).m_vertex[0]));
        RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, patch.ctrlAt(r,c).m_vertex[1]));
        RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, patch.ctrlAt(r,c).m_vertex[2]));
        RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, patch.ctrlAt(r,c).m_texcoord[0]));
        RETURN_FALSE_IF_FAIL(Tokeniser_getFloat(tokeniser, patch.ctrlAt(r,c).m_texcoord[1]));

        RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, ")"));
      }
      RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, ")"));
    }
  }
  tokeniser.nextLine();
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, ")"));
  return true;
}

inline bool Patch_importFooter(Patch& patch, Tokeniser& tokeniser)
{
  patch.controlPointsChanged();

  tokeniser.nextLine();
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "}"));

  tokeniser.nextLine();
  RETURN_FALSE_IF_FAIL(Tokeniser_parseToken(tokeniser, "}"));
  return true;
}

class PatchTokenImporter : public MapImporter
{
  Patch& m_patch;
public:
  PatchTokenImporter(Patch& patch) : m_patch(patch)
  {
  }
  bool importTokens(Tokeniser& tokeniser)
  {
    RETURN_FALSE_IF_FAIL(Patch_importHeader(m_patch, tokeniser));
    RETURN_FALSE_IF_FAIL(Patch_importShader(m_patch, tokeniser));
    RETURN_FALSE_IF_FAIL(Patch_importParams(m_patch, tokeniser));
    RETURN_FALSE_IF_FAIL(Patch_importMatrix(m_patch, tokeniser));
    RETURN_FALSE_IF_FAIL(Patch_importFooter(m_patch, tokeniser));

    return true;
  }
};

// Takes the given patch token and tries to fill the retrieved values into the passed reference 
class PatchDoom3TokenImporter : public MapImporter {
	Patch& m_patch;
public:
	PatchDoom3TokenImporter(Patch& patch) : m_patch(patch)
	{
	}
	
	bool importTokens(Tokeniser& tokeniser) {
		RETURN_FALSE_IF_FAIL(Patch_importHeader(m_patch, tokeniser));
		RETURN_FALSE_IF_FAIL(PatchDoom3_importShader(m_patch, tokeniser));
		RETURN_FALSE_IF_FAIL(Patch_importParams(m_patch, tokeniser));
		RETURN_FALSE_IF_FAIL(Patch_importMatrix(m_patch, tokeniser));
		RETURN_FALSE_IF_FAIL(Patch_importFooter(m_patch, tokeniser));

		return true;
	}
};

inline void Patch_exportHeader(const Patch& patch, std::ostream& os)
{
	os << "{\n";
	os << (patch.m_patchDef3 ? "patchDef3\n" : "patchDef2\n");
	
	os << "{\n";
}

inline void PatchDoom3_exportShader(const Patch& patch, std::ostream& os)
{
	if (*(shader_get_textureName(patch.GetShader().c_str())) == '\0') {
    	os << "\"_default\"";
	}
	else  {
    	os << "\"" << patch.GetShader() << "\"";
	}
	os << "\n";
}

inline void Patch_exportParams(const Patch& patch, std::ostream& os)
{
	os << "( ";
	os << patch.getWidth() << " ";
	os << patch.getHeight() << " ";

	if(patch.m_patchDef3) {
		os << patch.m_subdivisions_x << " ";
		os << patch.m_subdivisions_y << " ";
	}
	
	os << "0 0 0 )\n";
}

inline void Patch_writeFloat(const float& f, std::ostream& os) {
	// Check for NaN
	if (f == f) {
		os << f;
	}
	else {
		// isNaN, write 0
		os << "0";
	}
} 

inline void Patch_exportMatrix(const Patch& patch, std::ostream& os)
{
	// Write matrix
	os << "(\n";

	for(std::size_t c=0; c<patch.getWidth(); c++) {
	    os << "( ";
		for(std::size_t r=0; r<patch.getHeight(); r++) {
		    os << "( ";
      		Patch_writeFloat(patch.ctrlAt(r,c).m_vertex[0], os); 
      		os << " ";
      		Patch_writeFloat(patch.ctrlAt(r,c).m_vertex[1], os); 
      		os << " ";
      		Patch_writeFloat(patch.ctrlAt(r,c).m_vertex[2], os); 
      		os << " ";
      		Patch_writeFloat(patch.ctrlAt(r,c).m_texcoord[0], os); 
      		os << " ";
      		Patch_writeFloat(patch.ctrlAt(r,c).m_texcoord[1], os); 
      		os << " ";
      		os << ") ";
		}
	    os << ")\n";
	}
    os << ")\n";
}
  
inline void Patch_exportFooter(const Patch& patch, std::ostream& os)
{
	os << "}\n}\n";
}

/** Map exporter for Doom 3 patches.
 */

class PatchDoom3TokenExporter : public MapExporter
{
  const Patch& m_patch;
public:
  PatchDoom3TokenExporter(Patch& patch) : m_patch(patch)
  {
  }

	// Required export function
	void exportTokens(std::ostream& os) const {
	    Patch_exportHeader(m_patch, os);
	    PatchDoom3_exportShader(m_patch, os);
	    Patch_exportParams(m_patch, os);
	    Patch_exportMatrix(m_patch, os);
	    Patch_exportFooter(m_patch, os);
	}
};

#endif /*PATCHIMPORTEXPORT_H_*/
