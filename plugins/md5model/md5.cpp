/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "MD5Surface.h"
#include "md5.h"
#include "MD5ModelLoader.h"

#include "math/FloatTools.h"
#include "imodel.h"
#include "archivelib.h"

#include "parser/DefTokeniser.h"

#include <boost/lexical_cast.hpp>

namespace {
	
/*
 * Parse an MD5 vector, which consists of three separated floats enclosed with
 * parentheses.
 */
Vector3 parseVector3(parser::DefTokeniser& tok)
{
	using boost::lexical_cast;
	
	tok.assertNextToken("(");

	double x = lexical_cast<double>(tok.nextToken());
	double y = lexical_cast<double>(tok.nextToken());
	double z = lexical_cast<double>(tok.nextToken());

	tok.assertNextToken(")");
	
	return Vector3(x, y, z);
}

} // namespace

/**
 * Data structure containing MD5 Joint information.
 */
struct MD5Joint
{
	int parent;
	Vector3 position;
	Vector4 rotation;
};

typedef std::vector<MD5Joint> MD5Joints;

#ifdef _DEBUG

// Stream insertion for MD5Joint
std::ostream& operator<< (std::ostream& os, const MD5Joint& jt) {
	os << "MD5Joint { parent=" << jt.parent
	   << " position=" << jt.position
	   << " rotation=" << jt.rotation
	   << " }";
	return os;
}

#endif

/**
 * Data structure containing MD5 Vertex information. Vertices do not contain 
 * their own positional information, but are instead attached to joints
 * according to one or more "weight" parameters.
 */
struct MD5Vert
{
	std::size_t index;
	float u;
	float v;
	std::size_t weight_index;
	std::size_t weight_count;
};

typedef std::vector<MD5Vert> MD5Verts;

/**
 * Data structure containing MD5 triangle information. A triangle connects
 * three vertices, addressed by index.
 */
struct MD5Tri
{
	std::size_t index;
	std::size_t a;
	std::size_t b;
	std::size_t c;
};

typedef std::vector<MD5Tri> MD5Tris;

/**
 * Data structure containing weight information.
 */
struct MD5Weight
{
	std::size_t index;
	std::size_t joint;
	float t;
	Vector3 v;
};

typedef std::vector<MD5Weight> MD5Weights;

typedef float MD5Component;
typedef std::vector<MD5Component> MD5Components;

class MD5Frame
{
public:
	MD5Components m_components;
};

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

/**
 * Main parse function for an MD5MESH file.
 */
