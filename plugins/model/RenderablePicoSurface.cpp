#include "RenderablePicoSurface.h"

#include "modelskin.h"
#include "math/frustum.h"
#include "selectable.h"
#include "renderable.h"

#include <boost/algorithm/string/replace.hpp>

namespace model {

// Constructor. Copy the provided picoSurface_t structure into this object
RenderablePicoSurface::RenderablePicoSurface(picoSurface_t* surf, 
											 const std::string& fExt)
: _originalShaderName(""),
  _mappedShaderName(""),
  _normalList(0),
  _lightingList(0)
{
	// Get the shader from the picomodel struct. If this is a LWO model, use
	// the material name to select the shader, while for an ASE model the
	// bitmap path should be used.
    picoShader_t* shader = PicoGetSurfaceShader(surf);
    if (shader != 0) {
		if (fExt == "lwo") {
	    	_originalShaderName = PicoGetShaderName(shader);
		}
		else if (fExt == "ase") {
			std::string rawMapName = PicoGetShaderMapName(shader);
			boost::algorithm::replace_all(rawMapName, "\\", "/");
			
			// Take off the everything before "base/", and everything after
			// the first period if it exists (i.e. strip off ".tga")
			int basePos = rawMapName.find("base");
			int dotPos = rawMapName.find(".");
			if (basePos > 0) {
				_originalShaderName = rawMapName.substr(basePos + 5, 
														dotPos - basePos - 5);
			}
			else {
				// Unrecognised shader path
				_originalShaderName = "";
			}
		}
    }
    
    globalOutputStream() << "  RenderablePicoSurface: using shader " 
    					 << _originalShaderName.c_str() << "\n";
    
    _mappedShaderName = _originalShaderName; // no skin at this time
    
    // Capture the shader
    _shader = GlobalShaderCache().capture(_originalShaderName);
    
    // Get the number of vertices and indices, and reserve capacity in our 
    // vectors in advance by populating them with empty structs.
    int nVerts = PicoGetSurfaceNumVertexes(surf);
    _nIndices = PicoGetSurfaceNumIndexes(surf);
    _vertices.resize(nVerts);
    _indices.resize(_nIndices);
    
    // Stream in the vertex data from the raw struct, expanding the local AABB 
    // to include each vertex.
    for (int vNum = 0; vNum < nVerts; ++vNum) {
		Vertex3f vertex(PicoGetSurfaceXYZ(surf, vNum));
    	_localAABB.includePoint(vertex);
    	
    	_vertices[vNum].vertex = vertex;
    	_vertices[vNum].normal = Normal3f(PicoGetSurfaceNormal(surf, vNum));
    	_vertices[vNum].texcoord = TexCoord2f(PicoGetSurfaceST(surf, 0, vNum));
    }
    
    // Stream in the index data
    picoIndex_t* ind = PicoGetSurfaceIndexes(surf, 0);
    for (unsigned int i = 0; i < _nIndices; i++)
    	_indices[i] = ind[i];
	
	// Calculate the tangent and bitangent vectors
	calculateTangents();
	
	// Construct the DLs
	createDisplayLists();
}

// Tangent calculation
void RenderablePicoSurface::calculateTangents() {
	
	// Calculate the tangents and bitangents using the indices into the vertex
	// array.
	for (Indices::iterator i = _indices.begin();
		 i != _indices.end();
		 i += 3)
	{
		ArbitraryMeshVertex& a = _vertices[*i];
		ArbitraryMeshVertex& b = _vertices[*(i + 1)];
		ArbitraryMeshVertex& c = _vertices[*(i + 2)];
		
		// Call the tangent calculation function from render.h
		ArbitraryMeshTriangle_sumTangents(a, b, c);
	}
	
	// Normalise all of the tangent and bitangent vectors
	for (VertexVector::iterator j = _vertices.begin();
		 j != _vertices.end();
		 ++j)
	{
		j->tangent.normalise();
		j->bitangent.normalise();		
	}
}

// Front-end renderable submission
void RenderablePicoSurface::submitRenderables(Renderer& rend,
											  const Matrix4& localToWorld)
{
	// Submit geometry
	rend.SetState(_shader, Renderer::eFullMaterials);
    rend.addRenderable(*this, localToWorld);
}

// Back-end render function
void RenderablePicoSurface::render(RenderStateFlags flags) const {

	// Invoke appropriate display list
	if (flags & RENDER_BUMP) {
		glCallList(_lightingList);
	}
	else {
		glCallList(_normalList);
	}
}

// Construct the two display lists
void RenderablePicoSurface::createDisplayLists() {

	// Generate the list for lighting mode
	_lightingList = glGenLists(1);
	glNewList(_lightingList, GL_COMPILE);

	glBegin(GL_TRIANGLES);
	for (Indices::const_iterator i = _indices.begin();
		 i != _indices.end();
		 ++i)
	{
		// Get the vertex for this index
		ArbitraryMeshVertex& v = _vertices[*i];

		// Submit the vertex attributes and coordinate
		glVertexAttrib2fvARB(ATTR_TEXCOORD, v.texcoord);
		glVertexAttrib3fvARB(ATTR_TANGENT, v.tangent);
		glVertexAttrib3fvARB(ATTR_BITANGENT, v.bitangent);
		glVertexAttrib3fvARB(ATTR_NORMAL, v.normal);		
		glVertex3fv(v.vertex);	
	}
	glEnd();
	glEndList();

	// Generate the list for flat-shaded (unlit) mode
	_normalList = glGenLists(1);
	glNewList(_normalList, GL_COMPILE);
	
	glBegin(GL_TRIANGLES);
	for (Indices::const_iterator i = _indices.begin();
		 i != _indices.end();
		 ++i)
	{
		// Get the vertex for this index
		ArbitraryMeshVertex& v = _vertices[*i];
		
		// Submit attributes
		glNormal3fv(v.normal);
		glTexCoord2fv(v.texcoord);
		glVertex3fv(v.vertex);	
	}
	glEnd();
	
	glEndList();
}

// Apply a skin to this surface
void RenderablePicoSurface::applySkin(const ModelSkin& skin) {
	// Look up the remap for this surface's material name. If there is a remap
	// change the Shader* to point to the new shader.
	std::string remap = skin.getRemap(_originalShaderName);
	if (remap != "" && remap != _mappedShaderName) { // change to a new shader
		// Switch shader objects
		_shader = GlobalShaderCache().capture(remap);

		// Save the remapped shader name
		_mappedShaderName = remap; 
	}
	else if (remap == "" && _mappedShaderName != _originalShaderName) {
		// No remap, so reset our shader to the original unskinned shader	
		_shader = GlobalShaderCache().capture(_originalShaderName);

		// Reset the remapped shader name
		_mappedShaderName = _originalShaderName; 
	}
}

// Perform volume intersection test on this surface's geometry
VolumeIntersectionValue RenderablePicoSurface::intersectVolume(
	const VolumeTest& test, 
	const Matrix4& localToWorld) const
{
	return test.TestAABB(_localAABB, localToWorld);
}

// Perform selection test for this surface
void RenderablePicoSurface::testSelect(Selector& selector, 
									   SelectionTest& test,
									   const Matrix4& localToWorld) const
{
	// Test for triangle selection
    test.BeginMesh(localToWorld);
    SelectionIntersection result;
    test.TestTriangles(
		VertexPointer(VertexPointer::pointer(&_vertices[0].vertex), 
					  sizeof(ArbitraryMeshVertex)),
      	IndexPointer(&_indices[0], 
      				 IndexPointer::index_type(_indices.size())),
		result
    );

	// Add the intersection to the selector if it is valid
    if(result.valid()) {
		selector.addIntersection(result);
    }
}

} // namespace model
