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

	int _numFrames;
	int _frameRate;
	int _numAnimatedComponents;

	struct Joint
	{
		std::string name;
		int parentId; // ID of parent bone (-1 == no parent)

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
	};

	std::vector<Joint> _joints;

	// One AABB per frame
	std::vector<AABB> _bounds;

	struct Key
	{
		Vector3 origin;
		Quaternion orientation;
	};
	typedef std::vector<Key> Keys;

	Keys _baseFrame;

	// Each frame has <numAnimatedComponents> float values
	std::vector< std::vector<float> > _frames;

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

	const Joint& getJointById(std::size_t id) const
	{
		return _joints[id];
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
