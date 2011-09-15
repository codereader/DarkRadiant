#pragma once

#include <set>
#include <map>
#include <string>

#include "iselection.h"
#include "math/Vector3.h"
#include "brush/TextureProjection.h"

#include "selection/algorithm/Primitives.h"

namespace brush {
namespace algorithm {

class BrushByPlaneClipper
{
private:
	const Vector3& _p0;
	const Vector3& _p1;
	const Vector3& _p2;
	TextureProjection _projection;
	EBrushSplit _split;

	// Whether to use the _caulkShader texture for new brush faces
	bool _useCaulk;

	// The shader name used for new faces when _useCaulk is true
	std::string _caulkShader;

	mutable std::string _mostUsedShader;
	mutable TextureProjection _mostUsedProjection;

	mutable std::set<scene::INodePtr> _deleteList;

	typedef std::map<scene::INodePtr, scene::INodePtr> InsertMap;
	mutable InsertMap _insertList;

public:
	BrushByPlaneClipper(const Vector3& p0, const Vector3& p1, const Vector3& p2,
						const TextureProjection& projection, EBrushSplit split);

	void split(const BrushPtrVector& brushes);

private:
	void getMostUsedTexturing(const Brush& brush) const;
};

} // namespace algorithm
} // namespace brush
