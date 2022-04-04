#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "render/MeshVertex.h"
#include "render/VertexHashing.h"

namespace model
{

class FbxSurface
{
private:
	std::vector<unsigned int> indices;
	std::vector<MeshVertex> vertices;
	std::string material;

	// Hash index to share vertices with the same set of attributes
	std::unordered_map<MeshVertex, std::size_t> vertexIndices;

public:
	std::vector<MeshVertex>& getVertexArray()
	{
		return vertices;
	}

	std::vector<unsigned int>& getIndexArray()
	{
		return indices;
	}

	const std::string& getMaterial() const
	{
		return material;
	}

    void setMaterial(const std::string& newMaterial)
    {
        material = newMaterial;
    }

	void addVertex(const MeshVertex& vertex)
	{
		// Try to look up an existing vertex or add a new index
		auto emplaceResult = vertexIndices.try_emplace(vertex, vertices.size());

		if (emplaceResult.second)
		{
			// This was a new vertex, copy it to the vertex array
			vertices.emplace_back(emplaceResult.first->first);
		}

		// The emplaceResult now points to a valid index in the vertex array
		indices.emplace_back(static_cast<unsigned int>(emplaceResult.first->second));
	}
};

}
