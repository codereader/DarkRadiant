#include "MD5Model.h"

#include "ishaders.h"
#include "texturelib.h"
#include "ifilter.h"
#include "string/string.h"
#include "MD5DataStructures.h"

namespace md5 {

MD5Model::MD5Model() :
	_polyCount(0),
	_vertexCount(0)
{}

MD5Model::const_iterator MD5Model::begin() const {
	return _surfaces.begin();
}

MD5Model::const_iterator MD5Model::end() const {
	return _surfaces.end();
}

std::size_t MD5Model::size() const {
	return _surfaces.size();
}

MD5Surface& MD5Model::newSurface() {
	_surfaces.push_back(MD5SurfacePtr(new MD5Surface));
	return *_surfaces.back();
}

void MD5Model::updateAABB() {
	_aabb_local = AABB();
	for(SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i) {
		_aabb_local.includeAABB((*i)->localAABB());
	}
}

VolumeIntersectionValue MD5Model::intersectVolume(
	const VolumeTest& test, const Matrix4& localToWorld) const
{
	return test.TestAABB(_aabb_local, localToWorld);
}

const AABB& MD5Model::localAABB() const {
	return _aabb_local;
}

void MD5Model::testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld) {
	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i) {
		if ((*i)->intersectVolume(test.getVolume(), localToWorld) != c_volumeOutside) {
			(*i)->testSelect(selector, test, localToWorld);
		}
	}
}

std::string MD5Model::getFilename() const {
	return _filename;
}

void MD5Model::setFilename(const std::string& name) {
	_filename = name;
}

std::string MD5Model::getModelPath() const {
	return _modelPath;
}

void MD5Model::setModelPath(const std::string& modelPath) {
	_modelPath = modelPath;
}

void MD5Model::applySkin(const ModelSkin& skin) {
	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i) {
		(*i)->applySkin(skin);
	}

	updateMaterialList();
}

int MD5Model::getSurfaceCount() const {
	return static_cast<int>(size());
}

int MD5Model::getVertexCount() const {
	return static_cast<int>(_vertexCount);
}

int MD5Model::getPolyCount() const {
	return static_cast<int>(_polyCount);
}

void MD5Model::updateMaterialList() {
	_surfaceNames.clear();

	for (SurfaceList::const_iterator i = _surfaces.begin();
		 i != _surfaces.end();
		 ++i)
	{
		_surfaceNames.push_back((*i)->getShader());
	}
}

const std::vector<std::string>& MD5Model::getActiveMaterials() const {
	return _surfaceNames;
}

void MD5Model::render(const RenderInfo& info) const {
	// Render options
	if (info.checkFlag(RENDER_TEXTURE_2D))
		glEnable(GL_TEXTURE_2D);
	if (info.checkFlag(RENDER_SMOOTH))
		glShadeModel(GL_SMOOTH);

	for (SurfaceList::const_iterator i = _surfaces.begin(); i != _surfaces.end(); ++i) {
		// Get the Material to test the shader name against the filter system
		MaterialPtr surfaceShader = (*i)->getState()->getMaterial();
		if (surfaceShader->isVisible()) {
			// Bind the OpenGL texture and render the surface geometry
			TexturePtr tex = surfaceShader->getEditorImage();
			glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());
			(*i)->render(info.getFlags());
		}
	}
}

