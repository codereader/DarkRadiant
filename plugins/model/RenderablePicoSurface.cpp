#include "RenderablePicoSurface.h"

#include <boost/algorithm/string/replace.hpp>

namespace model {

// Constructor. Copy the provided picoSurface_t structure into this object

RenderablePicoSurface::RenderablePicoSurface(picoSurface_t* surf, const std::string& fExt)
: _shaderName("")
{
	// Get the shader from the picomodel struct. If this is a LWO model, use
	// the material name to select the shader, while for an ASE model the
	// bitmap path should be used.
    picoShader_t* shader = PicoGetSurfaceShader(surf);
    if (shader != 0) {
		if (fExt == "lwo") {
	    	_shaderName = PicoGetShaderName(shader);
		}
		else if (fExt == "ase") {
			std::string rawMapName = PicoGetShaderMapName(shader);
			boost::algorithm::replace_all(rawMapName, "\\", "/");
			// Take off the everything before "base/", and everything after
			// the first period if it exists (i.e. strip off ".tga")
			int basePos = rawMapName.find("base");
			int dotPos = rawMapName.find(".");
			_shaderName = rawMapName.substr(basePos + 5, dotPos - basePos - 5);
		}
    }
    globalOutputStream() << "  RenderablePicoSurface: using shader " << _shaderName.c_str() << "\n";
    
    // Get the number of vertices and indices, and reserve capacity in our vectors in advance
    // by populating them with empty structs.
    int nVerts = PicoGetSurfaceNumVertexes(surf);
    int nInds = PicoGetSurfaceNumIndexes(surf);
    _vertices.resize(nVerts);
    _indices.resize(nInds);
    
    // Stream in the vertex data from the raw struct
    for (int vNum = 0; vNum < nVerts; ++vNum) {
    	_vertices[vNum].vertex = Vertex3f(PicoGetSurfaceXYZ(surf, vNum));
    	_vertices[vNum].normal = Normal3f(PicoGetSurfaceNormal(surf, vNum));
    	_vertices[vNum].texcoord = TexCoord2f(PicoGetSurfaceST(surf, 0, vNum));
    }
    
    // Stream in the index data
    picoIndex_t* ind = PicoGetSurfaceIndexes(surf, 0);
    for (int i = 0; i < nInds; i++)
    	_indices[i] = ind[i];
	
}

} // namespace model
