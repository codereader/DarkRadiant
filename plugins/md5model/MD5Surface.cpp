#include "MD5Surface.h"

#include "ivolumetest.h"
#include "GLProgramAttributes.h"

namespace md5
{

inline VertexPointer vertexpointer_arbitrarymeshvertex(const ArbitraryMeshVertex* array)
{
  return VertexPointer(VertexPointer::pointer(&array->vertex), sizeof(ArbitraryMeshVertex));
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
void MD5Surface::updateGeometry() {
	_aabb_local = AABB();
	for (vertices_t::iterator i = _vertices.begin(); 
		 i != _vertices.end(); 
		 ++i)
	  _aabb_local.includePoint(reinterpret_cast<const Vector3&>(i->vertex));
	
	for (MD5Surface::indices_t::iterator i = _indices.begin(); 
		 i != _indices.end(); 
		 i += 3)
	{
			ArbitraryMeshVertex& a = _vertices[*(i + 0)];
			ArbitraryMeshVertex& b = _vertices[*(i + 1)];
			ArbitraryMeshVertex& c = _vertices[*(i + 2)];
	
	  ArbitraryMeshTriangle_sumTangents(a, b, c);
	}
	
	for (MD5Surface::vertices_t::iterator i = _vertices.begin(); 
		 i != _vertices.end(); 
		 ++i)
	{
	  vector3_normalise(reinterpret_cast<Vector3&>((*i).tangent));
	  vector3_normalise(reinterpret_cast<Vector3&>((*i).bitangent));
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
	for (indices_t::const_iterator i = _indices.begin();
		 i != _indices.end();
		 ++i)
	{
		// Get the vertex for this index
		ArbitraryMeshVertex& v = _vertices[*i];

		// Submit the vertex attributes and coordinate
		if (GLEW_ARB_vertex_program) {
			// Submit the vertex attributes and coordinate
			glVertexAttrib2dvARB(ATTR_TEXCOORD, v.texcoord);
			glVertexAttrib3dvARB(ATTR_TANGENT, v.tangent);
			glVertexAttrib3dvARB(ATTR_BITANGENT, v.bitangent);
			glVertexAttrib3dvARB(ATTR_NORMAL, v.normal);		
		}
		glVertex3dv(v.vertex);	
	}
	glEnd();
	
	glEndList();
	
	// Generate the list for flat-shaded (unlit) mode
	_normalList = glGenLists(1);
	glNewList(_normalList, GL_COMPILE);
	
	glBegin(GL_TRIANGLES);
	for (indices_t::const_iterator i = _indices.begin();
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

void MD5Surface::captureShader() {
	_shader = GlobalRenderSystem().capture(_shaderName);
}

MD5Surface::vertices_t& MD5Surface::vertices() {
	return _vertices;
}

MD5Surface::indices_t& MD5Surface::indices() {
	return _indices;
}

void MD5Surface::setShader(const std::string& name) {
	_shaderName = name;
	_originalShaderName = name;
	captureShader();
}

std::string MD5Surface::getShader() const {
	return _shaderName;
}

ShaderPtr MD5Surface::getState() const {
	return _shader;
}

void MD5Surface::applySkin(const ModelSkin& skin) {
	// Look up the remap for this surface's material name. If there is a remap
	// change the Shader* to point to the new shader.
	std::string remap = skin.getRemap(_originalShaderName);

	if (!remap.empty()) {
		// Save the remapped shader name
		_shaderName = remap; 
	}
	else {
		// No remap, so reset our shader to the original unskinned shader	
		_shaderName = _originalShaderName; 
	}

	captureShader();
}

const AABB& MD5Surface::localAABB() const {
	return _aabb_local;
}

void MD5Surface::render(RenderableCollector& collector, const Matrix4& localToWorld, ShaderPtr state) const {
	collector.SetState(state, RenderableCollector::eFullMaterials);
	collector.addRenderable(*this, localToWorld);
}

void MD5Surface::render(RenderableCollector& collector, const Matrix4& localToWorld) const {
	render(collector, localToWorld, _shader);
}

} // namespace md5
