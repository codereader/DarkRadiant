#ifndef ECLASSMODELINSTANCE_H_
#define ECLASSMODELINSTANCE_H_

#include "renderable.h"
#include "transformlib.h"
#include "generic/callbackfwd.h"

#include "../targetable.h"
#include "EclassModel.h"

namespace entity {

class EclassModelInstance :
	public TargetableInstance,
	public TransformModifier,
	public Renderable
{
	EclassModel& m_contained;

	mutable bool _updateSkin;
public:
	EclassModelInstance(const scene::Path& path, scene::Instance* parent, EclassModel& contained);
	
	~EclassModelInstance();
	
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

	void evaluateTransform();
	void applyTransform();
	typedef MemberCaller<EclassModelInstance, &EclassModelInstance::applyTransform> ApplyTransformCaller;

	void skinChanged(const std::string& value);
	typedef MemberCaller1<EclassModelInstance, const std::string&, &EclassModelInstance::skinChanged> SkinChangedCaller;
};

} // namespace entity

#endif /*ECLASSMODELINSTANCE_H_*/
