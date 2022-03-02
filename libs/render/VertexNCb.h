#pragma once

#include "Vertex3f.h"
#include "Colour4b.h"

/// 3-element vertex with a 4-byte colour and a normal vector
struct VertexNCb
{
	Vertex3 vertex;
	Colour4b colour;
	Normal3 normal;
};


