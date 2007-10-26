#ifndef PATCHIMPORTEXPORT_H_
#define PATCHIMPORTEXPORT_H_

#include "Patch.h"  // The main Patch class

#include "imap.h"
#include "shaderlib.h"
#include "string/string.h"
#include "parser/DefTokeniser.h"

/* greebo: These are the import/export functions for Patches from and to map files. A PatchDoom3 for example
 * makes use of the PatchDoom3TokenImporter to load a patch from the map file.
 * 
 * This section looks like it could do with some reorganising.
 */

inline void Patch_importShader(Patch& patch, parser::DefTokeniser& tokeniser) {

    std::string texture = tokeniser.nextToken();
    if (texture == "NULL") {
        patch.SetShader(texdef_name_default());
    }
    else {
        patch.SetShader(GlobalTexturePrefix_get() + texture);
    }
}

inline void PatchDoom3_importShader(Patch& patch, 
                                    parser::DefTokeniser& tokeniser)
{
    std::string texture = tokeniser.nextToken();
    if (texture == "_default") {
        patch.SetShader(texdef_name_default());
    }
    else {
        patch.SetShader(texture);
    }
}

inline void Patch_importParams(Patch& patch, parser::DefTokeniser& tok)
{
    tok.assertNextToken("(");

  // parse matrix dimensions
  {
    std::size_t c = strToSizet(tok.nextToken());
    std::size_t r = strToSizet(tok.nextToken());

    patch.setDims(c, r);
  }

    // Version 3 of the patchDef has variable subdivisions
    if(patch.m_patchDef3) {
        patch.m_subdivisions_x = strToSizet(tok.nextToken());
        patch.m_subdivisions_y = strToSizet(tok.nextToken());
    }

    // ignore contents/flags/value
    tok.skipTokens(3);

    tok.assertNextToken(")");
}

inline void Patch_importMatrix(Patch& patch, parser::DefTokeniser& tok)
{
    tok.assertNextToken("(");

    // For each row
    for(std::size_t c=0; c < patch.getWidth(); c++)
    {
        tok.assertNextToken("(");

        // For each column
        for(std::size_t r=0; r<patch.getHeight(); r++) 
        {
            tok.assertNextToken("(");
    
            // Parse vertex coordinates
            patch.ctrlAt(r, c).m_vertex[0] = strToDouble(tok.nextToken());
            patch.ctrlAt(r, c).m_vertex[1] = strToDouble(tok.nextToken());
            patch.ctrlAt(r, c).m_vertex[2] = strToDouble(tok.nextToken());
 
            // Parse texture coordinates
            patch.ctrlAt(r, c).m_texcoord[0] = strToDouble(tok.nextToken());
            patch.ctrlAt(r, c).m_texcoord[1] = strToDouble(tok.nextToken());
 
            tok.assertNextToken(")");
        }

        tok.assertNextToken(")");
    }

    tok.assertNextToken(")");
}

inline void Patch_importFooter(Patch& patch, parser::DefTokeniser& tokeniser)
{
    patch.controlPointsChanged();

    tokeniser.assertNextToken("}");  
    tokeniser.assertNextToken("}");  
}

class PatchTokenImporter : 
    public MapImporter
{
  Patch& m_patch;
public:
  PatchTokenImporter(Patch& patch) : m_patch(patch)
  {
  }
  bool importTokens(parser::DefTokeniser& tokeniser)
  {
    Patch_importShader(m_patch, tokeniser);
    Patch_importParams(m_patch, tokeniser);
    Patch_importMatrix(m_patch, tokeniser);
    Patch_importFooter(m_patch, tokeniser);

    return true;
  }
};

// Takes the given patch token and tries to fill the retrieved values into the passed reference 
class PatchDoom3TokenImporter 
: public MapImporter {
    Patch& m_patch;
public:
    PatchDoom3TokenImporter(Patch& patch) : m_patch(patch)
    {
    }
    
    bool importTokens(parser::DefTokeniser& tokeniser) {
        tokeniser.assertNextToken("{");
        PatchDoom3_importShader(m_patch, tokeniser);
        Patch_importParams(m_patch, tokeniser);
        Patch_importMatrix(m_patch, tokeniser);
        Patch_importFooter(m_patch, tokeniser);

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

inline void Patch_writeDouble(const double& d, std::ostream& os) {
	
	if (isValid(d)) {
		os << d;
	}
	else {
		// Is infinity or NaN, write 0
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
            Patch_writeDouble(patch.ctrlAt(r,c).m_vertex[0], os); 
            os << " ";
            Patch_writeDouble(patch.ctrlAt(r,c).m_vertex[1], os); 
            os << " ";
            Patch_writeDouble(patch.ctrlAt(r,c).m_vertex[2], os); 
            os << " ";
            Patch_writeDouble(patch.ctrlAt(r,c).m_texcoord[0], os); 
            os << " ";
            Patch_writeDouble(patch.ctrlAt(r,c).m_texcoord[1], os); 
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
