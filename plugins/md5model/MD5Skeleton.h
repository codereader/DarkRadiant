#pragma once

#include <vector>
#include "imd5anim.h"

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
	// Update the skeleton to match the given animation at the given time
	void update(const IMD5AnimPtr& anim, std::size_t time);

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
	void updateJointRecursively(std::size_t jointId);
};

} // namespace
