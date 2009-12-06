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

	// Get the translated texture position
	Vector2 newTexPosition = _windingVertex.texcoord + translation;
	
	// Scale the translation with the shader image dimensions
	/*translation[0] *= _sourceFace.getFaceShader().width();
	translation[1] *= _sourceFace.getFaceShader().height();
	
	// Invert the s-translation, the ShiftTexDef does it inversely for some reason. 
	translation[0] *= -1;
	
	// Shift the texdef accordingly
	_sourceFace.shiftTexdef(translation[0], translation[1]);*/

	Vector2 texCentroid;
	 
	for (Winding::const_iterator i = _sourceFace.getWinding().begin(); 
		 i != _sourceFace.getWinding().end(); ++i)
	{
		texCentroid += i->texcoord;
	}
	
	// Take the average value of all the winding texcoords to retrieve the centroid
	texCentroid /= _sourceFace.getWinding().size();

	// Take the centroid as pivot
	Vector2 newDist = newTexPosition - texCentroid;
	Vector2 dist = _windingVertex.texcoord - texCentroid;

	double uniScale = newDist.getLength() / dist.getLength();

	Vector3 windingTranslation(texCentroid.x(), texCentroid.y(), 0);

	Matrix4 texTransform = _sourceFace.getTexdef().m_projection.getTransform();

	Matrix4 translationToOrigin = Matrix4::getTranslation(-windingTranslation);
	Matrix4 translationFromOrigin = Matrix4::getTranslation(windingTranslation);

	Matrix4 scale = Matrix4::getScale(Vector3(newDist.x()/dist.x(), newDist.y()/dist.y(), 0));

	matrix4_premultiply_by_matrix4(texTransform, translationToOrigin);
	matrix4_premultiply_by_matrix4(texTransform, scale);
	matrix4_premultiply_by_matrix4(texTransform, translationFromOrigin);

	//_sourceFace.getTexdef().m_projection.scale(newDist.x()/dist.x(), newDist.y()/dist.y());

	_sourceFace.getTexdef().m_projection.setTransform(1, 1, texTransform);

	_sourceFace.texdefChanged();
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