void MD5Model::parseFromTokens(parser::DefTokeniser& tok) {
	_vertexCount = 0;
	_polyCount = 0;

	// Check the version number
	tok.assertNextToken("MD5Version");
	tok.assertNextToken("10");

	// Commandline
	tok.assertNextToken("commandline");
	tok.skipTokens(1); // quoted command string

	// Number of joints and meshes
	tok.assertNextToken("numJoints");
	std::size_t numJoints = strToSizet(tok.nextToken());
	tok.assertNextToken("numMeshes");
	std::size_t numMeshes = strToSizet(tok.nextToken());

	// ------ JOINTS  ------

	// Start of joints datablock
	tok.assertNextToken("joints");
	tok.assertNextToken("{");

	// Initialise the Joints vector with the specified number of objects
	MD5Joints joints(numJoints);

	// Iterate over the vector of Joints, filling in each one with parsed
	// values
	for(MD5Joints::iterator i = joints.begin(); i != joints.end(); ++i) {

		// Skip the joint name
		tok.skipTokens(1);
		
		// Index of parent joint
		i->parent = strToInt(tok.nextToken());
		
		// Joint's position vector
		i->position = parseVector3(tok);

	    // Parse joint's rotation
		Vector3 rawRotation = parseVector3(tok);

	    // Calculate the W value. If it is NaN (due to underflow in the sqrt),
	    // set it to 0.
	    double lSq = rawRotation.getLengthSquared();
	    double w = -sqrt(1.0 - lSq);
	    if (isNaN(w)) {
	    	w = 0;
	    }
	
		// Set the Vector4 rotation on the joint
	    i->rotation = Vector4(rawRotation, w);
	}

	// End of joints datablock
	tok.assertNextToken("}");

	// ------ MESHES ------

	// For each mesh, there should be a mesh datablock
	for (std::size_t i = 0; i < numMeshes; ++i) {
		// Start of datablock
		tok.assertNextToken("mesh");
		tok.assertNextToken("{");
		
		// Construct the surface for this mesh
		MD5Surface& surface = newSurface();

		// Get the shader name
		tok.assertNextToken("shader");
		surface.setShader(tok.nextToken());

		// ----- VERTICES ------

		// Read the vertex count
		tok.assertNextToken("numverts");
	    std::size_t numVerts = strToSizet(tok.nextToken());	

		// Initialise the vertex vector
		MD5Verts verts(numVerts);

		// Update the vertexcount
		_vertexCount += numVerts;

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

		// Update the polycount
		_polyCount += numTris;

		// Initialise the triangle vector
		MD5Tris tris(numTris);

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
		MD5Weights weights(numWeights);

		// Populate with weight data
		for(MD5Weights::iterator w = weights.begin(); w != weights.end(); ++w) {

			tok.assertNextToken("weight");

			// Index and joint
			w->index = strToSizet(tok.nextToken());
			w->joint = strToSizet(tok.nextToken());

			// Strength and direction (?)
			w->t = strToFloat(tok.nextToken());
			w->v = parseVector3(tok);

		} // for each weight
		
		// ----- END OF MESH DECL -----
		
		tok.assertNextToken("}");

		// ------ CALCULATION ------

		for (MD5Verts::iterator j = verts.begin(); j != verts.end(); ++j) {
			MD5Vert& vert = (*j);

			Vector3 skinned(0, 0, 0);
			for (std::size_t k = 0; k != vert.weight_count; ++k) {
				MD5Weight& weight = weights[vert.weight_index + k];
				MD5Joint& joint = joints[weight.joint];

				Vector3 rotatedPoint = quaternion_transformed_point(
						joint.rotation, weight.v);
				skinned += (rotatedPoint + joint.position) * weight.t;
			}

			surface.vertices().push_back(ArbitraryMeshVertex(Vertex3f(skinned),
					Normal3f(0, 0, 0), TexCoord2f(vert.u, vert.v)));
		}

		for (MD5Tris::iterator j = tris.begin(); j != tris.end(); ++j) {
			MD5Tri& tri = (*j);
			surface.indices().insert(RenderIndex(tri.a));
			surface.indices().insert(RenderIndex(tri.b));
			surface.indices().insert(RenderIndex(tri.c));
		}

		for (MD5Surface::indices_t::iterator j = surface.indices().begin(); j != surface.indices().end(); j += 3) {
			ArbitraryMeshVertex& a = surface.vertices()[*(j + 0)];
			ArbitraryMeshVertex& b = surface.vertices()[*(j + 1)];
			ArbitraryMeshVertex& c = surface.vertices()[*(j + 2)];
			Vector3 weightedNormal((c.vertex - a.vertex).crossProduct(b.vertex - a.vertex) );
			a.normal += weightedNormal;
			b.normal += weightedNormal;
			c.normal += weightedNormal;
		}

		for (MD5Surface::vertices_t::iterator j = surface.vertices().begin(); j != surface.vertices().end(); ++j) {
			j->normal = Normal3f(j->normal.getNormalised());
		}

		surface.updateGeometry();

	} // for each mesh

	updateAABB();
	updateMaterialList();
}

Vector3 MD5Model::parseVector3(parser::DefTokeniser& tok) {
	tok.assertNextToken("(");

	double x = strToDouble(tok.nextToken());
	double y = strToDouble(tok.nextToken());
	double z = strToDouble(tok.nextToken());

	tok.assertNextToken(")");
	
	return Vector3(x, y, z);
}

} // namespace md5

/** greebo: The commented out stuff below are remnants from an MD5Anim parser,
 *          left for later reference.
 */

//bool MD5Anim_parse(Tokeniser& tokeniser)
//{
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseVersion(tokeniser));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "commandline"));
//  const char* commandline;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseString(tokeniser, commandline));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "numFrames"));
//  std::size_t numFrames;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, numFrames));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "numJoints"));
//  std::size_t numJoints;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, numJoints));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "frameRate"));
//  std::size_t frameRate;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, frameRate));
//  tokeniser.nextLine();
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "numAnimatedComponents"));
//  std::size_t numAnimatedComponents;
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, numAnimatedComponents));
//  tokeniser.nextLine();
//
//  // parse heirarchy
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "hierarchy"));
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "{"));
//  tokeniser.nextLine();
//
//  for(std::size_t i = 0; i < numJoints; ++i)
//  {
//    const char* name;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseString(tokeniser, name));
//    int parent;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseInteger(tokeniser, parent));
//    std::size_t flags;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, flags));
//    std::size_t index;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseSize(tokeniser, index));
//    tokeniser.nextLine();
//  }
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "}"));
//  tokeniser.nextLine();
//
//  // parse bounds
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "bounds"));
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "{"));
//  tokeniser.nextLine();
//
//  for(std::size_t i = 0; i < numFrames; ++i)
//  {
//    Vector3 mins;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseVector3(tokeniser, mins));
//    Vector3 maxs;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseVector3(tokeniser, maxs));
//    tokeniser.nextLine();
//  }
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "}"));
//  tokeniser.nextLine();
//
//  // parse baseframe
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "baseframe"));
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "{"));
//  tokeniser.nextLine();
//
//  for(std::size_t i = 0; i < numJoints; ++i)
//  {
//    Vector3 position;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseVector3(tokeniser, position));
//    Vector3 rotation;
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseVector3(tokeniser, rotation));
//    tokeniser.nextLine();
//  }
//
//  MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "}"));
//  tokeniser.nextLine();
//
//  // parse frames
//  for(std::size_t i = 0; i < numFrames; ++i)
//  {
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "frame"));
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "{"));
//    tokeniser.nextLine();
//
//    for(std::size_t i = 0; i < numAnimatedComponents; ++i)
//    {
//      float component;
//      MD5_RETURN_FALSE_IF_FAIL(MD5_parseFloat(tokeniser, component));
//      tokeniser.nextLine();
//    }
//
//    MD5_RETURN_FALSE_IF_FAIL(MD5_parseToken(tokeniser, "}"));
//    tokeniser.nextLine();
//  }
//
//  return true;
//}
