#include "MD5Skeleton.h"

#include <cmath>

namespace md5
{

namespace
{
	// greebo: this code has been mostly taken from the web, with some additional fixes on my behalf and the D3 SDK
	inline Quaternion slerp(const Quaternion& qa, const Quaternion& qb, float fraction)
	{
		// quaternion to return
		Quaternion qm;

		// Calculate angle between them.
		float cosHalfTheta = qa.w() * qb.w() + qa.x() * qb.x() + qa.y() * qb.y() + qa.z() * qb.z();

		// if qa=qb or qa=-qb then theta = 0 and we can return qa
		if (abs(cosHalfTheta) > 1.0f)
		{
 			return qb;
		}

		// greebo: I spotted this fix in the D3 SDK - sometimes we run into rotations
		// of theta being almost 2*pi which can lead to huge rotational steps (~90 degrees)
		// in a single frame - use this to rectify that.
		Quaternion temp;

		if (cosHalfTheta < 0.0f)
		{
			temp = qb*(-1);
			cosHalfTheta = -cosHalfTheta;
		} 
		else
		{
			temp = qb;
		}

		// Calculate temporary values.
		float halfTheta = acos(cosHalfTheta);
		float sinHalfTheta = sqrt(1.0f - cosHalfTheta*cosHalfTheta);

		// if theta = 180 degrees then result is not fully defined
		// we could rotate around any axis normal to qa or qb
		if (fabs(sinHalfTheta) < 0.006f)
		{ 
			qm.w() = (qa.w() * (1-fraction) + temp.w() * fraction);
			qm.x() = (qa.x() * (1-fraction) + temp.x() * fraction);
			qm.y() = (qa.y() * (1-fraction) + temp.y() * fraction);
			qm.z() = (qa.z() * (1-fraction) + temp.z() * fraction);
			return qm;
		}

		float ratioA = sin((1 - fraction) * halfTheta) / sinHalfTheta;
		float ratioB = sin(fraction * halfTheta) / sinHalfTheta; 

		//calculate Quaternion.
		qm.w() = (qa.w() * ratioA + temp.w() * ratioB);
		qm.x() = (qa.x() * ratioA + temp.x() * ratioB);
		qm.y() = (qa.y() * ratioA + temp.y() * ratioB);
		qm.z() = (qa.z() * ratioA + temp.z() * ratioB);

		return qm;
	}
}

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
	
	float frameTime = time / timePerFrameMsec;

	// Pre-calculate the weighting of each frame
	float nextFrameFrac = float_mod(frameTime, 1.0f);
	float curFrameFrac = 1.0f - nextFrameFrac;

	std::size_t curFrame = static_cast<std::size_t>(std::floor(frameTime)) % _anim->getNumFrames();
	std::size_t nextFrame = curFrame == _anim->getNumFrames() -1 ? curFrame : (curFrame + 1) % _anim->getNumFrames();

	// Apply the current frame keys to the base frame
	for (std::size_t i = 0; i < numJoints; ++i)
	{
		const Joint& joint = _anim->getJoint(i);
		const IMD5Anim::Key& baseKey = _anim->getBaseFrameKey(joint.id);

		// Apply base frame
		_skeleton[i].origin = baseKey.origin;
		_skeleton[i].orientation = baseKey.orientation;
		
		// Apply actual frame data
		const IMD5Anim::FrameKeys& cur = _anim->getFrameKeys(curFrame);
		const IMD5Anim::FrameKeys& next = _anim->getFrameKeys(nextFrame);

		// The joint.firstKey member holds the offset into the frame data array
		std::size_t key = joint.firstKey;

		// Shortcuts for handling the rotations
		Quaternion& orientation = _skeleton[i].orientation;
		Quaternion nextOrientation = baseKey.orientation;

		// Animate each vector component, interpolating values in between frames

		if (joint.animComponents & Joint::X)
		{
			_skeleton[i].origin.x() = cur[key]*curFrameFrac + next[key]*nextFrameFrac;
			key++;
		}

		if (joint.animComponents & Joint::Y)
		{
			_skeleton[i].origin.y() = cur[key]*curFrameFrac + next[key]*nextFrameFrac;
			key++;
		}

		if (joint.animComponents & Joint::Z)
		{
			_skeleton[i].origin.z() = cur[key]*curFrameFrac + next[key]*nextFrameFrac;
			key++;
		}

		if (joint.animComponents & Joint::YAW)
		{
			orientation.x() = cur[key];
			nextOrientation.x() = next[key];
			key++;
		}

		if (joint.animComponents & Joint::PITCH)
		{
			orientation.y() = cur[key];
			nextOrientation.y() = next[key];
			key++;
		}

		if (joint.animComponents & Joint::ROLL)
		{
			orientation.z() = cur[key];
			nextOrientation.z() = next[key];
			key++;
		}

		if (joint.animComponents & (Joint::YAW | Joint::PITCH | Joint::ROLL))
		{
			float lSq = orientation.getVector3().getLengthSquared();
			float w = -sqrt(1.0f - lSq);

			orientation.w() = isNaN(w) ? 0 : w;

			lSq = nextOrientation.getVector3().getLengthSquared();
			w = -sqrt(1.0f - lSq);

			nextOrientation.w() = isNaN(w) ? 0 : w;

			orientation = slerp(orientation, nextOrientation, nextFrameFrac).getNormalised();
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
