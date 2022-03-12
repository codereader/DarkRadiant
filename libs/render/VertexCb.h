#pragma once

#include "Vertex3f.h"
#include "Colour4b.h"

/// Vertex with a 4-byte colour value (0 - 255)
class VertexCb
{
public:
	Colour4b colour;
	Vertex3 vertex;

    /// Default constructor, leaves all values uninitialised
    VertexCb() { }

    /// Initialise with position and colour
	VertexCb(const Vertex3& _vertex, const Colour4b& _colour) :
		colour(_colour),
		vertex(_vertex)
	{}

    /// Initialise with position and default white colour
	VertexCb(const Vector3& vector) :
		colour(255, 255, 255, 255),
		vertex(vector)
	{}

	bool operator< (const VertexCb& other) const {
		if (vertex != other.vertex) {
			return vertex < other.vertex;
		}

		if (colour != other.colour) {
			return colour < other.colour;
		}

		return false;
	}

	bool operator== (const VertexCb& other) const {
		return colour == other.colour && vertex == other.vertex;
	}

	bool operator!= (const VertexCb& other) const {
		return !(*this == other);
	}
};
