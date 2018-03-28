#pragma once

#include "irender.h"
#include "MD5Skeleton.h"

namespace md5
{

class RenderableMD5Skeleton :
	public OpenGLRenderable
{
private:
	const MD5Skeleton& _skeleton;

public:
	RenderableMD5Skeleton(const MD5Skeleton& skeleton) :
		_skeleton(skeleton)
	{}

	void render(const RenderInfo& info) const
	{
		if (_skeleton.size() == 0) return;

		glBegin(GL_LINES);

		std::size_t numJoints = _skeleton.size();

		for (std::size_t i = 0; i < _skeleton.size(); ++i)
		{
			const IMD5Anim::Key& bone = _skeleton.getKey(i);
			const Joint& joint = _skeleton.getJoint(i);
			
			if (joint.parentId != -1)
			{
				const IMD5Anim::Key& parentBone = _skeleton.getKey(joint.parentId);

				glVertex3dv(parentBone.origin);
				glVertex3dv(bone.origin);
			}
			else
			{
				glVertex3d(0, 0, 0);
				glVertex3dv(bone.origin);
			}
		}

		for (std::size_t i = 0; i < numJoints; ++i)
		{
			const IMD5Anim::Key& joint = _skeleton.getKey(i);
			
			Vector3 x(2,0,0);
			Vector3 y(0,2,0);
			Vector3 z(0,0,2);

			x = joint.orientation.transformPoint(x);
			y = joint.orientation.transformPoint(y);
			z = joint.orientation.transformPoint(z);

			Vector3 origin(joint.origin);

			glColor3f(1, 0, 0);
			glVertex3dv(origin);
			glVertex3dv(origin + x);

			glColor3f(0, 1, 0);
			glVertex3dv(origin);
			glVertex3dv(origin + y);

			glColor3f(0, 0, 1);
			glVertex3dv(origin);
			glVertex3dv(origin + z);
		}

		glEnd();
	}
};

} // namespace
