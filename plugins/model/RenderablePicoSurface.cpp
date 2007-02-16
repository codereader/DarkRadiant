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
  _mappedShaderName("")
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
			_originalShaderName = rawMapName.substr(basePos + 5, dotPos - basePos - 5);
		}
    }
    globalOutputStream() << "  RenderablePicoSurface: using shader " << _originalShaderName.c_str() << "\n";
    
    _mappedShaderName = _originalShaderName; // no skin at this time
    
    // Capture the shader
    _shader = GlobalShaderCache().capture(_originalShaderName);
    
    // Get the number of vertices and indices, and reserve capacity in our vectors in advance
    // by populating them with empty structs.
    int nVerts = PicoGetSurfaceNumVertexes(surf);
    _nIndices = PicoGetSurfaceNumIndexes(surf);
    _vertices.resize(nVerts);
    _indices.resize(_nIndices);
    
    // Stream in the vertex data from the raw struct, expanding the local AABB to include
    // each vertex.
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
	
}

// Destructor
RenderablePicoSurface::~RenderablePicoSurface() {
	GlobalShaderCache().release(_mappedShaderName);
}

// Front-end renderable submission
void RenderablePicoSurface::submitRenderables(Renderer& rend,
											  const Matrix4& localToWorld)
{
	// Submit the lights
	rend.setLights(_lights);
	
	// Submit geometry
	rend.SetState(_shader, Renderer::eFullMaterials);
    rend.addRenderable(*this, localToWorld);
}

// Back-end render function
void RenderablePicoSurface::render(RenderStateFlags flags) const {

	// Use Vertex Arrays to submit data to the GL. We will assume that it is
	// acceptable to perform pointer arithmetic over the elements of a 
	// std::vector, starting from the address of element 0.
	
	glNormalPointer(
		GL_FLOAT, sizeof(ArbitraryMeshVertex), &_vertices[0].normal);
	glVertexPointer(
		3, GL_FLOAT, sizeof(ArbitraryMeshVertex), &_vertices[0].vertex);
	glTexCoordPointer(
		2, GL_FLOAT, sizeof(ArbitraryMeshVertex), &_vertices[0].texcoord);
	
	glDrawElements(GL_TRIANGLES, _nIndices, GL_UNSIGNED_INT, &_indices[0]);

}

// Add a light to our light list
void RenderablePicoSurface::addLight(const RendererLight& light) {
	_lights.addLight(light);
}

// Clear the light list
void RenderablePicoSurface::clearLights() {
	_lights.clear();	
}

// Apply a skin to this surface
void RenderablePicoSurface::applySkin(const ModelSkin& skin) {
	// Look up the remap for this surface's material name. If there is a remap
	// change the Shader* to point to the new shader.
	std::string remap = skin.getRemap(_originalShaderName);
	if (remap != "" && remap != _mappedShaderName) { // change to a new shader
		// Switch shader objects
		GlobalShaderCache().release(_mappedShaderName);
		_shader = GlobalShaderCache().capture(remap);

		// Save the remapped shader name
		_mappedShaderName = remap; 
	}
	else if (remap == "" && _mappedShaderName != _originalShaderName) {
		// No remap, so reset our shader to the original unskinned shader	
		GlobalShaderCache().release(_mappedShaderName);
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
