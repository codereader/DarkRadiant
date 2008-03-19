#ifndef WAVEFRONTBRUSHVISITOR_H_
#define WAVEFRONTBRUSHVISITOR_H_

/* greebo: This visits every brush/face and exports it into the given TextFileOutputStream
 * 
 * Inherits from BrushVisitor class in radiant/brush.h 
 */

#include "brush/Brush.h"	// for BrushVisitor declaration

#include "stream/stringstream.h"
#include "stream/textfilestream.h"

/* Exporterclass which will pass every visit-call to a special formatexporter. 
 */
class CExportFormatWavefront: 
	public BrushVisitor 
{
	TextFileOutputStream& m_file;

	StringOutputStream vertexbuffer;
	StringOutputStream texcoordbuffer;
	StringOutputStream facebuffer;
    
	size_t vertices;
	size_t exported;
    
public:
    
    CExportFormatWavefront(TextFileOutputStream& file)
        : m_file(file)
    {
      exported = 0;
      vertices = 0;
    }
    
    virtual ~CExportFormatWavefront(void) {}
    
    void visit(const scene::INodePtr& node)
    {
		Brush* brush = Node_getBrush(node);
		if (brush != NULL)
      {
        m_file << "\ng " << "Brush" << static_cast<int>(exported) << "\n";
        brush->forEachFace(*this);
        m_file << vertexbuffer.c_str() << "\n";
        m_file << texcoordbuffer.c_str();
        m_file << facebuffer.c_str() << "\n";
        vertexbuffer.clear();
        texcoordbuffer.clear();
        facebuffer.clear();
        ++exported;
      }
    }
   
    void visit(Face& face) const
    {
      // cast the stupid const away
      const_cast<CExportFormatWavefront*>(this)->visit(face);
    }
    
    void visit(Face& face)
    {
      size_t v_start = vertices;
      const Winding& w(face.getWinding());
      for(size_t i = 0; i < w.numpoints; ++i)
      {
        vertexbuffer << "v " << w[i].vertex.x() << " " << w[i].vertex.y() << " " << w[i].vertex.z() << "\n";
        texcoordbuffer << "vt " << w[i].texcoord.x() << " " << w[i].texcoord.y() << "\n";
        ++vertices;
      }
      
      facebuffer << "\nf";
      for(size_t i = v_start; i < vertices; ++i)
        facebuffer << " " << static_cast<int>(i+1) << "/" << static_cast<int>(i+1);
    }
}; // class CExportFormatWavefront

#endif /*WAVEFRONTBRUSHVISITOR_H_*/
