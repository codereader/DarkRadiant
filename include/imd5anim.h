#pragma once

#include "imodule.h"

#include <vector>
#include "math/Vector3.h"
#include "math/Quaternion.h"

namespace md5
{

struct Joint
{
	// The ID of this joint
	int id;

	// the name of this bone
	std::string name;

	// ID of parent bone (-1 == no parent)
	int parentId; 

	// The 6 possible components that might be modified by a key
	enum AnimComponent 
	{
		X		= 1 << 0, 
		Y		= 1 << 1, 
		Z		= 1 << 2,
		YAW		= 1 << 3, 
		PITCH	= 1 << 4, 
		ROLL	= 1 << 5,
		INVALID_COMPONENT	= 1 << 6
	};

	// A bit mask explaining which components we're animating
	std::size_t animComponents;

	std::size_t firstKey;

	// The child joint IDs
	std::vector<int> children;
};

/** 
 * Base class for an MD5 animation as used in Doom 3.
 */
class IMD5Anim 
{
public:
	// Information used by the base frame
	struct Key
	{
		Vector3 origin;
		Quaternion orientation;
	};
	
	// Each frame has a series of float values, applied to one or more animated components (x, y, z, yaw, pitch, roll)
	typedef std::vector<float> FrameKeys;

	/**
	 * Get the number of joints in this animation.
	 */
	virtual std::size_t getNumJoints() const = 0;

	/** 
	 * Retrieve a certain joint by index, bounds are [0..getNumJoints())
	 */
	virtual const Joint& getJoint(std::size_t index) const = 0;

	/**
	 * Returns the baseframe info for the given joint number.
	 */
	virtual const Key& getBaseFrameKey(std::size_t jointNum) const = 0;

	/**
	 * Returns the frame rate this anim should be played.
	 */
	virtual int getFrameRate() const = 0;

	/**
	 * Returns the number of frames in this animation.
	 */
	virtual std::size_t getNumFrames() const = 0;

	/**
	 * Returns the float values of the given frame index.
	 */
	virtual const FrameKeys& getFrameKeys(std::size_t index) const = 0;
};
typedef boost::shared_ptr<IMD5Anim> IMD5AnimPtr;

class IAnimationCache :
	public RegisterableModule
{
public:
	/**
	 * Returns the MD5 animation for the given VFS path, or NULL if 
	 * the file does not exist or the anim was found to be invalid.
	 */
	virtual IMD5AnimPtr getAnim(const std::string& vfsPath) = 0;
};

const char* const MODULE_ANIMATIONCACHE("MD5AnimationCache");

} // namespace

inline md5::IAnimationCache& GlobalAnimationCache()
{
	// Cache the reference locally
	static md5::IAnimationCache& _animationCache(
		*boost::static_pointer_cast<md5::IAnimationCache>(
			module::GlobalModuleRegistry().getModule(md5::MODULE_ANIMATIONCACHE)
		)
	);
	return _animationCache;
}
