#pragma once

#include "Colour4b.h"

/// 3-element vertex with a 4-byte colour and a normal vector
struct VertexNCb
{
	Vertex3f vertex;
	Colour4b colour;
	Normal3f normal;
};


