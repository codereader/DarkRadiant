#include "MD5Parser.h"

#include "MD5DataStructures.h"
#include "string/string.h"

namespace md5 {

MD5Parser::MD5Parser(std::istream& inputStream) :
	parser::BasicDefTokeniser<std::istream>(inputStream)
{}

void MD5Parser::parseToModel(MD5Model& model) {
	// Check the version number
	assertNextToken("MD5Version");
	assertNextToken("10");

	// Commandline
	assertNextToken("commandline");
	skipTokens(1); // quoted command string

	// Number of joints and meshes
	assertNextToken("numJoints");
	std::size_t numJoints = strToSizet(nextToken());
	assertNextToken("numMeshes");
	std::size_t numMeshes = strToSizet(nextToken());

	// ------ JOINTS  ------

	// Start of joints datablock
	assertNextToken("joints");
	assertNextToken("{");

	// Initialise the Joints vector with the specified number of objects
	MD5Joints joints(numJoints);

	// Iterate over the vector of Joints, filling in each one with parsed
	// values
	for(MD5Joints::iterator i = joints.begin(); i != joints.end(); ++i) {

		// Skip the joint name
		skipTokens(1);
		
		// Index of parent joint
		i->parent = strToInt(nextToken());
		
		// Joint's position vector
		i->position = parseVector3();

	    // Parse joint's rotation
		Vector3 rawRotation = parseVector3();

	    // Calculate the W value. If it is NaN (due to underflow in the sqrt),
	    // set it to 0.
	    double lSq = rawRotation.getLengthSquared();
	    float w = -sqrt(1.0 - lSq);
	    if (isNaN(w)) {
	    	w = 0;
	    }
	
		// Set the Vector4 rotation on the joint
	    i->rotation = Vector4(rawRotation, w);
	}

	// End of joints datablock
	assertNextToken("}");

	// ------ MESHES ------

	// For each mesh, there should be a mesh datablock
	for (std::size_t i = 0; i < numMeshes; ++i) {
		// Start of datablock
		assertNextToken("mesh");
		assertNextToken("{");
		
		// Construct the surface for this mesh
		MD5Surface& surface = model.newSurface();

		// Get the shader name
		assertNextToken("shader");
		surface.setShader(nextToken());

		// ----- VERTICES ------

		// Read the vertex count
		assertNextToken("numverts");
	    std::size_t numVerts = strToSizet(nextToken());	

		// Initialise the vertex vector
		MD5Verts verts(numVerts);

		// Populate each vertex struct with parsed values
		for (MD5Verts::iterator vt = verts.begin(); vt != verts.end(); ++vt) {
			
			assertNextToken("vert");

			// Index of vert
			vt->index = strToSizet(nextToken());

			// U and V texcoords
			assertNextToken("(");
			vt->u = strToFloat(nextToken());
			vt->v = strToFloat(nextToken());
			assertNextToken(")");

			// Weight index and count
			vt->weight_index = strToSizet(nextToken());
			vt->weight_count = strToSizet(nextToken());
		
		} // for each vertex
	
		// ------  TRIANGLES ------
	
		// Read the number of triangles
		assertNextToken("numtris");
		std::size_t numTris = strToSizet(nextToken());

		// Initialise the triangle vector
		MD5Tris tris(numTris);

		// Read each triangle
		for(MD5Tris::iterator tr = tris.begin(); tr != tris.end(); ++tr) {

			assertNextToken("tri");

			// Triangle index, followed by the indexes of its 3 vertices
			tr->index = strToSizet(nextToken());
			tr->a = 	strToSizet(nextToken());
			tr->b = 	strToSizet(nextToken());
			tr->c = 	strToSizet(nextToken());

		} // for each triangle

		// -----  WEIGHTS ------

		// Read the number of weights
		assertNextToken("numweights");
		std::size_t numWeights = strToSizet(nextToken());

		// Initialise weights vector
		MD5Weights weights(numWeights);

		// Populate with weight data
		for(MD5Weights::iterator w = weights.begin(); w != weights.end(); ++w) {

			assertNextToken("weight");

			// Index and joint
			w->index = strToSizet(nextToken());
			w->joint = strToSizet(nextToken());

			// Strength and direction (?)
			w->t = strToFloat(nextToken());
			w->v = parseVector3();

		} // for each weight
		
		// ----- END OF MESH DECL -----
		
		assertNextToken("}");

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

	model.updateAABB();
}

Vector3 MD5Parser::parseVector3() {
	assertNextToken("(");

	double x = strToDouble(nextToken());
	double y = strToDouble(nextToken());
	double z = strToDouble(nextToken());

	assertNextToken(")");
	
	return Vector3(x, y, z);
}

} // namespace md5
