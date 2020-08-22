#pragma once

#include <map>
#include <string>
#include "ipatch.h"
#include "ibrush.h"
#include "iscenegraph.h"

namespace scene
{

/**
 * greebo: This object traverses the scenegraph on construction
 * counting all occurrences of each shader.
 */
class ShaderBreakdown :
	public scene::NodeVisitor
{
public:
	struct ShaderCount
	{
		std::size_t faceCount;
		std::size_t patchCount;

		ShaderCount() :
			faceCount(0),
			patchCount(0)
		{}
	};

	typedef std::map<std::string, ShaderCount> Map;

private:
	mutable Map _map;

public:
	ShaderBreakdown()
	{
		_map.clear();
		GlobalSceneGraph().root()->traverseChildren(*this);
	}

	bool pre(const scene::INodePtr& node) override
	{
		// Check if this node is a patch
		if (Node_isPatch(node))
		{
			increaseShaderCount(Node_getIPatch(node)->getShader(), false);
			return false;
		}

		if (Node_isBrush(node))
        {
			auto brush = Node_getIBrush(node);
			
			for (std::size_t i = 0; i < brush->getNumFaces(); ++i)
            {
                increaseShaderCount(brush->getFace(i).getShader(), true);
            }

			return false;
		}

		return true;
	}

	// Accessor method to retrieve the shader breakdown map
	const Map& getMap() const
	{
		return _map;
	}

	Map::const_iterator begin() const
	{
		return _map.begin();
	}

	Map::const_iterator end() const
	{
		return _map.end();
	}

private:
	// Local helper to increase the shader occurrence count
	void increaseShaderCount(const std::string& shaderName, bool isFace)
	{
		// Try to look up the shader in the map
		auto found = _map.find(shaderName);

		if (found == _map.end())
		{
			// Shader not yet registered, create new entry
			auto result = _map.emplace(shaderName, ShaderCount());

			found = result.first;
		}

		// Iterator is valid at this point, increase the counter
		if (isFace)
		{
			found->second.faceCount++;
		}
		else
		{
			found->second.patchCount++;
		}
	}

}; // class ShaderBreakdown

} // namespace
