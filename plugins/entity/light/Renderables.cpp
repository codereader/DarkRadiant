#include "Renderables.h"

void light_draw_box_lines(const Vector3& origin, const Vector3 points[8]) {
	//draw lines from the center of the bbox to the corners
	glBegin(GL_LINES);

	glVertex3fv(origin);
	glVertex3fv(points[1]);

	glVertex3fv(origin);
	glVertex3fv(points[5]);

	glVertex3fv(origin);
	glVertex3fv(points[2]);

	glVertex3fv(origin);
	glVertex3fv(points[6]);

	glVertex3fv(origin);
	glVertex3fv(points[0]);

	glVertex3fv(origin);
	glVertex3fv(points[4]);

	glVertex3fv(origin);
	glVertex3fv(points[3]);

	glVertex3fv(origin);
	glVertex3fv(points[7]);

	glEnd();
}