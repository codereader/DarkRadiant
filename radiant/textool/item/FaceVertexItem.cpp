#include "FaceVertexItem.h"

namespace textool
{

// Constructor, allocates all child FacItems
FaceVertexItem::FaceVertexItem(Face& sourceFace, WindingVertex& windingVertex) :
	_sourceFace(sourceFace),
	_windingVertex(windingVertex)
{}

void FaceVertexItem::beginTransformation()
{
	_sourceFace.undoSave();
	_saved = _windingVertex.texcoord;
}

bool FaceVertexItem::testSelect(const Rectangle& rectangle)
{
	return rectangle.contains(_windingVertex.texcoord);
}

void FaceVertexItem::transform(const Matrix4& matrix)
{
	// Pick the translation components from the matrix and apply the translation
	Vector2 translation(matrix.tx(), matrix.ty());
	
	// Scale the translation with the shader image dimensions
	translation[0] *= _sourceFace.getFaceShader().width();
	translation[1] *= _sourceFace.getFaceShader().height();
	
	// Invert the s-translation, the ShiftTexDef does it inversely for some reason. 
	translation[0] *= -1;
	
	// Shift the texdef accordingly
	_sourceFace.shiftTexdef(translation[0], translation[1]);
}

void FaceVertexItem::render()
{
	glPointSize(5);

	glBegin(GL_POINTS);

	if (isSelected())
	{
		glColor3f(1, 0.2f, 0.2f);
	}
	else
	{
		glColor3f(1, 1, 1);
	}

	glVertex2dv(_windingVertex.texcoord);

	glEnd();

	// Call the base class
	TexToolItem::render();
}
	
} // namespace textool
