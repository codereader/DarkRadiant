#include "MD5Skeleton.h"

namespace md5
{

void MD5Skeleton::update(const IMD5AnimPtr& anim, std::size_t time)
{
	_anim = anim;

	// Update the joint positions, recursively, starting from the first
	// Only root nodes need to be processed, the children are reached through them
	std::size_t numJoints = _anim ? _anim->getNumJoints() : 0;

	// Ensure the correct size
	if (_skeleton.size() != numJoints)
	{
		_skeleton.resize(numJoints);
	}

	for (std::size_t i = 0; i < numJoints; ++i)
	{
		const Joint& joint = _anim->getJoint(i);

		if (joint.parentId == -1)
		{
			updateJointRecursively(i, time);
		}
	}
}

void MD5Skeleton::updateJointRecursively(std::size_t jointId, std::size_t time)
{
	// Reset info to base first
	const Joint& joint = _anim->getJoint(jointId);
	const IMD5Anim::Key& baseKey = _anim->getBaseFrameKey(joint.id);

	_skeleton[joint.id].origin = baseKey.origin;
	_skeleton[joint.id].orientation = baseKey.orientation;

	if (joint.parentId >= 0)
	{
		// Joint has a parent, update this position and rotation
		_skeleton[joint.id].orientation.preMultiplyBy(_skeleton[joint.parentId].orientation);

		// Transform the origin of this joint using the rotation of the parent joint
		_skeleton[joint.id].origin = _skeleton[joint.parentId].orientation.transformPoint(_skeleton[joint.id].origin);
			
		// Apply the parent joint's translation to this child bone
		_skeleton[joint.id].origin += _skeleton[joint.parentId].origin;
	}

	// Update all children as well
	for (std::vector<int>::const_iterator i = joint.children.begin(); i != joint.children.end(); ++i)
	{
		updateJointRecursively(*i, time);
	}
}

} // namespace
