#pragma once

#include "imd5anim.h"
#include <vector>
#include "parser/DefTokeniser.h"
#include "math/AABB.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"

namespace md5
{

class MD5AnimTokeniser;

class MD5Anim :
	public IMD5Anim
{
private:
	// The command line used to export this md5anim def
	std::string _commandLine;

	int _frameRate;
	int _numAnimatedComponents;

	std::vector<Joint> _joints;

	// One AABB per frame
	std::vector<AABB> _bounds;

	typedef std::vector<IMD5Anim::Key> Keys;

	Keys _baseFrame;

	// Each frame has <numAnimatedComponents> float values
	std::vector<FrameKeys> _frames;

public:
	MD5Anim();

	const std::string& getCommandLine() const
	{
		return _commandLine;
	}

	std::size_t getNumJoints() const
	{
		return _joints.size();
	}

	const Joint& getJoint(std::size_t index) const
	{
		return _joints[index];
	}

	const Key& getBaseFrameKey(std::size_t jointNum) const
	{
		return _baseFrame[jointNum];
	}

	int getFrameRate() const
	{
		return _frameRate;
	}

	std::size_t getNumFrames() const
	{
		return _frames.size();
	}

	const FrameKeys& getFrameKeys(std::size_t index) const
	{
		return _frames[index];
	}

	void parseFromStream(std::istream& stream);

private:
	void parseFromTokens(parser::DefTokeniser& tok);
	void parseJointHierarchy(parser::DefTokeniser& tok);
	void parseFrameBounds(parser::DefTokeniser& tok);
	void parseBaseFrame(parser::DefTokeniser& tok);
	void parseFrame(std::size_t frame, parser::DefTokeniser& tok);
};
typedef boost::shared_ptr<MD5Anim> MD5AnimPtr;

} // namespace
