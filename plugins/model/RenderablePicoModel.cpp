#include "RenderablePicoModel.h"

namespace model {

// Virtual render function

void RenderablePicoModel::render(RenderStateFlags flags) const {
		// Test model.
		glBegin(GL_QUADS);
			// Top
			glColor3f(1, 0, 0); glNormal3f(0, 1, 0);
			glVertex3f(1, 1, 1);
			glVertex3f(1, 1, -1);
			glVertex3f(-1, 1, -1);
			glVertex3f(-1, 1, 1);
			// Front
			glColor3f(1, 1, 0); glNormal3f(0, 0, 1);
			glVertex3f(1, 1, 1);
			glVertex3f(-1, 1, 1);
			glVertex3f(-1, -1, 1);
			glVertex3f(1, -1, 1);
			// Right
			glColor3f(0, 1, 0); glNormal3f(1, 0, 0);
			glVertex3f(1, 1, 1);
			glVertex3f(1, -1, 1);
			glVertex3f(1, -1, -1);
			glVertex3f(1, 1, -1);
			// Left
			glColor3f(0, 1, 1); glNormal3f(-1, 0, 0);
			glVertex3f(-1, 1, 1);
			glVertex3f(-1, 1, -1);
			glVertex3f(-1, -1, -1);
			glVertex3f(-1, -1, 1);
			// Bottom
			glColor3f(0, 0, 1); glNormal3f(0, -1, 0);
			glVertex3f(1, -1, 1);
			glVertex3f(-1, -1, 1);
			glVertex3f(-1, -1, -1);
			glVertex3f(1, -1, -1);
			// Back
			glColor3f(1, 0, 1); glNormal3f(0, 0, -1);
			glVertex3f(1, 1, -1);
			glVertex3f(1, -1, -1);
			glVertex3f(-1, -1, -1);
			glVertex3f(-1, 1, -1);
		glEnd();
}
	
}
