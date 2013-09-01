#include "RenderablePicoSurface.h"

#include "modelskin.h"
#include "math/Frustum.h"
#include "math/Ray.h"
#include "iselectiontest.h"
#include "irenderable.h"

#include <boost/algorithm/string/replace.hpp>

namespace model {

// Constructor. Copy the provided picoSurface_t structure into this object
RenderablePicoSurface::RenderablePicoSurface(picoSurface_t* surf,
											 const std::string& fExt)
: _shaderName(""),
  _dlRegular(0),
  _dlProgramVcol(0),
  _dlProgramNoVCol(0)
{
	// Get the shader from the picomodel struct. If this is a LWO model, use
	// the material name to select the shader, while for an ASE model the
	// bitmap path should be used.
	picoShader_t* shader = PicoGetSurfaceShader(surf);
	std::string rawName = "";

	if (shader != 0)
	{
		if (fExt == "lwo")
		{
			_shaderName = PicoGetShaderName(shader);
		}
		else if (fExt == "ase")
		{
			rawName = PicoGetShaderName(shader);
			std::string rawMapName = PicoGetShaderMapName(shader);
			_shaderName = cleanupShaderName(rawMapName);
		}
	}

	// If shader not found, fallback to alternative if available
	// _shaderName is empty if the ase material has no BITMAP
	// materialIsValid is false if _shaderName is not an existing shader
	if ((_shaderName.empty() || !GlobalMaterialManager().materialExists(_shaderName)) &&
		!rawName.empty())
	{
		_shaderName = cleanupShaderName(rawName);
	}

	// Capturing the shader happens later on when we have a RenderSystem reference

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

std::string RenderablePicoSurface::cleanupShaderName(const std::string& inName)
{
	const std::string baseFolder = "base";	//FIXME: should be from game.xml
	std::size_t basePos;

	std::string mapName = boost::algorithm::replace_all_copy(inName, "\\", "/");

	// for paths given relative, start from the beginning
	if (mapName.substr(0,6) == "models" || mapName.substr(0,8) == "textures")
	{
		basePos = 0;
	}
	else
	{
		// Take off the everything before "base/", and everything after
		// the first period if it exists (i.e. strip off ".tga")
		basePos = mapName.find(baseFolder);
		if (basePos == std::string::npos)
		{
			// Unrecognised shader path, no base folder.
			// Try the original incase it was already given relative.
			basePos = 0;
		}
		else
		{
			// Increment for for the length of "base/", the / is the +1
			basePos += (baseFolder.size() + 1);
		}
	}

	std::size_t dotPos = mapName.find(".");

	if (dotPos != std::string::npos)
	{
		return mapName.substr(basePos, dotPos - basePos);
	}
	else
	{
		return mapName.substr(basePos);
	}
}

// Destructor. Release the GL display lists.
RenderablePicoSurface::~RenderablePicoSurface()
{
	glDeleteLists(_dlRegular, 1);
	glDeleteLists(_dlProgramNoVCol, 1);
	glDeleteLists(_dlProgramVcol, 1);
}

// Convert byte pointers to colour vector
Vector3 RenderablePicoSurface::getColourVector(unsigned char* array) {
	if (array) {
		return Vector3(array[0] / 255.0f, array[1] / 255.0f, array[2] / 255.0f);
	}
	else {
		return Vector3(1.0f, 1.0f, 1.0f); // white
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

		// Call the tangent calculation function
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
											  const Matrix4& localToWorld,
											  const ShaderPtr& shader,
											  const IRenderEntity& entity)
{
	// Submit geometry
	rend.SetState(shader, RenderableCollector::eFullMaterials);
    rend.addRenderable(*this, localToWorld, entity);
}

// Back-end render function
void RenderablePicoSurface::render(const RenderInfo& info) const
{
	// Invoke appropriate display list
	if (info.checkFlag(RENDER_PROGRAM))
    {
        if (info.checkFlag(RENDER_VERTEX_COLOUR))
        {
            glCallList(_dlProgramVcol);
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
GLuint RenderablePicoSurface::compileProgramList(bool includeColour)
{
    GLuint list = glGenLists(1);
	assert(list != 0); // check if we run out of display lists
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
        if (includeColour)
        {
            glColor3dv(v.colour);
        }

        // Submit the vertex itself
		glVertex3dv(v.vertex);
	}
	glEnd();

	glEndList();

    return list;
}

// Construct the two display lists
void RenderablePicoSurface::createDisplayLists()
{
	// Generate the lists for lighting mode
    _dlProgramNoVCol = compileProgramList(false);
    _dlProgramVcol = compileProgramList(true);

	// Generate the list for flat-shaded (unlit) mode
	_dlRegular = glGenLists(1);
	assert(_dlRegular != 0); // check if we run out of display lists
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

// Perform selection test for this surface
void RenderablePicoSurface::testSelect(Selector& selector,
									   SelectionTest& test,
									   const Matrix4& localToWorld) const
{
	if (!_vertices.empty() && !_indices.empty())
	{
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

int RenderablePicoSurface::getNumVertices() const
{
	return static_cast<int>(_vertices.size());
}

int RenderablePicoSurface::getNumTriangles() const
{
	return static_cast<int>(_indices.size() / 3); // 3 indices per triangle
}

const ArbitraryMeshVertex& RenderablePicoSurface::getVertex(int vertexIndex) const
{
	assert(vertexIndex >= 0 && vertexIndex < static_cast<int>(_vertices.size()));
	return _vertices[vertexIndex];
}

ModelPolygon RenderablePicoSurface::getPolygon(int polygonIndex) const
{
	assert(polygonIndex >= 0 && polygonIndex*3 < static_cast<int>(_indices.size()));

	ModelPolygon poly;

	poly.a = _vertices[_indices[polygonIndex*3]];
	poly.b = _vertices[_indices[polygonIndex*3 + 1]];
	poly.c = _vertices[_indices[polygonIndex*3 + 2]];

	return poly;
}

const std::string& RenderablePicoSurface::getDefaultMaterial() const
{
	return _shaderName;
}

void RenderablePicoSurface::setDefaultMaterial(const std::string& defaultMaterial)
{
	_shaderName = defaultMaterial;
}

bool RenderablePicoSurface::getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld)
{
	Vector3 bestIntersection = ray.origin;
	Vector3 triIntersection;

	for (Indices::const_iterator i = _indices.begin();
		 i != _indices.end();
		 i += 3)
	{
		// Get the vertices for this triangle
		const ArbitraryMeshVertex& p1 = _vertices[*(i)];
		const ArbitraryMeshVertex& p2 = _vertices[*(i+1)];
		const ArbitraryMeshVertex& p3 = _vertices[*(i+2)];

		if (ray.intersectTriangle(localToWorld.transformPoint(p1.vertex), 
			localToWorld.transformPoint(p2.vertex), localToWorld.transformPoint(p3.vertex), triIntersection))
		{
			intersection = triIntersection;
			
			// Test if this surface intersection is better than what we currently have
			float oldDistSquared = (bestIntersection - ray.origin).getLengthSquared();
			float newDistSquared = (triIntersection - ray.origin).getLengthSquared();

			if ((oldDistSquared == 0 && newDistSquared > 0) || newDistSquared < oldDistSquared)
			{
				bestIntersection = triIntersection;
			}
		}
	}

	if ((bestIntersection - ray.origin).getLengthSquared() > 0)
	{
		intersection = bestIntersection;
		return true;
	}
	else
	{
		return false;
	}
}

} // namespace model
