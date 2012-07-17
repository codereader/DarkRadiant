#ifndef _PARTICLE_QUAD_H_
#define _PARTICLE_QUAD_H_

namespace particles
{

/**
 * Each particle stage consists of a bunch of quads.
 * A quad in turn consists of 4 vertices, each of them carrying
 * 3D coordinates, a normal vector, texture coords and a vertex colour.
 */
struct ParticleQuad
{
	struct Vertex
	{
		Vector3 vertex;			// The 3D coordinates of the point
		Vector2 texcoord;		// The UV coordinates
		Vector3 normal;			// The normals
		Vector4 colour;		// vertex colour

		Vertex()
		{}

		Vertex(const Vector3& vertex_, const Vector2& texcoord_) :
			vertex(vertex_),
			texcoord(texcoord_),
			normal(0,0,1),
			colour(1,1,1,1)
		{}

		Vertex(const Vector3& vertex_, const Vector2& texcoord_, const Vector4& colour_, const Vector3& normal_) :
			vertex(vertex_),
			texcoord(texcoord_),
			normal(normal_),
			colour(colour_)
		{}
	};

	Vertex verts[4];

	ParticleQuad()
	{}

	ParticleQuad(float size)
	{
		verts[0] = Vertex(Vector3(-size, +size, 0), Vector2(0,0));
		verts[1] = Vertex(Vector3(+size, +size, 0), Vector2(1,0));
		verts[2] = Vertex(Vector3(+size, -size, 0), Vector2(1,1));
		verts[3] = Vertex(Vector3(-size, -size, 0), Vector2(0,1));
	}

	/**
	 * Create a new quad, using the given size and angle.
	 * Specify an optional vertex colour which is assigned to all four corners.
	 *
	 * [Optional]: s0 and sWidth are used for particle animation frames.
	 *
	 * @aspect: scales the horizontal coords by this factor.
	 * @s0: defines the horizontal frame start coordinate in texture space (s).
	 * @sWidth: defines the width of this frame in texture space.
	 */
	ParticleQuad(float size, float aspect, float angle, const Vector4& colour = Vector4(1,1,1,1),
				 const Vector3& normal = Vector3(0,0,1),
				 float s0 = 0.0f, float sWidth = 1.0f, float t0 = 0.0f, float tWidth = 1.0f)
	{
		double cosPhi = cos(degrees_to_radians(angle));
		double sinPhi = sin(degrees_to_radians(angle));
		Matrix4 rotation = Matrix4::byColumns(
			cosPhi, -sinPhi, 0, 0,
			sinPhi, cosPhi, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);

		verts[0] = Vertex(rotation.transformPoint(Vector3(-size*aspect, +size, 0)), Vector2(s0,t0), colour, normal);
		verts[1] = Vertex(rotation.transformPoint(Vector3(+size*aspect, +size, 0)), Vector2(s0 + sWidth,t0), colour, normal);
		verts[2] = Vertex(rotation.transformPoint(Vector3(+size*aspect, -size, 0)), Vector2(s0 + sWidth,t0 + tWidth), colour, normal);
		verts[3] = Vertex(rotation.transformPoint(Vector3(-size*aspect, -size, 0)), Vector2(s0,t0 + tWidth), colour, normal);
	}

	void translate(const Vector3& offset)
	{
		verts[0].vertex += offset;
		verts[1].vertex += offset;
		verts[2].vertex += offset;
		verts[3].vertex += offset;
	}

	void transform(const Matrix4& mat)
	{
		verts[0].vertex = mat.transformPoint(verts[0].vertex);
		verts[1].vertex = mat.transformPoint(verts[1].vertex);
		verts[2].vertex = mat.transformPoint(verts[2].vertex);
		verts[3].vertex = mat.transformPoint(verts[3].vertex);
	}

	void assignColour(const Vector4& colour)
	{
		verts[0].colour = colour;
		verts[1].colour = colour;
		verts[2].colour = colour;
		verts[3].colour = colour;
	}

	// Sets the horizontal texture coordinates, the quad will use the interval [s0..s0+sWidth]
	void setHorizTexCoords(float s0, float sWidth)
	{
		verts[0].texcoord[0] = s0;
		verts[1].texcoord[0] = s0 + sWidth;
		verts[2].texcoord[0] = s0 + sWidth;
		verts[3].texcoord[0] = s0;
	}
};

} // namespace

#endif /* _PARTICLE_QUAD_H_ */
