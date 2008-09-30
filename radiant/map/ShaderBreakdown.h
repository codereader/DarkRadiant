#ifndef SHADERBREAKDOWN_H_
#define SHADERBREAKDOWN_H_

#include <map>
#include <string>
#include "scenelib.h"
#include "ipatch.h"
#include "ibrush.h"
#include "patch/Patch.h"
#include "brush/Brush.h"

namespace map {

/** 
 * greebo: This object traverses the scenegraph on construction
 * counting all occurrences of each shader. 
 */
class ShaderBreakdown :
	public scene::NodeVisitor,
	public BrushVisitor
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
	ShaderBreakdown() {
		_map.clear();
		GlobalSceneGraph().root()->traverse(*this);
	}
	
	bool pre(const scene::INodePtr& node) {

		// Check if this node is a patch
		Patch* patch = Node_getPatch(node);

		if (patch != NULL) {
			increaseShaderCount(patch->GetShader(), false);
			return false;
		}

		Brush* brush = Node_getBrush(node);

		if (brush != NULL) {
			brush->forEachFace(*this);
			return false;
		}

		return true;
	}

	// Brushvisitor implementation
	void visit(Face& face) const {
		const_cast<ShaderBreakdown*>(this)->increaseShaderCount(face.GetShader(), true);
	}

	// Accessor method to retrieve the entity breakdown map
	Map getMap() {
		return _map;
	}
	
	Map::const_iterator begin() const {
		return _map.begin();
	}
	
	Map::const_iterator end() const {
		return _map.end();
	}

private:
	// Local helper to increase the shader occurrence count
	void increaseShaderCount(const std::string& shaderName, bool isFace) {
		// Try to look up the shader in the map
		Map::iterator found = _map.find(shaderName);

		if (found == _map.end()) {
			// Shader not yet registered, create new entry
			std::pair<Map::iterator, bool> result = _map.insert(
				Map::value_type(shaderName, ShaderCount())
			);

			found = result.first;
		}

		// Iterator is valid at this point, increase the counter
		if (isFace) {
			found->second.faceCount++;
		}
		else {
			found->second.patchCount++;
		}
	}

}; // class ShaderBreakdown

} // namespace map

#endif /* SHADERBREAKDOWN_H_ */
