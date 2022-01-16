#include "StaticModelSurface.h"

#include "itextstream.h"
#include "modelskin.h"
#include "math/Frustum.h"
#include "math/Ray.h"
#include "iselectiontest.h"
#include "irenderable.h"
#include "gamelib.h"

#include "string/replace.h"

namespace model
{

StaticModelSurface::StaticModelSurface(std::vector<ArbitraryMeshVertex>&& vertices, std::vector<unsigned int>&& indices) :
    _vertices(vertices),
    _indices(indices),
    _dlRegular(0),
    _dlProgramVcol(0),
    _dlProgramNoVCol(0)
{
    // Expand the local AABB to include all vertices
    for (const auto& vertex : _vertices)
    {
        _localAABB.includePoint(vertex.vertex);
    }

    calculateTangents();
    createDisplayLists();
}

StaticModelSurface::StaticModelSurface(const StaticModelSurface& other) :
	_defaultMaterial(other._defaultMaterial),
	_vertices(other._vertices),
	_indices(other._indices),
	_localAABB(other._localAABB),
	_dlRegular(0),
	_dlProgramVcol(0),
	_dlProgramNoVCol(0)
{
	createDisplayLists();
}

// Destructor. Release the GL display lists.
StaticModelSurface::~StaticModelSurface()
{
	glDeleteLists(_dlRegular, 1);
	glDeleteLists(_dlProgramNoVCol, 1);
	glDeleteLists(_dlProgramVcol, 1);
}

// Tangent calculation
void StaticModelSurface::calculateTangents()
{
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
void StaticModelSurface::render(const RenderInfo& info) const
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
GLuint StaticModelSurface::compileProgramList(bool includeColour)
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
            glColor4dv(v.colour);
        }

        // Submit the vertex itself
		glVertex3dv(v.vertex);
	}
	glEnd();

	glEndList();

    return list;
}

// Construct the two display lists
void StaticModelSurface::createDisplayLists()
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
void StaticModelSurface::testSelect(Selector& selector, SelectionTest& test,
    const Matrix4& localToWorld, bool twoSided) const
{
	if (!_vertices.empty() && !_indices.empty())
	{
		// Test for triangle selection
		test.BeginMesh(localToWorld, twoSided);
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

int StaticModelSurface::getNumVertices() const
{
	return static_cast<int>(_vertices.size());
}

int StaticModelSurface::getNumTriangles() const
{
	return static_cast<int>(_indices.size() / 3); // 3 indices per triangle
}

const ArbitraryMeshVertex& StaticModelSurface::getVertex(int vertexIndex) const
{
	assert(vertexIndex >= 0 && vertexIndex < static_cast<int>(_vertices.size()));
	return _vertices[vertexIndex];
}

ModelPolygon StaticModelSurface::getPolygon(int polygonIndex) const
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

const std::vector<ArbitraryMeshVertex>& StaticModelSurface::getVertexArray() const
{
	return _vertices;
}

const std::vector<unsigned int>& StaticModelSurface::getIndexArray() const
{
	return _indices;
}

const std::string& StaticModelSurface::getDefaultMaterial() const
{
	return _defaultMaterial;
}

void StaticModelSurface::setDefaultMaterial(const std::string& defaultMaterial)
{
	_defaultMaterial = defaultMaterial;
}

const std::string& StaticModelSurface::getActiveMaterial() const
{
    return !_activeMaterial.empty() ? _activeMaterial : _defaultMaterial;
}

void StaticModelSurface::setActiveMaterial(const std::string& activeMaterial)
{
	_activeMaterial = activeMaterial;
}

const AABB& StaticModelSurface::getSurfaceBounds() const
{
    return getAABB();
}

bool StaticModelSurface::getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld)
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
			auto oldDistSquared = (bestIntersection - ray.origin).getLengthSquared();
			auto newDistSquared = (triIntersection - ray.origin).getLengthSquared();

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

void StaticModelSurface::applyScale(const Vector3& scale, const StaticModelSurface& originalSurface)
{
	if (scale.x() == 0 || scale.y() == 0 || scale.z() == 0)
	{
		rMessage() << "StaticModelSurface: Cannot apply scale with a zero diagonal element" << std::endl;
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
