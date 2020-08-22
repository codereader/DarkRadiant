#include "RenderablePicoSurface.h"

#include "itextstream.h"
#include "modelskin.h"
#include "math/Frustum.h"
#include "math/Ray.h"
#include "iselectiontest.h"
#include "irenderable.h"

#include "string/replace.h"

namespace model {

// Constructor. Copy the provided picoSurface_t structure into this object
RenderablePicoSurface::RenderablePicoSurface(picoSurface_t* surf,
											 const std::string& fExt)
: _defaultMaterial(""),
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
			_defaultMaterial = PicoGetShaderName(shader);
		}
		else if (fExt == "ase")
		{
			rawName = PicoGetShaderName(shader);
			std::string rawMapName = PicoGetShaderMapName(shader);
			_defaultMaterial = cleanupShaderName(rawMapName);
		}
        else // if extension is not handled explicitly, use at least something
        {
            _defaultMaterial = PicoGetShaderName(shader);
        }
	}

	// If shader not found, fallback to alternative if available
	// _defaultMaterial is empty if the ase material has no BITMAP
	// materialIsValid is false if _defaultMaterial is not an existing shader
	if ((_defaultMaterial.empty() || !GlobalMaterialManager().materialExists(_defaultMaterial)) &&
		!rawName.empty())
	{
		_defaultMaterial = cleanupShaderName(rawName);
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
		
		Normal3f normal = PicoGetSurfaceNormal(surf, vNum);

		// Expand the AABB to include this new vertex
    	_localAABB.includePoint(vertex);

    	_vertices[vNum].vertex = vertex;
    	_vertices[vNum].normal = normal;
    	_vertices[vNum].texcoord = TexCoord2f(PicoGetSurfaceST(surf, 0, vNum));
    	_vertices[vNum].colour =
    		getColourVector(PicoGetSurfaceColor(surf, 0, vNum));
    }

    // Stream in the index data
    picoIndex_t* ind = PicoGetSurfaceIndexes(surf, 0);
    for (unsigned int i = 0; i < _nIndices; i++)
    {
    	_indices[i] = ind[i];
    }

	// Calculate the tangent and bitangent vectors
	calculateTangents();

	// Construct the DLs
	createDisplayLists();
}

RenderablePicoSurface::RenderablePicoSurface(const RenderablePicoSurface& other) :
	_defaultMaterial(other._defaultMaterial),
	_vertices(other._vertices),
	_indices(other._indices),
	_nIndices(other._nIndices),
	_localAABB(other._localAABB),
	_dlRegular(0),
	_dlProgramVcol(0),
	_dlProgramNoVCol(0)
{
	createDisplayLists();
}

std::string RenderablePicoSurface::cleanupShaderName(const std::string& inName)
{
	const std::string baseFolder = "base";	//FIXME: should be from game.xml
	std::size_t basePos;

	std::string mapName = string::replace_all_copy(inName, "\\", "/");

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
		if(result.isValid()) {
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

	// For some reason, the PicoSurfaces are loaded such that the triangles have clockwise winding
	// The common convention is to use CCW winding direction, so reverse the index order
	// ASE models define tris in the usual CCW order, but it appears that the pm_ase.c file
	// reverses the vertex indices during parsing.
	poly.c = _vertices[_indices[polygonIndex*3]];
	poly.b = _vertices[_indices[polygonIndex*3 + 1]];
	poly.a = _vertices[_indices[polygonIndex*3 + 2]];

	return poly;
}

const std::vector<ArbitraryMeshVertex>& RenderablePicoSurface::getVertexArray() const
{
	return _vertices;
}

const std::vector<unsigned int>& RenderablePicoSurface::getIndexArray() const
{
	return _indices;
}

const std::string& RenderablePicoSurface::getDefaultMaterial() const
{
	return _defaultMaterial;
}

void RenderablePicoSurface::setDefaultMaterial(const std::string& defaultMaterial)
{
	_defaultMaterial = defaultMaterial;
}

const std::string& RenderablePicoSurface::getActiveMaterial() const
{
	return _activeMaterial;
}

void RenderablePicoSurface::setActiveMaterial(const std::string& activeMaterial)
{
	_activeMaterial = activeMaterial;
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

void RenderablePicoSurface::applyScale(const Vector3& scale, const RenderablePicoSurface& originalSurface)
{
	if (scale.x() == 0 || scale.y() == 0 || scale.z() == 0)
	{
		rMessage() << "RenderablePicoSurface: Cannot apply scale with a zero diagonal element" << std::endl;
		return;
	}

	_localAABB = AABB();

	Matrix4 scaleMatrix = Matrix4::getScale(scale);
	Matrix4 invTranspScale = Matrix4::getScale(Vector3(1/scale.x(), 1/scale.y(), 1/scale.z()));

	assert(originalSurface.getNumVertices() == getNumVertices());

	for (std::size_t i = 0; i < _vertices.size(); ++i)
	{
		_vertices[i].vertex = scaleMatrix.transformPoint(originalSurface._vertices[i].vertex);
		_vertices[i].normal = invTranspScale.transformPoint(originalSurface._vertices[i].normal).getNormalised();

		// Expand the AABB to include this new vertex
		_localAABB.includePoint(_vertices[i].vertex);
	}

	calculateTangents();

	glDeleteLists(_dlRegular, 1);
	glDeleteLists(_dlProgramNoVCol, 1);
	glDeleteLists(_dlProgramVcol, 1);

	createDisplayLists();
}

} // namespace model
