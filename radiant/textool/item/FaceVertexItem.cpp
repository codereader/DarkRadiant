#include "FaceVertexItem.h"

#include "registry/registry.h"
#include "../TexTool.h"

#include "FaceItem.h"

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
FaceVertexItem::FaceVertexItem(Face& sourceFace, WindingVertex& windingVertex, FaceItem& parent) :
	_sourceFace(sourceFace),
	_windingVertex(windingVertex),
	_parent(parent)
{}

void FaceVertexItem::beginTransformation()
{
	_sourceFace.undoSave();
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
	if (registry::getValue<bool>(ui::RKEY_FACE_VERTEX_SCALE_PIVOT_IS_CENTROID))
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

	texTransform.premultiplyBy(pivotToOrigin);
	texTransform.premultiplyBy(scale);
	texTransform.premultiplyBy(originToPivot);

	// Save it back to the face
	_sourceFace.getTexdef().m_projection.setTransform(1, 1, texTransform);

	_sourceFace.texdefChanged();
}

void FaceVertexItem::snapSelectedToGrid(float grid)
{
	if (_selected)
	{
		// Calculate how far we need to move our vertex
		Vector2 snapped = _windingVertex.texcoord;

		snapped[0] = float_snapped(snapped[0], grid);
		snapped[1] = float_snapped(snapped[1], grid);

		Vector2 translation = snapped - _windingVertex.texcoord;

		if (translation.getLength() == 0)
		{
			return; // nothing to do
		}

		Matrix4 matrix = Matrix4::getTranslation(Vector3(translation.x(), translation.y(), 0));

		// Do the transformation
		_parent.transform(matrix);
	}
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
