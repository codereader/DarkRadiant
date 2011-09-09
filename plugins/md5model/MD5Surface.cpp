#include "MD5Surface.h"

#include "ivolumetest.h"
#include "GLProgramAttributes.h"
#include "string/string.h"
#include "MD5Model.h"

namespace md5
{

inline VertexPointer vertexpointer_arbitrarymeshvertex(const ArbitraryMeshVertex* array)
{
  return VertexPointer(&array->vertex, sizeof(ArbitraryMeshVertex));
}

// Constructor
MD5Surface::MD5Surface()
: _shaderName(""),
  _originalShaderName(""),
  _normalList(0),
  _lightingList(0)
{}

// Destructor
MD5Surface::~MD5Surface() {
	// Release GL display lists
	glDeleteLists(_normalList, 1);
	glDeleteLists(_lightingList, 1);
}

// Update geometry
void MD5Surface::updateGeometry()
{
	_aabb_local = AABB();

	for (Vertices::const_iterator i = _vertices.begin(); i != _vertices.end(); ++i)
	{
		_aabb_local.includePoint(i->vertex);
	}

	for (Indices::iterator i = _indices.begin();
		 i != _indices.end();
		 i += 3)
	{
		ArbitraryMeshVertex& a = _vertices[*(i + 0)];
		ArbitraryMeshVertex& b = _vertices[*(i + 1)];
		ArbitraryMeshVertex& c = _vertices[*(i + 2)];

		ArbitraryMeshTriangle_sumTangents(a, b, c);
	}

	for (Vertices::iterator i = _vertices.begin();
		 i != _vertices.end();
		 ++i)
	{
		i->tangent.normalise();
		i->bitangent.normalise();
	}

	// Build the display lists
	createDisplayLists();
}

// Back-end render
void MD5Surface::render(const RenderInfo& info) const
{
	if (info.checkFlag(RENDER_BUMP))
    {
		glCallList(_lightingList);
	}
	else
    {
		glCallList(_normalList);
	}
}

// Construct the display lists
void MD5Surface::createDisplayLists()
{
	// Create the list for lighting mode
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
		if (GLEW_ARB_vertex_program) {
			// Submit the vertex attributes and coordinate
			glVertexAttrib2fvARB(ATTR_TEXCOORD, v.texcoord);
			glVertexAttrib3fvARB(ATTR_TANGENT, v.tangent);
			glVertexAttrib3fvARB(ATTR_BITANGENT, v.bitangent);
			glVertexAttrib3fvARB(ATTR_NORMAL, v.normal);
		}
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

// Selection test
void MD5Surface::testSelect(Selector& selector,
							SelectionTest& test,
							const Matrix4& localToWorld)
{
	test.BeginMesh(localToWorld);

	SelectionIntersection best;
	test.TestTriangles(
	  vertexpointer_arbitrarymeshvertex(_vertices.data()),
	  IndexPointer(_indices.data(), IndexPointer::index_type(_indices.size())),
	  best
	);

	if(best.valid()) {
		selector.addIntersection(best);
	}
}

void MD5Surface::captureShader()
{
	RenderSystemPtr renderSystem = _renderSystem.lock();

	if (renderSystem)
	{
		// Capture current shader
		_shader = renderSystem->capture(_shaderName);
	}
	else
	{
		// Free shaders
		_shader.reset();
	}
}

void MD5Surface::setShader(const std::string& name)
{
	_shaderName = name;
	_originalShaderName = name;
	captureShader();
}

const ShaderPtr& MD5Surface::getState() const
{
	return _shader;
}

void MD5Surface::applySkin(const ModelSkin& skin)
{
	// Look up the remap for this surface's material name. If there is a remap
	// change the Shader* to point to the new shader.
	std::string remap = skin.getRemap(_originalShaderName);

	if (!remap.empty())
	{
		// Save the remapped shader name
		_shaderName = remap;
	}
	else
	{
		// No remap, so reset our shader to the original unskinned shader
		_shaderName = _originalShaderName;
	}

	captureShader();
}

const AABB& MD5Surface::localAABB() const {
	return _aabb_local;
}

void MD5Surface::render(RenderableCollector& collector, const Matrix4& localToWorld, 
						const IRenderEntity& entity) const
{
	assert(_shader); // shader must be captured at this point

	collector.SetState(_shader, RenderableCollector::eFullMaterials);
	collector.addRenderable(*this, localToWorld, entity);
}

void MD5Surface::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	_renderSystem = renderSystem;
	
	// Attempt to capture the shader now (or free them if renderSystem is NULL)
	captureShader();
}

int MD5Surface::getNumVertices() const
{
	return static_cast<int>(_vertices.size());
}

int MD5Surface::getNumTriangles() const
{
	return static_cast<int>(_indices.size() / 3);
}

const ArbitraryMeshVertex& MD5Surface::getVertex(int vertexIndex) const
{
	assert(vertexIndex >= 0 && vertexIndex < static_cast<int>(_vertices.size()));
	return _vertices[vertexIndex];
}

model::ModelPolygon MD5Surface::getPolygon(int polygonIndex) const
{
	assert(polygonIndex >= 0 && polygonIndex*3 < _indices.size());

	model::ModelPolygon poly;

	poly.a = _vertices[_indices[polygonIndex*3]];
	poly.b = _vertices[_indices[polygonIndex*3 + 1]];
	poly.c = _vertices[_indices[polygonIndex*3 + 2]];

	return poly;
}

const std::string& MD5Surface::getDefaultMaterial() const
{
	return _originalShaderName;
}

const std::string& MD5Surface::getActiveMaterial() const
{
	return _shaderName;
}

void MD5Surface::updateToDefaultPose(const MD5Joints& joints)
{
	MD5Verts& verts = _mesh.vertices;
	MD5Weights& weights= _mesh.weights;
	MD5Tris& tris= _mesh.triangles;

	_vertices.clear();

	for (MD5Verts::iterator j = verts.begin(); j != verts.end(); ++j)
	{
		MD5Vert& vert = (*j);

		Vector3 skinned(0, 0, 0);

		for (std::size_t k = 0; k != vert.weight_count; ++k)
		{
			MD5Weight& weight = weights[vert.weight_index + k];
			const MD5Joint& joint = joints[weight.joint];

			Vector3 rotatedPoint = joint.rotation.transformPoint(weight.v);
			skinned += (rotatedPoint + joint.position) * weight.t;
		}

		_vertices.push_back(
			ArbitraryMeshVertex(skinned, Normal3f(0, 0, 0), TexCoord2f(vert.u, vert.v))
		);
	}

	// Ensure the index array is ok
	if (_indices.empty())
	{
		buildIndexArray();
	}

	for (Indices::iterator j = _indices.begin(); j != _indices.end(); j += 3)
	{
		ArbitraryMeshVertex& a = _vertices[*(j + 0)];
		ArbitraryMeshVertex& b = _vertices[*(j + 1)];
		ArbitraryMeshVertex& c = _vertices[*(j + 2)];

		Vector3 weightedNormal((c.vertex - a.vertex).crossProduct(b.vertex - a.vertex));

		a.normal += weightedNormal;
		b.normal += weightedNormal;
		c.normal += weightedNormal;
	}

	// Normalise all normal vectors
	for (Vertices::iterator j = _vertices.begin(); j != _vertices.end(); ++j)
	{
		j->normal = Normal3f(j->normal.getNormalised());
	}

	updateGeometry();
}

void MD5Surface::buildIndexArray()
{
	_indices.clear();

	// Build the indices based on the triangle information
	for (MD5Tris::const_iterator j = _mesh.triangles.begin(); j != _mesh.triangles.end(); ++j)
	{
		const MD5Tri& tri = (*j);

		_indices.insert(static_cast<RenderIndex>(tri.a));
		_indices.insert(static_cast<RenderIndex>(tri.b));
		_indices.insert(static_cast<RenderIndex>(tri.c));
	}
}

void MD5Surface::parseFromTokens(parser::DefTokeniser& tok)
{
	// Start of datablock
	tok.assertNextToken("mesh");
	tok.assertNextToken("{");

	// Get the reference to the mesh definition
	MD5Mesh& mesh = _mesh;

	// Get the shader name
	tok.assertNextToken("shader");
	setShader(tok.nextToken());

	// ----- VERTICES ------

	// Read the vertex count
	tok.assertNextToken("numverts");
	std::size_t numVerts = strToSizet(tok.nextToken());

	// Initialise the vertex vector
	MD5Verts& verts = mesh.vertices;
	verts.resize(numVerts);

	// Populate each vertex struct with parsed values
	for (MD5Verts::iterator vt = verts.begin(); vt != verts.end(); ++vt) {

		tok.assertNextToken("vert");

		// Index of vert
		vt->index = strToSizet(tok.nextToken());

		// U and V texcoords
		tok.assertNextToken("(");
		vt->u = strToFloat(tok.nextToken());
		vt->v = strToFloat(tok.nextToken());
		tok.assertNextToken(")");

		// Weight index and count
		vt->weight_index = strToSizet(tok.nextToken());
		vt->weight_count = strToSizet(tok.nextToken());

	} // for each vertex

	// ------  TRIANGLES ------

	// Read the number of triangles
	tok.assertNextToken("numtris");
	std::size_t numTris = strToSizet(tok.nextToken());

	// Initialise the triangle vector
	MD5Tris& tris = mesh.triangles;
	tris.resize(numTris);

	// Read each triangle
	for(MD5Tris::iterator tr = tris.begin(); tr != tris.end(); ++tr) {

		tok.assertNextToken("tri");

		// Triangle index, followed by the indexes of its 3 vertices
		tr->index = strToSizet(tok.nextToken());
		tr->a = 	strToSizet(tok.nextToken());
		tr->b = 	strToSizet(tok.nextToken());
		tr->c = 	strToSizet(tok.nextToken());

	} // for each triangle

	// -----  WEIGHTS ------

	// Read the number of weights
	tok.assertNextToken("numweights");
	std::size_t numWeights = strToSizet(tok.nextToken());

	// Initialise weights vector
	MD5Weights& weights = mesh.weights;
	weights.resize(numWeights);

	// Populate with weight data
	for(MD5Weights::iterator w = weights.begin(); w != weights.end(); ++w) {

		tok.assertNextToken("weight");

		// Index and joint
		w->index = strToSizet(tok.nextToken());
		w->joint = strToSizet(tok.nextToken());

		// Strength and relative position
		w->t = strToFloat(tok.nextToken());
		w->v = MD5Model::parseVector3(tok);

	} // for each weight

	// ----- END OF MESH DECL -----

	tok.assertNextToken("}");
}

} // namespace md5
