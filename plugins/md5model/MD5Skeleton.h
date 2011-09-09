#pragma once

namespace md5
{

/**
 * This object represents a joint hierarchy as used
 * by animated MD5 models. At any point in time
 * the joints (bones) have a defined origin and orientation.
 */
class MD5Skeleton
{
protected:
	// The position and orientation of the animated joints at the current time
	std::vector<IMD5Anim::Key> _skeleton;

	// The current animation, needed to get joint information etc.
	IMD5AnimPtr _anim;

public:
	void update(const IMD5AnimPtr& anim, std::size_t time)
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

	std::size_t size() const
	{
		return _skeleton.size();
	}

	const IMD5Anim::Key& getKey(std::size_t jointIndex) const
	{
		return _skeleton[jointIndex];
	}

	const Joint& getJoint(std::size_t index) const
	{
		return _anim->getJoint(index);
	}

private:

	void updateJointRecursively(std::size_t jointId, std::size_t time)
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
};

} // namespace
