#include "ProcPatch.h"

namespace map
{

namespace
{
	// DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH

const float COPLANAR_EPSILON = 0.1f;
const float DEFAULT_CURVE_MAX_ERROR	= 4.0f;
const float DEFAULT_CURVE_MAX_LENGTH = -1.0f;

ArbitraryMeshVertex lerpVertex(const ArbitraryMeshVertex& a, const ArbitraryMeshVertex& b)
{
	ArbitraryMeshVertex out;

	out.vertex[0] = 0.5f * (a.vertex[0] + b.vertex[0]);
	out.vertex[1] = 0.5f * (a.vertex[1] + b.vertex[1]);
	out.vertex[2] = 0.5f * (a.vertex[2] + b.vertex[2]);

	out.normal[0] = 0.5f * (a.normal[0] + b.normal[0]);
	out.normal[1] = 0.5f * (a.normal[1] + b.normal[1]);
	out.normal[2] = 0.5f * (a.normal[2] + b.normal[2]);

	out.texcoord[0] = 0.5f * (a.texcoord[0] + b.texcoord[0]);
	out.texcoord[1] = 0.5f * (a.texcoord[1] + b.texcoord[1]);

	return out;
}

Vector3 projectPointOntoVector(const Vector3& point, const Vector3& vStart, const Vector3& vEnd)
{
	Vector3 pVec = point - vStart;
	Vector3 vec = vEnd - vStart;

	vec.normalise();

	// project onto the directional vector for this segment
	return (vStart + vec * (pVec.dot(vec)));
}

}

ProcPatch::ProcPatch(const IPatch& mapPatch) :
	_mapPatch(mapPatch),
	_width(0),
	_height(0),
	_expanded(false),
	_horzSubdivisions(0),
	_vertSubdivisions(0),
	_explicitSubdivisions(false),
	_maxWidth(0),
	_maxHeight(0)
{
	_width = _maxWidth = mapPatch.getWidth();
	_height = _maxHeight = mapPatch.getHeight();
	_vertices.resize(_width * _height);

	for (std::size_t w = 0; w < _width; ++w)
	{
		for (std::size_t h = 0; h < _height; ++h)
		{
			const PatchControl& ctrl = mapPatch.ctrlAt(h, w);

			_vertices[h*_width + w] = ArbitraryMeshVertex(ctrl.vertex, Vector3(0,0,0), ctrl.texcoord);
		}
	}
}

void ProcPatch::expand()
{
	_vertices.resize(_maxWidth * _maxHeight);

	if (_width != _maxWidth)
	{
		for (int j = static_cast<int>(_height) - 1; j >= 0; --j)
		{
			for (int i = static_cast<int>(_width) - 1; i >= 0; --i)
			{
				_vertices[j*_maxWidth + i] = _vertices[j*_width + i];
			}
		}
	}
}

void ProcPatch::resizeExpanded(std::size_t newHeight, std::size_t newWidth)
{
	if (newHeight <= _maxHeight && newWidth <= _maxWidth) 
	{
		return;
	}

	if (newHeight * newWidth > _maxHeight * _maxWidth)
	{
		_vertices.resize(newHeight * newWidth);
	}

	// space out verts for new height and width
	for (int j = static_cast<int>(_maxHeight) - 1; j >= 0; j--)
	{
		for (int i = static_cast<int>(_maxWidth) - 1; i >= 0; i--)
		{
			_vertices[j*newWidth + i] = _vertices[j*_maxWidth + i];
		}
	}

	_maxHeight = newHeight;
	_maxWidth = newWidth;
}

void ProcPatch::putOnCurve()
{
	// put all the approximating points on the curve
	for (std::size_t i = 0; i < _width; i++ )
	{
		for (std::size_t j = 1; j < _height; j += 2 )
		{
			ArbitraryMeshVertex prev = lerpVertex(_vertices[j*_maxWidth+i], _vertices[(j+1)*_maxWidth+i]);
			ArbitraryMeshVertex next = lerpVertex(_vertices[j*_maxWidth+i], _vertices[(j-1)*_maxWidth+i]);
			_vertices[j*_maxWidth+i] = lerpVertex(prev, next);
		}
	}

	for (std::size_t j = 0; j < _height; j++ )
	{
		for (std::size_t i = 1; i < _width; i += 2 )
		{
			ArbitraryMeshVertex prev = lerpVertex(_vertices[j*_maxWidth+i], _vertices[j*_maxWidth+i+1]);
			ArbitraryMeshVertex next = lerpVertex(_vertices[j*_maxWidth+i], _vertices[j*_maxWidth+i-1]);
			_vertices[j*_maxWidth+i] = lerpVertex(prev, next);
		}
	}
}

void ProcPatch::removeLinearColumnsRows()
{
	for (int j = 1; j < _width - 1; j++) 
	{
		float maxLength = 0;

		for (int i = 0; i < _height; i++)
		{
			Vector3 proj = projectPointOntoVector(_vertices[i*_maxWidth + j].vertex,
									_vertices[i*_maxWidth + j-1].vertex, _vertices[i*_maxWidth + j+1].vertex);

			Vector3 dir = _vertices[i*_maxWidth + j].vertex - proj;
			float len = dir.getLengthSquared();
			if ( len > maxLength ) {
				maxLength = len;
			}
		}

		if (maxLength < 0.2f*0.2f)
		{
			_width--;

			for (int i = 0; i < _height; i++ )
			{
				for (int k = j; k < _width; k++ )
				{
					_vertices[i*_maxWidth + k] = _vertices[i*_maxWidth + k+1];
				}
			}
			j--;
		}
	}

	for (int j = 1; j < _height - 1; j++ )
	{
		float maxLength = 0;

		for (int i = 0; i < _width; i++ )
		{
			Vector3 proj = projectPointOntoVector( _vertices[j*_maxWidth + i].vertex,
									_vertices[(j-1)*_maxWidth + i].vertex, _vertices[(j+1)*_maxWidth + i].vertex);

			Vector3 dir = _vertices[j*_maxWidth + i].vertex - proj;
			float len = dir.getLengthSquared();
			if ( len > maxLength ) {
				maxLength = len;
			}
		}
		if ( maxLength < 0.2f*0.2f) 
		{
			_height--;

			for (int i = 0; i < _width; i++ )
			{
				for (int k = j; k < _height; k++ )
				{
					_vertices[k*_maxWidth + i] = _vertices[(k+1)*_maxWidth + i];
				}
			}

			j--;
		}
	}
}

void ProcPatch::collapse() 
{
	if (_width != _maxWidth)
	{
		for (std::size_t j = 0; j < _height; j++ )
		{
			for (std::size_t i = 0; i < _width; i++)
			{
				_vertices[j*_width + i] = _vertices[j*_maxWidth + i];
			}
		}
	}

	_vertices.resize(_width * _height);
}

void ProcPatch::generateIndices()
{
	_indices.resize((_width-1) * (_height-1) * 2 * 3);

	std::size_t index = 0;

	for (int i = 0; i < _width - 1; i++)
	{
		for (int j = 0; j < _height - 1; j++)
		{
			int v1 = j * static_cast<int>(_width) + i;
			int v2 = v1 + 1;
			int v3 = v1 + static_cast<int>(_width) + 1;
			int v4 = v1 + static_cast<int>(_width);

			_indices[index++] = v1;
			_indices[index++] = v3;
			_indices[index++] = v2;
			_indices[index++] = v1;
			_indices[index++] = v4;
			_indices[index++] = v3;
		}
	}

	//FIXME? GenerateEdgeIndexes();
}

void ProcPatch::subdivide(bool genNormals)
{
	// generate normals for the control mesh
	if (genNormals)
	{
		generateNormals();
	}

	float maxHorizontalErrorSqr = DEFAULT_CURVE_MAX_ERROR * DEFAULT_CURVE_MAX_ERROR;
	float maxVerticalErrorSqr = DEFAULT_CURVE_MAX_ERROR * DEFAULT_CURVE_MAX_ERROR;
	float maxgetLengthSquared = DEFAULT_CURVE_MAX_LENGTH * DEFAULT_CURVE_MAX_LENGTH;

	expand();

	// horizontal subdivisions
	for (std::size_t j = 0; j + 2 < _width; j += 2)
	{
		// check subdivided midpoints against control points
		std::size_t i = 0;

		for (i = 0; i < _height; ++i)
		{
			Vector3 prevVertex;
			Vector3 midVertex;
			Vector3 nextVertex;

			for (std::size_t l = 0; l < 3; ++l)
			{
				prevVertex[l] = _vertices[i*_maxWidth + j+1].vertex[l] - _vertices[i*_maxWidth + j  ].vertex[l];
				nextVertex[l] = _vertices[i*_maxWidth + j+2].vertex[l] - _vertices[i*_maxWidth + j+1].vertex[l];
				midVertex[l] = (_vertices[i*_maxWidth + j  ].vertex[l] + _vertices[i*_maxWidth + j+1].vertex[l] * 2.0f +
														   _vertices[i*_maxWidth + j+2].vertex[l] ) * 0.25f;
			}

			if (DEFAULT_CURVE_MAX_LENGTH > 0.0f)
			{
				// if the span length is too long, force a subdivision
				if (prevVertex.getLengthSquared() > maxgetLengthSquared || nextVertex.getLengthSquared() > maxgetLengthSquared)
				{
					break;
				}
			}

			// see if this midpoint is off far enough to subdivide
			Vector3 delta = _vertices[i*_maxWidth + j+1].vertex - midVertex;

			if (delta.getLengthSquared() > maxHorizontalErrorSqr)
			{
				break;
			}
		}

		if (i == _height)
		{
			continue;	// didn't need subdivision
		}

		if (_width + 2 >= _maxWidth)
		{
			resizeExpanded(_maxHeight, _maxWidth + 4);
		}

		// insert two columns and replace the peak
		_width += 2;

		for (i = 0; i < _height; ++i)
		{
			ArbitraryMeshVertex prev = lerpVertex(_vertices[i*_maxWidth + j], _vertices[i*_maxWidth + j+1]);
			ArbitraryMeshVertex next = lerpVertex(_vertices[i*_maxWidth + j+1], _vertices[i*_maxWidth + j+2]);
			ArbitraryMeshVertex mid =  lerpVertex(prev, next);

			for (int k = static_cast<int>(_width) - 1; k > j + 3; k--)
			{
				_vertices[i*_maxWidth + k] = _vertices[i*_maxWidth + k-2];
			}

			_vertices[i*_maxWidth + j+1] = prev;
			_vertices[i*_maxWidth + j+2] = mid;
			_vertices[i*_maxWidth + j+3] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;
	}

	// vertical subdivisions
	for (std::size_t j = 0; j + 2 < _height; j += 2) 
	{
		// check subdivided midpoints against control points
		std::size_t i = 0;
		
		for (i = 0; i < _width; ++i)
		{
			Vector3 prevVertex;
			Vector3 midVertex;
			Vector3 nextVertex;

			for (std::size_t l = 0; l < 3; ++l)
			{
				prevVertex[l] = _vertices[(j+1)*_maxWidth + i].vertex[l] - _vertices[j*_maxWidth + i].vertex[l];
				nextVertex[l] = _vertices[(j+2)*_maxWidth + i].vertex[l] - _vertices[(j+1)*_maxWidth + i].vertex[l];
				midVertex[l] = (_vertices[j*_maxWidth + i].vertex[l] + _vertices[(j+1)*_maxWidth + i].vertex[l] * 2.0f +
														_vertices[(j+2)*_maxWidth + i].vertex[l] ) * 0.25f;
			}

			if (DEFAULT_CURVE_MAX_LENGTH > 0.0f)
			{
				// if the span length is too long, force a subdivision
				if (prevVertex.getLengthSquared() > maxgetLengthSquared || nextVertex.getLengthSquared() > maxgetLengthSquared)
				{
					break;
				}
			}

			// see if this midpoint is off far enough to subdivide
			Vector3 delta = _vertices[(j+1)*_maxWidth + i].vertex - midVertex;

			if (delta.getLengthSquared() > maxVerticalErrorSqr)
			{
				break;
			}
		}

		if (i == _width)
		{
			continue;	// didn't need subdivision
		}

		if (_height + 2 >= _maxHeight)
		{
			resizeExpanded(_maxHeight + 4, _maxWidth);
		}

		// insert two columns and replace the peak
		_height += 2;

		for (i = 0; i < _width; ++i)
		{
			ArbitraryMeshVertex prev = lerpVertex(_vertices[j*_maxWidth + i], _vertices[(j+1)*_maxWidth + i]);
			ArbitraryMeshVertex next = lerpVertex(_vertices[(j+1)*_maxWidth + i], _vertices[(j+2)*_maxWidth + i]);
			ArbitraryMeshVertex mid =  lerpVertex(prev, next);

			for (int k = static_cast<int>(_height) - 1; k > j + 3; k--)
			{
				_vertices[k*_maxWidth + i] = _vertices[(k-2)*_maxWidth + i];
			}

			_vertices[(j+1)*_maxWidth + i] = prev;
			_vertices[(j+2)*_maxWidth + i] = mid;
			_vertices[(j+3)*_maxWidth + i] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;
	}

	putOnCurve();

	removeLinearColumnsRows();

	collapse();

	// normalize all the lerped normals
	if (genNormals)
	{
		for (std::size_t i = 0; i < _width * _height; ++i)
		{
			_vertices[i].normal.normalise();
		}
	}

	generateIndices();
}

void ProcPatch::sampleSinglePatchPoint(const ArbitraryMeshVertex ctrl[3][3], 
	float u, float v, ArbitraryMeshVertex& out) const
{
	float	vCtrl[3][8];

	// find the control points for the v coordinate
	for (std::size_t vPoint = 0; vPoint < 3; ++vPoint)
	{
		for (std::size_t axis = 0; axis < 8; ++axis)
		{
			float a, b, c;

			if (axis < 3)
			{
				a = ctrl[0][vPoint].vertex[axis];
				b = ctrl[1][vPoint].vertex[axis];
				c = ctrl[2][vPoint].vertex[axis];
			} 
			else if ( axis < 6 )
			{
				a = ctrl[0][vPoint].normal[axis-3];
				b = ctrl[1][vPoint].normal[axis-3];
				c = ctrl[2][vPoint].normal[axis-3];
			} 
			else 
			{
				a = ctrl[0][vPoint].texcoord[axis-6];
				b = ctrl[1][vPoint].texcoord[axis-6];
				c = ctrl[2][vPoint].texcoord[axis-6];
			}

			float qA = a - 2.0f * b + c;
			float qB = 2.0f * b - 2.0f * a;
			float qC = a;

			vCtrl[vPoint][axis] = qA * u * u + qB * u + qC;
		}
	}

	// interpolate the v value
	for (std::size_t axis = 0; axis < 8; ++axis)
	{
		float a = vCtrl[0][axis];
		float b = vCtrl[1][axis];
		float c = vCtrl[2][axis];
		float qA = a - 2.0f * b + c;
		float qB = 2.0f * b - 2.0f * a;
		float qC = a;

		if (axis < 3)
		{
			out.vertex[axis] = qA * v * v + qB * v + qC;
		}
		else if (axis < 6)
		{
			out.normal[axis-3] = qA * v * v + qB * v + qC;
		} 
		else
		{
			out.texcoord[axis-6] = qA * v * v + qB * v + qC;
		}
	}
}

void ProcPatch::sampleSinglePatch(const ArbitraryMeshVertex ctrl[3][3], 
	std::size_t baseCol, std::size_t baseRow, std::size_t width, std::size_t horzSub, std::size_t vertSub, 
	std::vector<ArbitraryMeshVertex>& outVerts) const
{
	horzSub++;
	vertSub++;

	for (std::size_t i = 0; i < horzSub; ++i)
	{
		for (std::size_t j = 0; j < vertSub; ++j)
		{
			float u = static_cast<float>(i) / ( horzSub - 1 );
			float v = static_cast<float>(j) / ( vertSub - 1 );

			sampleSinglePatchPoint(ctrl, u, v, outVerts[((baseRow + j) * width) + i + baseCol]);
		}
	}
}

void ProcPatch::subdivideExplicit(const Subdivisions& subdivisions, bool genNormals)
{
	std::size_t outWidth = ((_width - 1) / 2 * subdivisions[0]) + 1;
	std::size_t outHeight = ((_height - 1) / 2 * subdivisions[1]) + 1;

	std::vector<ArbitraryMeshVertex> dv(outWidth * outHeight);

	// generate normals for the control mesh
	if (genNormals)
	{
		generateNormals();
	}

	int baseCol = 0;
	ArbitraryMeshVertex sample[3][3];

	for (std::size_t i = 0; i + 2 < _width; i += 2)
	{
		std::size_t baseRow = 0;

		for (std::size_t j = 0; j + 2 < _height; j += 2)
		{
			for (std::size_t k = 0; k < 3; ++k)
			{
				for (std::size_t l = 0; l < 3; ++l)
				{
					sample[k][l] = _vertices[((j + l) * _width) + i + k];
				}
			}

			sampleSinglePatch(sample, baseCol, baseRow, outWidth, subdivisions[0], subdivisions[1], dv);

			baseRow += subdivisions[1];
		}

		baseCol += subdivisions[0];
	}

	_vertices.swap(dv);

	_width = _maxWidth = outWidth;
	_height = _maxHeight = outHeight;

	if (false)
	{
		expand();
		removeLinearColumnsRows();
		collapse();
	}

	// normalize all the lerped normals
	if (genNormals)
	{
		for (std::size_t i = 0; i < _width * _height; ++i)
		{
			_vertices[i].normal.normalise();
		}
	}

	generateIndices();
}

void ProcPatch::generateNormals()
{
	// if all points are coplanar, set all normals to that plane
	Vector3	extent[3];
	
	extent[0] = _vertices[_width - 1].vertex - _vertices[0].vertex;
	extent[1] = _vertices[(_height-1) * _width + _width - 1].vertex - _vertices[0].vertex;
	extent[2] = _vertices[(_height-1) * _width].vertex - _vertices[0].vertex;

	Vector3 norm = extent[0].crossProduct(extent[1]);

	if (norm.getLengthSquared() == 0.0f)
	{
		norm = extent[0].crossProduct(extent[2]);

		if (norm.getLengthSquared() == 0.0f)
		{
			norm = extent[1].crossProduct(extent[2]);
		}
	}

	// wrapped patched may not get a valid normal here
	if (norm.normalise() != 0.0f)
	{
		float offset = _vertices[0].vertex.dot(norm);

		std::size_t i = 0;

		for (i = 1; i < _width * _height; ++i)
		{
			float d = _vertices[i].vertex.dot(norm);

			if (fabs(d - offset) > COPLANAR_EPSILON)
			{
				break;
			}
		}

		if (i == _width * _height)
		{
			// all are coplanar
			for ( i = 0; i < _width * _height; ++i)
			{
				_vertices[i].normal = norm;
			}

			return;
		}
	}

	// check for wrapped edge cases, which should smooth across themselves
	bool wrapWidth = false;

	int i = 0;

	for (i = 0; i < _height; ++i)
	{
		Vector3 delta = _vertices[i * _width].vertex - _vertices[i * _width + _width-1].vertex;

		if (delta.getLengthSquared() > 1.0f)
		{
			break;
		}
	}

	if (i == _height)
	{
		wrapWidth = true;
	}

	bool wrapHeight = false;

	for (i = 0; i < _width; ++i)
	{
		Vector3 delta = _vertices[i].vertex - _vertices[(_height-1) * _width + i].vertex;

		if (delta.getLengthSquared() > 1.0f)
		{
			break;
		}
	}

	if (i == _width)
	{
		wrapHeight = true;
	}

	Vector3 around[8];
	bool good[8];

	static int neighbors[8][2] = { {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}	};
	
	for (i = 0; i < _width; ++i)
	{
		for (int j = 0; j < _height; ++j)
		{
			std::size_t count = 0;
			const Vector3& base = _vertices[j * _width + i].vertex;

			for (std::size_t k = 0; k < 8; ++k)
			{
				around[k] = Vector3(0,0,0);
				good[k] = false;

				for (int dist = 1; dist <= 3; ++dist)
				{
					int x = i + neighbors[k][0] * dist;
					int y = j + neighbors[k][1] * dist;

					if (wrapWidth)
					{
						if (x < 0)
						{
							x = static_cast<int>(_width) - 1 + x;
						} 
						else if (x >= _width) 
						{
							x = 1 + x - static_cast<int>(_width);
						}
					}

					if (wrapHeight) 
					{
						if (y < 0)
						{
							y = static_cast<int>(_height) - 1 + y;
						} 
						else if (y >= _height)
						{
							y = 1 + y - static_cast<int>(_height);
						}
					}

					if (x < 0 || x >= _width || y < 0 || y >= _height)
					{
						break;					// edge of patch
					}

					Vector3 temp = _vertices[y * _width + x].vertex - base;

					if (temp.normalise() == 0.0f)
					{
						continue;				// degenerate edge, get more dist
					} 
					else 
					{
						good[k] = true;
						around[k] = temp;
						break;					// good edge
					}
				}
			}

			Vector3 sum(0,0,0);

			for (std::size_t k = 0; k < 8; k++ )
			{
				if (!good[k] || !good[(k+1) & 7])
				{
					continue;	// didn't get two points
				}

				norm = around[(k+1) & 7].crossProduct(around[k]);
				
				if (norm.normalise() == 0.0f)
				{
					continue;
				}

				sum += norm;
				count++;
			}

			if (count == 0)
			{
				//idLib::common->Printf("bad normal\n");
				count = 1;
			}

			_vertices[j * _width + i].normal = sum;
			_vertices[j * _width + i].normal.normalise();
		}
	}
}

} // namespace
