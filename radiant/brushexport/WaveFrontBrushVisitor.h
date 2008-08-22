#ifndef WAVEFRONTBRUSHVISITOR_H_
#define WAVEFRONTBRUSHVISITOR_H_

/* greebo: This visits every brush/face and exports it into the given TextFileOutputStream
 * 
 * Inherits from BrushVisitor class in radiant/brush.h 
 */

#include "brush/Brush.h"	// for BrushVisitor declaration

#include <fstream>
#include <boost/lexical_cast.hpp>

/* Exporterclass which will pass every visit-call to a special formatexporter. 
 */
class CExportFormatWavefront: 
	public BrushVisitor 
{
	std::ofstream& m_file;

    // Buffers for written vertices etc
    std::string _vertexBuf;
    std::string _texCoordBuf;
    std::string _faceBuf;
    
    // Persistent counters
	size_t vertices;
	size_t exported;
    
public:
    
	CExportFormatWavefront(std::ofstream& file) : 
		m_file(file),
		vertices(0),
		exported(0)
    {}
    
    // Visitor function, called for each brush
    void visit(const scene::INodePtr& node)
    {
        // Buffers should be clean at start of each brush
        assert(_vertexBuf.empty());
        assert(_texCoordBuf.empty());
        assert(_faceBuf.empty());

        // Get the brush and visit its faces
		Brush* brush = Node_getBrush(node);
		if (brush != NULL)
        {
            m_file << "\ng " << "Brush" << static_cast<int>(exported) << "\n";
            brush->forEachFace(*this);
            m_file << _vertexBuf << "\n";
            m_file << _texCoordBuf;
            m_file << _faceBuf << "\n";

            // Clear the buffers
            _vertexBuf.clear();
            _texCoordBuf.clear();
            _faceBuf.clear();

            // Count the exported brush
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
        // Write coordinates into the export buffers
        _vertexBuf += "v " + std::string(w[i].vertex) + "\n";
        _texCoordBuf += "vt " + std::string(w[i].texcoord) + "\n";

        // Count the exported vertex
        ++vertices;
      }
      
      // Construct the face section
      using boost::lexical_cast;
      _faceBuf += "\nf";
      for(size_t i = v_start; i < vertices; ++i)
      {
        _faceBuf += " " + lexical_cast<std::string>(i+1) 
                  + "/" + lexical_cast<std::string>(i+1);
      }
    }
}; // class CExportFormatWavefront

#endif /*WAVEFRONTBRUSHVISITOR_H_*/
