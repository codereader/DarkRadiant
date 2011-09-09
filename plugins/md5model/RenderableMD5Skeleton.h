#pragma once

#include "irender.h"

namespace md5
{

class RenderableMD5Skeleton :
	public OpenGLRenderable
{
private:
	IMD5AnimPtr _anim;

	RenderSystemWeakPtr _renderSystem;

	// The position and orientation of the animated joints at the current time
	std::vector<IMD5Anim::Key> _skeleton;

public:
	void setAnim(const IMD5AnimPtr& anim)
	{
		_anim = anim;

		_skeleton.resize(_anim ? _anim->getNumJoints() : 0);
	}

	void update()
	{
		if (!_anim) return;

		RenderSystemPtr renderSystem = _renderSystem.lock();

		if (renderSystem)
		{
			std::size_t time = renderSystem->getTime();

			// Update the joint positions, recursively, starting from the first
			std::size_t numJoints = _anim->getNumJoints();

			for (std::size_t i = 0; i < numJoints; ++i)
			{
				const Joint& joint = _anim->getJoint(i);

				if (joint.parentId == -1)
				{
					updateJointRecursively(joint, time);
				}
			}
		}
	}

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		_renderSystem = renderSystem;
	}

	void render(const RenderInfo& info) const
	{
		if (!_anim) return;

		glBegin(GL_LINES);

		std::size_t numJoints = _anim->getNumJoints();

		for (std::size_t i = 0; i < _skeleton.size(); ++i)
		{
			const IMD5Anim::Key& bone = _skeleton[i];
			const Joint& joint = _anim->getJoint(i);
			
			if (joint.parentId != -1)
			{
				const IMD5Anim::Key& parentBone = _skeleton[joint.parentId];

				glVertex3fv(parentBone.origin);
				glVertex3fv(bone.origin);
			}
			else
			{
				glVertex3f(0, 0, 0);
				glVertex3fv(bone.origin);
			}
		}

		for (std::size_t i = 0; i < numJoints; ++i)
		{
			const IMD5Anim::Key& joint = _skeleton[i];
			
			Vector3 x(2,0,0);
			Vector3 y(0,2,0);
			Vector3 z(0,0,2);

			x = joint.orientation.transformPoint(x);
			y = joint.orientation.transformPoint(y);
			z = joint.orientation.transformPoint(z);

			Vector3 origin(joint.origin);

			glColor3f(1, 0, 0);
			glVertex3fv(origin);
			glVertex3fv(origin + x);

			glColor3f(0, 1, 0);
			glVertex3fv(origin);
			glVertex3fv(origin + y);

			glColor3f(0, 0, 1);
			glVertex3fv(origin);
			glVertex3fv(origin + z);
		}

		glEnd();
	}

private:
	void updateJointRecursively(const Joint& joint, std::size_t time)
	{
		// Reset info to base first
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
			updateJointRecursively(_anim->getJoint(*i), time);
		}
	}
};

} // namespace