bool MD5Model_parse(md5::MD5Model& model, parser::DefTokeniser& tok)
{
	using boost::lexical_cast;
	
	// Check the version number
	tok.assertNextToken("MD5Version");
	tok.assertNextToken("10");

	// Commandline
	tok.assertNextToken("commandline");
	tok.skipTokens(1); // quoted command string

	// Number of joints and meshes
	tok.assertNextToken("numJoints");
	std::size_t numJoints = lexical_cast<std::size_t>(tok.nextToken());
	tok.assertNextToken("numMeshes");
	std::size_t numMeshes = lexical_cast<std::size_t>(tok.nextToken());

	/* JOINTS */

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
		i->parent = lexical_cast<int>(tok.nextToken());
		
		// Joint's position vector
		i->position = parseVector3(tok);

	    // Parse joint's rotation
		Vector3 rawRotation = parseVector3(tok);

	    // Calculate the W value. If it is NaN (due to underflow in the sqrt),
	    // set it to 0.
	    double lSq = rawRotation.getLengthSquared();
	    float w = -sqrt(1.0 - lSq);
	    if (isNaN(w))
	    	w = 0;
	
		// Set the Vector4 rotation on the joint
	    i->rotation = Vector4(rawRotation, w); 
	    
	}

	// End of joints datablock
	tok.assertNextToken("}");

	/* MESHES */

	// For each mesh, there should be a mesh datablock
	for(std::size_t i = 0; i < numMeshes; ++i) {
		
		// Start of datablock
		tok.assertNextToken("mesh");
		tok.assertNextToken("{");
		
		// Construct the surface for this mesh
		md5::MD5Surface& surface = model.newSurface();

		// Get the shader name
		tok.assertNextToken("shader");
		surface.setShader(tok.nextToken());

		/* VERTICES */

		// Read the vertex count
		tok.assertNextToken("numverts");
	    std::size_t numVerts = lexical_cast<std::size_t>(tok.nextToken());	

		// Initialise the vertex vector
		MD5Verts verts(numVerts);

		// Populate each vertex struct with parsed values
		for(MD5Verts::iterator vt = verts.begin(); vt != verts.end(); ++vt) {
			
			tok.assertNextToken("vert");

			// Index of vert
			vt->index = lexical_cast<std::size_t>(tok.nextToken());

			// U and V texcoords
			tok.assertNextToken("(");
			vt->u = lexical_cast<float>(tok.nextToken());
			vt->v = lexical_cast<float>(tok.nextToken());
			tok.assertNextToken(")");

			// Weight index and count
			vt->weight_index = lexical_cast<std::size_t>(tok.nextToken());
			vt->weight_count = lexical_cast<std::size_t>(tok.nextToken());
		
		} // for each vertex
	
		/* TRIANGLES */
	
		// Read the number of triangles
		tok.assertNextToken("numtris");
		std::size_t numTris = lexical_cast<std::size_t>(tok.nextToken());

		// Initialise the triangle vector
		MD5Tris tris(numTris);

		// Read each triangle
		for(MD5Tris::iterator tr = tris.begin(); tr != tris.end(); ++tr) {

			tok.assertNextToken("tri");

			// Triangle index, followed by the indexes of its 3 vertices
			tr->index = lexical_cast<std::size_t>(tok.nextToken());
			tr->a = 	lexical_cast<std::size_t>(tok.nextToken());
			tr->b = 	lexical_cast<std::size_t>(tok.nextToken());
			tr->c = 	lexical_cast<std::size_t>(tok.nextToken());

		} // for each triangle

		/* WEIGHTS */

		// Read the number of weights
		tok.assertNextToken("numweights");
		std::size_t numWeights = lexical_cast<std::size_t>(tok.nextToken());

		// Initialise weights vector
		MD5Weights weights(numWeights);

		// Populate with weight data
		for(MD5Weights::iterator w = weights.begin(); w != weights.end(); ++w) {

			tok.assertNextToken("weight");

			// Index and joint
			w->index = lexical_cast<std::size_t>(tok.nextToken());
			w->joint = lexical_cast<std::size_t>(tok.nextToken());

			// Strength and direction (?)
			w->t = lexical_cast<float>(tok.nextToken());
			w->v = parseVector3(tok);

		} // for each weight
		
		/* END OF MESH DECL */
		
		tok.assertNextToken("}");

		/* CALCULATION */

    for(MD5Verts::iterator j = verts.begin(); j != verts.end(); ++j)
    {
      MD5Vert& vert = (*j);

      Vector3 skinned(0, 0, 0);
      for(std::size_t k = 0; k != vert.weight_count; ++k)
      {
        MD5Weight& weight = weights[vert.weight_index + k];
        MD5Joint& joint = joints[weight.joint];
	
		Vector3 rotatedPoint = quaternion_transformed_point(joint.rotation, 
															weight.v);
        skinned += (rotatedPoint + joint.position) * weight.t;
      }
      
		surface.vertices().push_back(ArbitraryMeshVertex(Vertex3f(skinned), Normal3f(0, 0, 0), TexCoord2f(vert.u, vert.v)));
    }

    for(MD5Tris::iterator j = tris.begin(); j != tris.end(); ++j)
    {
      MD5Tri& tri = (*j);
      surface.indices().insert(RenderIndex(tri.a));
      surface.indices().insert(RenderIndex(tri.b));
      surface.indices().insert(RenderIndex(tri.c));
    }

    for(md5::MD5Surface::indices_t::iterator j = surface.indices().begin(); j != surface.indices().end(); j += 3)
    {
		ArbitraryMeshVertex& a = surface.vertices()[*(j + 0)];
		ArbitraryMeshVertex& b = surface.vertices()[*(j + 1)];
		ArbitraryMeshVertex& c = surface.vertices()[*(j + 2)];
		Vector3 weightedNormal( (c.vertex - a.vertex).crossProduct(b.vertex - a.vertex) );
		a.normal += weightedNormal;
		b.normal += weightedNormal;
		c.normal += weightedNormal;
    }

    for(md5::MD5Surface::vertices_t::iterator j = surface.vertices().begin(); j != surface.vertices().end(); ++j)
    {
    	j->normal = Normal3f(j->normal.getNormalised());
      //vector3_normalise(reinterpret_cast<Vector3&>((*j).normal));
    }

    surface.updateGeometry();

	} // for each mesh

	model.updateAABB();

	return true;
}

void MD5Model_construct(md5::MD5Model& model, TextInputStream& inputStream)
{
	// Construct a DefTokeniser and start parsing
	try {
		std::istream is(&inputStream);
		parser::BasicDefTokeniser<std::istream> tok(is);
		MD5Model_parse(model, tok);
	}
	catch (parser::ParseException e) {
		globalErrorStream() << "[md5model] Parse failure. Exception was:\n"
							<< e.what() << "\n";		
	}
}

scene::INodePtr MD5Model_new(TextInputStream& inputStream)
{
	scene::INodePtr node(new md5::MD5ModelNode());
	md5::MD5ModelNode* modelNode = static_cast<md5::MD5ModelNode*>(node.get());
	MD5Model_construct(modelNode->model(), inputStream);
	return node;
}

scene::INodePtr loadMD5Model(ArchiveFile& file)
{
  BinaryToTextInputStream<InputStream> inputStream(file.getInputStream());
  return MD5Model_new(inputStream);
}
