#include "MD5Surface.h"

#include "ivolumetest.h"
#include "string/convert.h"
#include "MD5Model.h"
#include "math/Ray.h"

namespace md5
{

inline VertexPointer vertexpointer_Meshvertex(const MeshVertex* array)
{
  return VertexPointer(&array->vertex, sizeof(MeshVertex));
}

// Constructor
MD5Surface::MD5Surface() :
	_originalShaderName(""),
	_mesh(new MD5Mesh)
{}

MD5Surface::MD5Surface(const MD5Surface& other) :
	_aabb_local(other._aabb_local),
	_originalShaderName(other._originalShaderName),
	_mesh(other._mesh)
{}

// Update geometry
void MD5Surface::updateGeometry()
{
	_aabb_local = AABB();

	for (const auto& vertex : _vertices)
	{
		_aabb_local.includePoint(vertex.vertex);
	}

	for (Indices::iterator i = _indices.begin();
		 i != _indices.end();
		 i += 3)
	{
		auto& a = _vertices[*(i + 0)];
		auto& b = _vertices[*(i + 1)];
		auto& c = _vertices[*(i + 2)];

		MeshTriangle_sumTangents(a, b, c);
	}

	for (auto& vertex : _vertices)
	{
		vertex.tangent.normalise();
		vertex.bitangent.normalise();
	}
}

void MD5Surface::testSelect(Selector& selector,
							SelectionTest& test,
							const Matrix4& localToWorld)
{
	test.BeginMesh(localToWorld);

	SelectionIntersection best;
	test.TestTriangles(
	  vertexpointer_Meshvertex(_vertices.data()),
	  IndexPointer(_indices.data(), IndexPointer::index_type(_indices.size())),
	  best
	);

	if(best.isValid()) {
		selector.addIntersection(best);
	}
}

bool MD5Surface::getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld)
{
	Vector3 bestIntersection = ray.origin;
	Vector3 triIntersection;

	for (Indices::const_iterator i = _indices.begin();
		 i != _indices.end();
		 i += 3)
	{
		// Get the vertices for this triangle
		const auto& p1 = _vertices[*(i)];
		const auto& p2 = _vertices[*(i+1)];
		const auto& p3 = _vertices[*(i+2)];

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

void MD5Surface::setDefaultMaterial(const std::string& name)
{
	_originalShaderName = name;
}

const AABB& MD5Surface::localAABB() const {
	return _aabb_local;
}

int MD5Surface::getNumVertices() const
{
	return static_cast<int>(_vertices.size());
}

int MD5Surface::getNumTriangles() const
{
	return static_cast<int>(_indices.size() / 3);
}

const MeshVertex& MD5Surface::getVertex(int vertexIndex) const
{
	assert(vertexIndex >= 0 && vertexIndex < static_cast<int>(_vertices.size()));
	return _vertices[vertexIndex];
}

model::ModelPolygon MD5Surface::getPolygon(int polygonIndex) const
{
	assert(polygonIndex >= 0 && polygonIndex*3 < static_cast<int>(_indices.size()));

	model::ModelPolygon poly;

	poly.a = _vertices[_indices[polygonIndex*3]];
	poly.b = _vertices[_indices[polygonIndex*3 + 1]];
	poly.c = _vertices[_indices[polygonIndex*3 + 2]];

	return poly;
}

const std::vector<MeshVertex>& MD5Surface::getVertexArray() const
{
	return _vertices;
}

const std::vector<unsigned int>& MD5Surface::getIndexArray() const
{
	return _indices;
}

const std::string& MD5Surface::getDefaultMaterial() const
{
	return _originalShaderName;
}

const std::string& MD5Surface::getActiveMaterial() const
{
	return !_activeMaterial.empty() ? _activeMaterial : _originalShaderName;
}

void MD5Surface::setActiveMaterial(const std::string& activeMaterial)
{
	_activeMaterial = activeMaterial;
}

const AABB& MD5Surface::getSurfaceBounds() const
{
    return _aabb_local;
}

void MD5Surface::updateToDefaultPose(const MD5Joints& joints)
{
	if (_vertices.size() != _mesh->vertices.size())
	{
		_vertices.resize(_mesh->vertices.size());
	}

	for (std::size_t j = 0; j < _mesh->vertices.size(); ++j)
	{
		MD5Vert& vert = _mesh->vertices[j];

		Vector3 skinned(0, 0, 0);

		for (std::size_t k = 0; k != vert.weight_count; ++k)
		{
			MD5Weight& weight = _mesh->weights[vert.weight_index + k];
			const MD5Joint& joint = joints[weight.joint];

			Vector3 rotatedPoint = joint.rotation.transformPoint(weight.v);
			skinned += (rotatedPoint + joint.position) * weight.t;
		}

		_vertices[j].vertex = skinned;
		_vertices[j].texcoord = TexCoord2f(vert.u, vert.v);
		_vertices[j].normal = Normal3(0,0,0);
	}

	// Ensure the index array is ok
	if (_indices.empty())
	{
		buildIndexArray();
	}

	buildVertexNormals();

	updateGeometry();
}

void MD5Surface::updateToSkeleton(const MD5Skeleton& skeleton)
{
	// Ensure we have all vertices allocated
	if (_vertices.size() != _mesh->vertices.size())
	{
		_vertices.resize(_mesh->vertices.size());
	}

	// Deform vertices to fit the skeleton
	for (std::size_t j = 0; j < _mesh->vertices.size(); ++j)
	{
		MD5Vert& vert = _mesh->vertices[j];

		Vector3 skinned(0, 0, 0);

		for (std::size_t k = 0; k != vert.weight_count; ++k)
		{
			MD5Weight& weight = _mesh->weights[vert.weight_index + k];
			const IMD5Anim::Key& key = skeleton.getKey(weight.joint);
			//const Joint& joint = skeleton.getJoint(weight.joint);

			Vector3 rotatedPoint = key.orientation.transformPoint(weight.v);
			skinned += (rotatedPoint + key.origin) * weight.t;
		}

		_vertices[j].vertex = skinned;
		_vertices[j].texcoord = TexCoord2f(vert.u, vert.v);
		_vertices[j].normal = Normal3(0,0,0);
	}

	// Ensure the index array is ok
	if (_indices.empty())
	{
		buildIndexArray();
	}

	buildVertexNormals();

	updateGeometry();
}

void MD5Surface::buildVertexNormals()
{
	for (Indices::iterator j = _indices.begin(); j != _indices.end(); j += 3)
	{
		auto& a = _vertices[*(j + 0)];
		auto& b = _vertices[*(j + 1)];
		auto& c = _vertices[*(j + 2)];

		Vector3 weightedNormal((c.vertex - a.vertex).cross(b.vertex - a.vertex));

		a.normal += weightedNormal;
		b.normal += weightedNormal;
		c.normal += weightedNormal;
	}

	// Normalise all normal vectors
	for (auto& vertex : _vertices)
	{
        vertex.normal.normalise();
	}
}

void MD5Surface::buildIndexArray()
{
	_indices.clear();

	// Build the indices based on the triangle information
	for (const auto& tri : _mesh->triangles)
	{
		_indices.push_back(static_cast<RenderIndex>(tri.a));
		_indices.push_back(static_cast<RenderIndex>(tri.b));
		_indices.push_back(static_cast<RenderIndex>(tri.c));
	}
}

void MD5Surface::parseFromTokens(parser::DefTokeniser& tok)
{
	// Start of datablock
	tok.assertNextToken("mesh");
	tok.assertNextToken("{");

	// Get the reference to the mesh definition
	MD5Mesh& mesh = *_mesh;

	// Get the shader name
	tok.assertNextToken("shader");
	setDefaultMaterial(tok.nextToken());

	// ----- VERTICES ------

	// Read the vertex count
	tok.assertNextToken("numverts");
	std::size_t numVerts = string::convert<std::size_t>(tok.nextToken());

	// Initialise the vertex vector
	MD5Verts& verts = mesh.vertices;
	verts.resize(numVerts);

	// Populate each vertex struct with parsed values
	for (MD5Verts::iterator vt = verts.begin(); vt != verts.end(); ++vt) {

		tok.assertNextToken("vert");

		// Index of vert
		vt->index = string::convert<std::size_t>(tok.nextToken());

		// U and V texcoords
		tok.assertNextToken("(");
		vt->u = string::convert<float>(tok.nextToken());
		vt->v = string::convert<float>(tok.nextToken());
		tok.assertNextToken(")");

		// Weight index and count
		vt->weight_index = string::convert<std::size_t>(tok.nextToken());
		vt->weight_count = string::convert<std::size_t>(tok.nextToken());

	} // for each vertex

	// ------  TRIANGLES ------

	// Read the number of triangles
	tok.assertNextToken("numtris");
	std::size_t numTris = string::convert<std::size_t>(tok.nextToken());

	// Initialise the triangle vector
	MD5Tris& tris = mesh.triangles;
	tris.resize(numTris);

	// Read each triangle
	for(MD5Tris::iterator tr = tris.begin(); tr != tris.end(); ++tr) {

		tok.assertNextToken("tri");

		// Triangle index, followed by the indexes of its 3 vertices
		tr->index = string::convert<std::size_t>(tok.nextToken());
		tr->a = 	string::convert<std::size_t>(tok.nextToken());
		tr->b = 	string::convert<std::size_t>(tok.nextToken());
		tr->c = 	string::convert<std::size_t>(tok.nextToken());

	} // for each triangle

	// -----  WEIGHTS ------

	// Read the number of weights
	tok.assertNextToken("numweights");
	std::size_t numWeights = string::convert<std::size_t>(tok.nextToken());

	// Initialise weights vector
	MD5Weights& weights = mesh.weights;
	weights.resize(numWeights);

	// Populate with weight data
	for(MD5Weights::iterator w = weights.begin(); w != weights.end(); ++w) {

		tok.assertNextToken("weight");

		// Index and joint
		w->index = string::convert<std::size_t>(tok.nextToken());
		w->joint = string::convert<std::size_t>(tok.nextToken());

		// Strength and relative position
		w->t = string::convert<float>(tok.nextToken());
		w->v = MD5Model::parseVector3(tok);

	} // for each weight

	// ----- END OF MESH DECL -----

	tok.assertNextToken("}");
}

} // namespace
