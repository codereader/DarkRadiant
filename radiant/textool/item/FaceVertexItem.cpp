#include "FaceVertexItem.h"

#include "iregistry.h"
#include "../TexTool.h"

namespace textool
{

	namespace
	{
		// Returns the pivotCandidate if it's distance is larger to texCoord than the one of curPivot
		inline Vector2 getFurthestPivot(const Vector2& texcoord, const Vector2& curPivot, const Vector2& pivotCandidate)
		{
			double curDist = (texcoord - curPivot).getLengthSquared();
			double otherDist = (texcoord - pivotCandidate).getLengthSquared();

			return (curDist < otherDist) ? pivotCandidate : curPivot;
		}
	}

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

Vector2 FaceVertexItem::getTexCentroid()
{
	Vector2 texCentroid(0,0);

	for (Winding::const_iterator i = _sourceFace.getWinding().begin(); 
		 i != _sourceFace.getWinding().end(); ++i)
	{
		texCentroid += i->texcoord;
	}
	
	// Take the average value of all the winding texcoords to retrieve the centroid
	texCentroid /= _sourceFace.getWinding().size();

	return texCentroid;
}

AABB FaceVertexItem::getTexAABB()
{
	AABB aabb;

	for (Winding::const_iterator i = _sourceFace.getWinding().begin(); 
		 i != _sourceFace.getWinding().end(); ++i)
	{
		aabb.includePoint(Vector3(i->texcoord.x(), i->texcoord.y(), 0));
	}
	
	return aabb;
}

void FaceVertexItem::transform(const Matrix4& matrix)
{
	// Pick the translation components from the matrix and apply the translation
	Vector2 translation(matrix.tx(), matrix.ty());

	// Get the translated texture position
	Vector2 newTexPosition = _windingVertex.texcoord + translation;
	
	// Construct the pivot
	Vector2 pivot;

	// Check if the pivot 
	if (GlobalRegistry().get(ui::RKEY_FACE_VERTEX_SCALE_PIVOT_IS_CENTROID) == "1")
	{
		pivot = getTexCentroid();
	}
	else
	{
		// Take the farthest point of the texture AABB as pivot
		AABB texAABB = getTexAABB();

		Vector2 boxOrigin(texAABB.getOrigin().x(), texAABB.getOrigin().y());
		Vector2 boxExtentsS(texAABB.getExtents().x(), 0);
		Vector2 boxExtentsT(0, texAABB.getExtents().y());

		pivot = boxOrigin + boxExtentsS + boxExtentsT;

		pivot = getFurthestPivot(_windingVertex.texcoord, pivot, boxOrigin - boxExtentsS + boxExtentsT);
		pivot = getFurthestPivot(_windingVertex.texcoord, pivot, boxOrigin - boxExtentsS - boxExtentsT);
		pivot = getFurthestPivot(_windingVertex.texcoord, pivot, boxOrigin + boxExtentsS - boxExtentsT);
	}

	// Take the centroid as pivot
	Vector2 newDist = newTexPosition - pivot;
	Vector2 dist = _windingVertex.texcoord - pivot;

	// First, move the texture to the 0,0 origin in UV space
	// Second, apply the scale
	// Third, move the texture back to where it was, the pivot remains unchanged
	Vector3 pivotTranslation(pivot.x(), pivot.y(), 0);

	// Setup the matrices
	Matrix4 pivotToOrigin = Matrix4::getTranslation(-pivotTranslation);
	Matrix4 originToPivot = Matrix4::getTranslation(pivotTranslation);
	Matrix4 scale = Matrix4::getScale(Vector3(newDist.x()/dist.x(), newDist.y()/dist.y(), 0));

	// Apply the matrices to the current texture transform, pre-multiplied in the correct order
	Matrix4 texTransform = _sourceFace.getTexdef().m_projection.getTransform();

	matrix4_premultiply_by_matrix4(texTransform, pivotToOrigin);
	matrix4_premultiply_by_matrix4(texTransform, scale);
	matrix4_premultiply_by_matrix4(texTransform, originToPivot);

	// Save it back to the face
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
