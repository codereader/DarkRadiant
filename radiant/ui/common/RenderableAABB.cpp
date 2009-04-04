#include "RenderableAABB.h"

#include <GL/glew.h>

namespace ui
{

// Virtual render function from OpenGLRenderable

// OpenGL render function

void RenderableAABB::render(const RenderInfo& info) const {

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
