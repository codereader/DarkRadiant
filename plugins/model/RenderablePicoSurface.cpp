#include "RenderablePicoSurface.h"

#include "modelskin.h"
#include "math/frustum.h"
#include "iselectable.h"
#include "irenderable.h"

#include <boost/algorithm/string/replace.hpp>

namespace model {

// Constructor. Copy the provided picoSurface_t structure into this object
RenderablePicoSurface::RenderablePicoSurface(picoSurface_t* surf, 
											 const std::string& fExt)
: _originalShaderName(""),
  _mappedShaderName(""),
  _dlRegular(0),
  _dlProgramPosVCol(0),
  _dlProgramNegVCol(0),
  _dlProgramNoVCol(0)
{
	// Get the shader from the picomodel struct. If this is a LWO model, use
	// the material name to select the shader, while for an ASE model the
	// bitmap path should be used.
    picoShader_t* shader = PicoGetSurfaceShader(surf);
    if (shader != 0) {
		if (fExt == "lwo") 
        {
	    	_originalShaderName = PicoGetShaderName(shader);
		}
		else if (fExt == "ase") 
        {
			std::string rawMapName = PicoGetShaderMapName(shader);
			boost::algorithm::replace_all(rawMapName, "\\", "/");
			
			// Take off the everything before "base/", and everything after
			// the first period if it exists (i.e. strip off ".tga")
			std::size_t basePos = rawMapName.find("base");
			
			if (basePos != std::string::npos && basePos > 0)
			{
				std::size_t dotPos = rawMapName.find(".");

				if (dotPos != std::string::npos)
				{
					_originalShaderName = rawMapName.substr(basePos + 5, 
															dotPos - basePos - 5);
				}
				else
				{
					_originalShaderName = rawMapName.substr(basePos + 5);
				}
			}
			else {
				// Unrecognised shader path
				_originalShaderName = "";
			}
		}
    }
    
    _mappedShaderName = _originalShaderName; // no skin at this time
    
    // Capture the shader
    _shader = GlobalRenderSystem().capture(_originalShaderName);
    
    // Get the number of vertices and indices, and reserve capacity in our 
    // vectors in advance by populating them with empty structs.
    int nVerts = PicoGetSurfaceNumVertexes(surf);
    _nIndices = PicoGetSurfaceNumIndexes(surf);
    _vertices.resize(nVerts);
    _indices.resize(_nIndices);
    
    // Stream in the vertex data from the raw struct, expanding the local AABB 
    // to include each vertex.
    for (int vNum = 0; vNum < nVerts; ++vNum) {
    	
    	// Get the vertex position and colour
		Vertex3f vertex(PicoGetSurfaceXYZ(surf, vNum));
		
		// Expand the AABB to include this new vertex
    	_localAABB.includePoint(vertex);
    	
    	_vertices[vNum].vertex = vertex;
    	_vertices[vNum].normal = Normal3f(PicoGetSurfaceNormal(surf, vNum));
    	_vertices[vNum].texcoord = TexCoord2f(PicoGetSurfaceST(surf, 0, vNum));
    	_vertices[vNum].colour = 
    		getColourVector(PicoGetSurfaceColor(surf, 0, vNum));
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

// Destructor. Release the GL display lists.
RenderablePicoSurface::~RenderablePicoSurface() 
{
	glDeleteLists(_dlRegular, 1);
	glDeleteLists(_dlProgramNoVCol, 1);	
	glDeleteLists(_dlProgramPosVCol, 1);	
    glDeleteLists(_dlProgramNegVCol, 1);
}

// Convert byte pointers to colour vector
Vector3 RenderablePicoSurface::getColourVector(unsigned char* array) {
	if (array) {
		return Vector3(array[0] / 255.0, array[1] / 255.0, array[2] / 255.0);
	}
	else {
		return Vector3(1.0, 1.0, 1.0); // white
	}
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
void RenderablePicoSurface::submitRenderables(RenderableCollector& rend,
											  const Matrix4& localToWorld)
{
	// Submit geometry
	rend.SetState(_shader, RenderableCollector::eFullMaterials);
    rend.addRenderable(*this, localToWorld);
}

// Back-end render function
void RenderablePicoSurface::render(const RenderInfo& info) const 
{
	// Invoke appropriate display list
	if (info.checkFlag(RENDER_PROGRAM))
    {
        if (info.checkFlag(RENDER_MATERIAL_VCOL))
        {
            if (info.checkFlag(RENDER_VCOL_INVERT))
            {
                glCallList(_dlProgramNegVCol);
            }
            else
            {
                glCallList(_dlProgramPosVCol);
            }
        }
        else
        {
            glCallList(_dlProgramNoVCol);
        }
	}
	else 
    {
		glCallList(_dlRegular);
	}
}

// Construct a list for GLProgram mode, either with or without vertex colour
GLuint RenderablePicoSurface::compileProgramList(
        ShaderLayer::VertexColourMode mode
)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);

	glBegin(GL_TRIANGLES);
	for (Indices::const_iterator i = _indices.begin();
		 i != _indices.end();
		 ++i)
	{
		// Get the vertex for this index
		ArbitraryMeshVertex& v = _vertices[*i];

		// Submit the vertex attributes and coordinate
		if (GLEW_ARB_vertex_program) 
        {
			glVertexAttrib2dvARB(ATTR_TEXCOORD, v.texcoord);
			glVertexAttrib3dvARB(ATTR_TANGENT, v.tangent);
			glVertexAttrib3dvARB(ATTR_BITANGENT, v.bitangent);
			glVertexAttrib3dvARB(ATTR_NORMAL, v.normal);
		}

        // Optional vertex colour
        if (mode == ShaderLayer::VERTEX_COLOUR_MULTIPLY)
        {
            glColor3dv(v.colour);
        }
        else if (mode == ShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY)
        {
            glColor3d(
                1.0 - v.colour[0],
                1.0 - v.colour[1],
                1.0 - v.colour[2]
            );
        }

        // Submit the vertex itself
		glVertex3dv(v.vertex);	
	}

    // Set vertex colour back to white
    // HACK: find out why other objects are not setting their correct colour,
    // and fix them.
    glColor3f(1, 1, 1);

	glEnd();
	glEndList();

    return list;
}

// Construct the two display lists
void RenderablePicoSurface::createDisplayLists() 
{
	// Generate the lists for lighting mode
    _dlProgramNoVCol = compileProgramList(
        ShaderLayer::VERTEX_COLOUR_NONE
    );
    _dlProgramPosVCol = compileProgramList(
        ShaderLayer::VERTEX_COLOUR_MULTIPLY
    );
    _dlProgramNegVCol = compileProgramList(
        ShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY
    );

	// Generate the list for flat-shaded (unlit) mode
	_dlRegular = glGenLists(1);
	glNewList(_dlRegular, GL_COMPILE);
	
	glBegin(GL_TRIANGLES);
	for (Indices::const_iterator i = _indices.begin();
		 i != _indices.end();
		 ++i)
	{
		// Get the vertex for this index
		ArbitraryMeshVertex& v = _vertices[*i];
		
		// Submit attributes
		glNormal3dv(v.normal);
		glTexCoord2dv(v.texcoord);
		glVertex3dv(v.vertex);	
	}
	glEnd();
	
	glEndList();
}

// Apply a skin to this surface
void RenderablePicoSurface::applySkin(const ModelSkin& skin) 
{
	// Look up the remap for this surface's material name. If there is a remap
	// change the Shader* to point to the new shader.
	std::string remap = skin.getRemap(_originalShaderName);
	if (remap != "" && remap != _mappedShaderName) { // change to a new shader
		// Switch shader objects
		_shader = GlobalRenderSystem().capture(remap);

		// Save the remapped shader name
		_mappedShaderName = remap; 
	}
	else if (remap == "" && _mappedShaderName != _originalShaderName) {
		// No remap, so reset our shader to the original unskinned shader	
		_shader = GlobalRenderSystem().capture(_originalShaderName);

		// Reset the remapped shader name
		_mappedShaderName = _originalShaderName; 
	}
}

// Perform selection test for this surface
void RenderablePicoSurface::testSelect(Selector& selector, 
									   SelectionTest& test,
									   const Matrix4& localToWorld) const
{
	if (!_vertices.empty() && !_indices.empty()) {
		// Test for triangle selection
		test.BeginMesh(localToWorld);
		SelectionIntersection result;

		test.TestTriangles(
			VertexPointer(&_vertices[0].vertex, sizeof(ArbitraryMeshVertex)),
      		IndexPointer(&_indices[0], 
      					 IndexPointer::index_type(_indices.size())),
			result
		);
		
		// Add the intersection to the selector if it is valid
		if(result.valid()) {
			selector.addIntersection(result);
		}
	}
}

} // namespace model
