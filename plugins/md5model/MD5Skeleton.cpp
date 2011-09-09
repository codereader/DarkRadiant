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

	// Calculate the current frame number
	float timePerFrameMsec = 1000 / static_cast<float>(_anim->getFrameRate());
	std::size_t curFrame = static_cast<std::size_t>(static_cast<float>(time) / timePerFrameMsec) % _anim->getNumFrames();

	// Apply the current frame keys to the base frame
	for (std::size_t i = 0; i < numJoints; ++i)
	{
		const Joint& joint = _anim->getJoint(i);
		const IMD5Anim::Key& baseKey = _anim->getBaseFrameKey(joint.id);

		std::size_t key = joint.firstKey;

		// Base frame
		_skeleton[i].origin = baseKey.origin;
		_skeleton[i].orientation = baseKey.orientation;

		// Actual frame data
		const IMD5Anim::FrameKeys& frameKeys = _anim->getFrameKeys(curFrame);

		if (joint.animComponents & Joint::X)
		{
			_skeleton[i].origin.x() = frameKeys[key++];
		}

		if (joint.animComponents & Joint::Y)
		{
			_skeleton[i].origin.y() = frameKeys[key++];
		}

		if (joint.animComponents & Joint::Z)
		{
			_skeleton[i].origin.z() = frameKeys[key++];
		}

		if (joint.animComponents & Joint::YAW)
		{
			_skeleton[i].orientation.x() = frameKeys[key++];
		}

		if (joint.animComponents & Joint::PITCH)
		{
			_skeleton[i].orientation.y() = frameKeys[key++];
		}

		if (joint.animComponents & Joint::ROLL)
		{
			_skeleton[i].orientation.z() = frameKeys[key++];
		}

		if (joint.animComponents & (Joint::YAW | Joint::PITCH | Joint::ROLL))
		{
			// Calculate the fourth component of the quaternion
			float lSq = _skeleton[i].orientation.getVector3().getLengthSquared();
			float w = -sqrt(1.0f - lSq);

			_skeleton[i].orientation.w() = isNaN(w) ? 0 : w;
		}
	}

	for (std::size_t i = 0; i < numJoints; ++i)
	{
		const Joint& joint = _anim->getJoint(i);

		if (joint.parentId == -1)
		{
			updateJointRecursively(i);
		}
	}
}

void MD5Skeleton::updateJointRecursively(std::size_t jointId)
{
	// Reset info to base first
	const Joint& joint = _anim->getJoint(jointId);

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
		updateJointRecursively(*i);
	}
}

} // namespace
