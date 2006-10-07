#include "RenderableAABB.h"

#include <GL/glew.h>

namespace ui
{

// Virtual render function from OpenGLRenderable

// OpenGL render function

void RenderableAABB::render(RenderStateFlags flags) const {

	// Wireframe cuboid
	glBegin(GL_LINES);
		glVertex3f(_aabb.extents.x(), _aabb.extents.y(), _aabb.extents.z());
		glVertex3f(_aabb.extents.x(), _aabb.extents.y(), -_aabb.extents.z());

		glVertex3f(_aabb.extents.x(), _aabb.extents.y(), _aabb.extents.z());
		glVertex3f(-_aabb.extents.x(), _aabb.extents.y(), _aabb.extents.z());

		glVertex3f(_aabb.extents.x(), _aabb.extents.y(), -_aabb.extents.z());
		glVertex3f(-_aabb.extents.x(), _aabb.extents.y(), -_aabb.extents.z());

		glVertex3f(_aabb.extents.x(), _aabb.extents.y(), _aabb.extents.z());
		glVertex3f(_aabb.extents.x(), -_aabb.extents.y(), _aabb.extents.z());

		glVertex3f(-_aabb.extents.x(), _aabb.extents.y(), _aabb.extents.z());
		glVertex3f(-_aabb.extents.x(), -_aabb.extents.y(), _aabb.extents.z());

		glVertex3f(-_aabb.extents.x(), _aabb.extents.y(), -_aabb.extents.z());
		glVertex3f(-_aabb.extents.x(), -_aabb.extents.y(), -_aabb.extents.z());

		glVertex3f(_aabb.extents.x(), _aabb.extents.y(), -_aabb.extents.z());
		glVertex3f(_aabb.extents.x(), -_aabb.extents.y(), -_aabb.extents.z());

		glVertex3f(_aabb.extents.x(), -_aabb.extents.y(), _aabb.extents.z());
		glVertex3f(-_aabb.extents.x(), -_aabb.extents.y(), _aabb.extents.z());

		glVertex3f(_aabb.extents.x(), -_aabb.extents.y(), _aabb.extents.z());
		glVertex3f(_aabb.extents.x(), -_aabb.extents.y(), -_aabb.extents.z());

		glVertex3f(-_aabb.extents.x(), _aabb.extents.y(), _aabb.extents.z());
		glVertex3f(-_aabb.extents.x(), _aabb.extents.y(), -_aabb.extents.z());

		glVertex3f(-_aabb.extents.x(), -_aabb.extents.y(), _aabb.extents.z());
		glVertex3f(-_aabb.extents.x(), -_aabb.extents.y(), -_aabb.extents.z());

		glVertex3f(_aabb.extents.x(), -_aabb.extents.y(), -_aabb.extents.z());
		glVertex3f(-_aabb.extents.x(), -_aabb.extents.y(), -_aabb.extents.z());
	glEnd();
}


}
