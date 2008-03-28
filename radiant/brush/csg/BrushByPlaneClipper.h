#ifndef BRUSH_BY_PLANE_CLIPPER_H_
#define BRUSH_BY_PLANE_CLIPPER_H_

#include <set>
#include <map>
#include <string>

#include "iselection.h"
#include "math/Vector3.h"
#include "brush/TextureProjection.h"

namespace brush {
namespace algorithm {

class BrushByPlaneClipper : 
	public SelectionSystem::Visitor
{
	const Vector3& _p0;
	const Vector3& _p1;
	const Vector3& _p2;
	std::string _shader;
	TextureProjection _projection;
	EBrushSplit _split;

	mutable std::string _mostUsedShader;
	mutable TextureProjection _mostUsedProjection;

	mutable std::set<scene::INodePtr> _deleteList;

	typedef std::map<scene::INodePtr, scene::INodePtr> InsertMap;
	mutable InsertMap _insertList;

public:
	BrushByPlaneClipper(const Vector3& p0, const Vector3& p1, const Vector3& p2, 
							  const std::string& shader, 
							  const TextureProjection& projection, EBrushSplit split);

	// The destructor performs the node deletions and insertions
	~BrushByPlaneClipper();

	// SelectionSystem::Visitor implementation
	void visit(const scene::INodePtr& node) const;

private:
	void getMostUsedTexturing(const Brush* brush) const;
};

} // namespace algorithm
} // namespace brush 

#endif /* BRUSH_BY_PLANE_CLIPPER_H_ */
