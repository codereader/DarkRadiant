#include "PatchTesselation.h"

#include "Patch.h"

void PatchTesselation::clear()
{
    *this = PatchTesselation();
}

#define	COPLANAR_EPSILON	0.1f

void PatchTesselation::generateNormals()
{
	//
	// if all points are coplanar, set all normals to that plane
	//
	Vector3	extent[3];

	extent[0] = vertices[m_nArrayWidth - 1].vertex - vertices[0].vertex;
	extent[1] = vertices[(m_nArrayHeight - 1) * m_nArrayWidth + m_nArrayWidth - 1].vertex - vertices[0].vertex;
	extent[2] = vertices[(m_nArrayHeight - 1) * m_nArrayWidth].vertex - vertices[0].vertex;

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
		float offset = vertices[0].vertex.dot(norm);

		std::size_t i = 0;

		for (i = 1; i < m_nArrayWidth * m_nArrayHeight; i++)
		{
			float d = vertices[i].vertex.dot(norm);

			if (fabs(d - offset) > COPLANAR_EPSILON)
			{
				break;
			}
		}

		if (i == m_nArrayWidth * m_nArrayHeight)
		{
			// all are coplanar
			for (i = 0; i < m_nArrayWidth * m_nArrayHeight; i++)
			{
				vertices[i].normal = norm;
			}

			return;
		}
	}

	// check for wrapped edge cases, which should smooth across themselves
	bool wrapWidth = false;

	{
		std::size_t i = 0;

		for (i = 0; i < m_nArrayHeight; i++)
		{
			Vector3 delta = vertices[i * m_nArrayWidth].vertex - vertices[i * m_nArrayWidth + m_nArrayWidth - 1].vertex;

			if (delta.getLengthSquared() > 1.0f)
			{
				break;
			}
		}

		if (i == m_nArrayHeight)
		{
			wrapWidth = true;
		}
	}

	bool wrapHeight = false;

	{
		std::size_t i = 0;

		for (i = 0; i < m_nArrayWidth; i++)
		{
			Vector3 delta = vertices[i].vertex - vertices[(m_nArrayHeight - 1) * m_nArrayWidth + i].vertex;

			if (delta.getLengthSquared() > 1.0f)
			{
				break;
			}
		}

		if (i == m_nArrayWidth)
		{
			wrapHeight = true;
		}
	}

	Vector3 around[8];
	bool good[8];
	static int neighbors[8][2] = { { 0,1 },{ 1,1 },{ 1,0 },{ 1,-1 },{ 0,-1 },{ -1,-1 },{ -1,0 },{ -1,1 } };

	for (std::size_t i = 0; i < m_nArrayWidth; i++)
	{
		for (std::size_t j = 0; j < m_nArrayHeight; j++)
		{
			int count = 0;
			Vector3 base = vertices[j * m_nArrayWidth + i].vertex;

			for (std::size_t k = 0; k < 8; k++)
			{
				around[k] = Vector3(0, 0, 0);
				good[k] = false;

				for (std::size_t dist = 1; dist <= 3; dist++)
				{
					int x = i + neighbors[k][0] * dist;
					int y = j + neighbors[k][1] * dist;

					if (wrapWidth)
					{
						if (x < 0)
						{
							x = m_nArrayWidth - 1 + x;
						}
						else if (x >= m_nArrayWidth)
						{
							x = 1 + x - m_nArrayWidth;
						}
					}

					if (wrapHeight)
					{
						if (y < 0)
						{
							y = m_nArrayHeight - 1 + y;
						}
						else if (y >= m_nArrayHeight)
						{
							y = 1 + y - m_nArrayHeight;
						}
					}

					if (x < 0 || x >= m_nArrayWidth || y < 0 || y >= m_nArrayHeight)
					{
						break;					// edge of patch
					}

					Vector3 temp = vertices[y * m_nArrayWidth + x].vertex - base;

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

			Vector3 sum(0, 0, 0);

			for (std::size_t k = 0; k < 8; k++)
			{
				if (!good[k] || !good[(k + 1) & 7])
				{
					continue;	// didn't get two points
				}

				Vector3 tempNormal = around[(k + 1) & 7].crossProduct(around[k]);
				if (tempNormal.normalise() == 0.0f)
				{
					continue;
				}

				sum += tempNormal;
				count++;
			}

			if (count == 0)
			{
				count = 1;
			}

			vertices[j * m_nArrayWidth + i].normal = sum;
			vertices[j * m_nArrayWidth + i].normal.normalise();
		}
	}
}

void PatchTesselation::sampleSinglePatchPoint(const ArbitraryMeshVertex ctrl[3][3], float u, float v, ArbitraryMeshVertex* out) const
{
	float vCtrl[3][8];

	// find the control points for the v coordinate
	for (int vPoint = 0; vPoint < 3; vPoint++)
	{
		for (int axis = 0; axis < 8; axis++)
		{
			float a, b, c;

			if (axis < 3)
			{
				a = ctrl[0][vPoint].vertex[axis];
				b = ctrl[1][vPoint].vertex[axis];
				c = ctrl[2][vPoint].vertex[axis];
			}
			else if (axis < 6)
			{
				a = ctrl[0][vPoint].normal[axis - 3];
				b = ctrl[1][vPoint].normal[axis - 3];
				c = ctrl[2][vPoint].normal[axis - 3];
			}
			else
			{
				a = ctrl[0][vPoint].texcoord[axis - 6];
				b = ctrl[1][vPoint].texcoord[axis - 6];
				c = ctrl[2][vPoint].texcoord[axis - 6];
			}

			float qA = a - 2.0f * b + c;
			float qB = 2.0f * b - 2.0f * a;
			float qC = a;

			vCtrl[vPoint][axis] = qA * u * u + qB * u + qC;
		}
	}

	// interpolate the v value
	for (int axis = 0; axis < 8; axis++)
	{
		float a = vCtrl[0][axis];
		float b = vCtrl[1][axis];
		float c = vCtrl[2][axis];
		float qA = a - 2.0f * b + c;
		float qB = 2.0f * b - 2.0f * a;
		float qC = a;

		if (axis < 3)
		{
			out->vertex[axis] = qA * v * v + qB * v + qC;
		}
		else if (axis < 6)
		{
			out->normal[axis - 3] = qA * v * v + qB * v + qC;
		}
		else
		{
			out->texcoord[axis - 6] = qA * v * v + qB * v + qC;
		}
	}
}

void PatchTesselation::sampleSinglePatch(const ArbitraryMeshVertex ctrl[3][3], int baseCol, int baseRow, int width, int horzSub, int vertSub, ArbitraryMeshVertex* outVerts) const
{
	horzSub++;
	vertSub++;

	for (int i = 0; i < horzSub; i++)
	{
		for (int j = 0; j < vertSub; j++)
		{
			float u = static_cast<float>(i) / (horzSub - 1);
			float v = static_cast<float>(j) / (vertSub - 1);

			sampleSinglePatchPoint(ctrl, u, v, &outVerts[((baseRow + j) * width) + i + baseCol]);
		}
	}
}

void PatchTesselation::subdivideMeshFixed(unsigned int subdivX, unsigned int subdivY)
{
	int outWidth = ((m_nArrayWidth - 1) / 2 * subdivX) + 1;
	int outHeight = ((m_nArrayHeight - 1) / 2 * subdivY) + 1;

	ArbitraryMeshVertex* dv = new ArbitraryMeshVertex[outWidth * outHeight];

	int baseCol = 0;
	ArbitraryMeshVertex sample[3][3];

	for (int i = 0; i + 2 < m_nArrayWidth; i += 2)
	{
		int baseRow = 0;

		for (int j = 0; j + 2 < m_nArrayHeight; j += 2)
		{
			for (int k = 0; k < 3; k++)
			{
				for (int l = 0; l < 3; l++)
				{
					sample[k][l] = vertices[((j + l) * m_nArrayWidth) + i + k];
				}
			}

			sampleSinglePatch(sample, baseCol, baseRow, outWidth, subdivX, subdivY, dv);
			baseRow += subdivY;
		}

		baseCol += subdivX;
	}

	vertices.resize(outWidth * outHeight);

	for (int i = 0; i < outWidth * outHeight; i++)
	{
		vertices[i] = dv[i];
	}

	delete[] dv;

	m_nArrayWidth = _maxWidth = outWidth;
	m_nArrayHeight = _maxHeight = outHeight;
}

void PatchTesselation::collapseMesh()
{
	if (m_nArrayWidth != _maxWidth)
	{
		for (int j = 0; j < m_nArrayHeight; j++)
		{
			for (int i = 0; i < m_nArrayWidth; i++)
			{
				vertices[j*m_nArrayWidth + i] = vertices[j*_maxWidth + i];
			}
		}
	}

	vertices.resize(m_nArrayWidth * m_nArrayHeight);
}

void PatchTesselation::expandMesh()
{
	vertices.resize(_maxWidth * _maxHeight);

	if (m_nArrayWidth != _maxWidth)
	{
		for (int j = m_nArrayHeight - 1; j >= 0; j--)
		{
			for (int i = m_nArrayWidth - 1; i >= 0; i--)
			{
				vertices[j*_maxWidth + i] = vertices[j*m_nArrayWidth + i];
			}
		}
	}
}

void PatchTesselation::resizeExpandedMesh(int newHeight, int newWidth)
{
	if (newHeight <= _maxHeight && newWidth <= _maxWidth)
	{
		return;
	}

	if (newHeight * newWidth > _maxHeight * _maxWidth)
	{
		vertices.resize(newHeight * newWidth);
	}

	// space out verts for new height and width
	for (int j = _maxHeight - 1; j >= 0; j--)
	{
		for (int i = _maxWidth - 1; i >= 0; i--)
		{
			vertices[j*newWidth + i] = vertices[j*_maxWidth + i];
		}
	}

	_maxHeight = newHeight;
	_maxWidth = newWidth;
}

void PatchTesselation::lerpVert(const ArbitraryMeshVertex& a, const ArbitraryMeshVertex& b, ArbitraryMeshVertex&out) const
{
	out.vertex[0] = 0.5f * (a.vertex[0] + b.vertex[0]);
	out.vertex[1] = 0.5f * (a.vertex[1] + b.vertex[1]);
	out.vertex[2] = 0.5f * (a.vertex[2] + b.vertex[2]);
	out.normal[0] = 0.5f * (a.normal[0] + b.normal[0]);
	out.normal[1] = 0.5f * (a.normal[1] + b.normal[1]);
	out.normal[2] = 0.5f * (a.normal[2] + b.normal[2]);
	out.texcoord[0] = 0.5f * (a.texcoord[0] + b.texcoord[0]);
	out.texcoord[1] = 0.5f * (a.texcoord[1] + b.texcoord[1]);
}

void PatchTesselation::putOnCurve()
{
	ArbitraryMeshVertex prev, next;

	// put all the approximating points on the curve
	for (int i = 0; i < m_nArrayWidth; i++)
	{
		for (int j = 1; j < m_nArrayHeight; j += 2)
		{
			lerpVert(vertices[j*_maxWidth + i], vertices[(j + 1)*_maxWidth + i], prev);
			lerpVert(vertices[j*_maxWidth + i], vertices[(j - 1)*_maxWidth + i], next);
			lerpVert(prev, next, vertices[j*_maxWidth + i]);
		}
	}

	for (int j = 0; j < m_nArrayHeight; j++)
	{
		for (int i = 1; i < m_nArrayWidth; i += 2)
		{
			lerpVert(vertices[j*_maxWidth + i], vertices[j*_maxWidth + i + 1], prev);
			lerpVert(vertices[j*_maxWidth + i], vertices[j*_maxWidth + i - 1], next);
			lerpVert(prev, next, vertices[j*_maxWidth + i]);
		}
	}
}

void PatchTesselation::projectPointOntoVector(const Vector3& point, const Vector3& vStart, const Vector3& vEnd, Vector3& vProj)
{
	Vector3 pVec = point - vStart;
	Vector3 vec = vEnd - vStart;

	vec.normalise();

	// project onto the directional vector for this segment
	vProj = vStart + vec * pVec.dot(vec);
}

void PatchTesselation::removeLinearColumnsRows()
{
	for (int j = 1; j < m_nArrayWidth - 1; j++)
	{
		float maxLength = 0;

		for (int i = 0; i < m_nArrayHeight; i++)
		{
			Vector3 proj;
			projectPointOntoVector(vertices[i*_maxWidth + j].vertex,
				vertices[i*_maxWidth + j - 1].vertex,
				vertices[i*_maxWidth + j + 1].vertex, proj);

			Vector3 dir = vertices[i*_maxWidth + j].vertex - proj;

			float len = dir.getLengthSquared();

			if (len > maxLength)
			{
				maxLength = len;
			}
		}

		if (maxLength < 0.2f*0.2f)
		{
			m_nArrayWidth--;

			for (int i = 0; i < m_nArrayHeight; i++)
			{
				for (int k = j; k < m_nArrayWidth; k++)
				{
					vertices[i*_maxWidth + k] = vertices[i*_maxWidth + k + 1];
				}
			}

			j--;
		}
	}

	for (int j = 1; j < m_nArrayHeight - 1; j++)
	{
		float maxLength = 0;

		for (int i = 0; i < m_nArrayWidth; i++)
		{
			Vector3 proj;
			projectPointOntoVector(vertices[j*_maxWidth + i].vertex,
				vertices[(j - 1)*_maxWidth + i].vertex,
				vertices[(j + 1)*_maxWidth + i].vertex, proj);

			Vector3 dir = vertices[j*_maxWidth + i].vertex - proj;

			float len = dir.getLengthSquared();

			if (len > maxLength)
			{
				maxLength = len;
			}
		}

		if (maxLength < 0.2f*0.2f)
		{
			m_nArrayHeight--;

			for (int i = 0; i < m_nArrayWidth; i++)
			{
				for (int k = j; k < m_nArrayHeight; k++)
				{
					vertices[k*_maxWidth + i] = vertices[(k + 1)*_maxWidth + i];
				}
			}

			j--;
		}
	}
}

void PatchTesselation::subdivideMesh()
{
	static const float DEFAULT_CURVE_MAX_ERROR = 4.0f;
	static const float DEFAULT_CURVE_MAX_LENGTH = -1.0f;

	Vector3 prevxyz, nextxyz, midxyz;
	ArbitraryMeshVertex prev, next, mid;

	float maxHorizontalErrorSqr = DEFAULT_CURVE_MAX_ERROR * DEFAULT_CURVE_MAX_ERROR;
	float maxVerticalErrorSqr = DEFAULT_CURVE_MAX_ERROR * DEFAULT_CURVE_MAX_ERROR;
	float maxLengthSqr = DEFAULT_CURVE_MAX_LENGTH * DEFAULT_CURVE_MAX_LENGTH;

	expandMesh();

	// horizontal subdivisions
	for (int j = 0; j + 2 < m_nArrayWidth; j += 2)
	{
		int i;

		// check subdivided midpoints against control points
		for (i = 0; i < m_nArrayHeight; i++)
		{
			for (int l = 0; l < 3; l++)
			{
				prevxyz[l] = vertices[i*_maxWidth + j + 1].vertex[l] - vertices[i*_maxWidth + j].vertex[l];
				nextxyz[l] = vertices[i*_maxWidth + j + 2].vertex[l] - vertices[i*_maxWidth + j + 1].vertex[l];
				midxyz[l] = (vertices[i*_maxWidth + j].vertex[l] + vertices[i*_maxWidth + j + 1].vertex[l] * 2.0f + vertices[i*_maxWidth + j + 2].vertex[l]) * 0.25f;
			}

			if (DEFAULT_CURVE_MAX_LENGTH > 0.0f)
			{
				// if the span length is too long, force a subdivision
				if (prevxyz.getLengthSquared() > maxLengthSqr || nextxyz.getLengthSquared() > maxLengthSqr)
				{
					break;
				}
			}

			// see if this midpoint is off far enough to subdivide
			Vector3 delta = vertices[i*_maxWidth + j + 1].vertex - midxyz;

			if (delta.getLengthSquared() > maxHorizontalErrorSqr)
			{
				break;
			}
		}

		if (i == m_nArrayHeight)
		{
			continue;	// didn't need subdivision
		}

		if (m_nArrayWidth + 2 >= _maxWidth)
		{
			resizeExpandedMesh(_maxHeight, _maxWidth + 4);
		}

		// insert two columns and replace the peak
		m_nArrayWidth += 2;

		for (i = 0; i < m_nArrayHeight; i++)
		{
			lerpVert(vertices[i*_maxWidth + j], vertices[i*_maxWidth + j + 1], prev);
			lerpVert(vertices[i*_maxWidth + j + 1], vertices[i*_maxWidth + j + 2], next);
			lerpVert(prev, next, mid);

			for (int k = m_nArrayWidth - 1; k > j + 3; k--)
			{
				vertices[i*_maxWidth + k] = vertices[i*_maxWidth + k - 2];
			}
			vertices[i*_maxWidth + j + 1] = prev;
			vertices[i*_maxWidth + j + 2] = mid;
			vertices[i*_maxWidth + j + 3] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;
	}

	// vertical subdivisions
	for (int j = 0; j + 2 < m_nArrayHeight; j += 2)
	{
		int i;

		// check subdivided midpoints against control points
		for (i = 0; i < m_nArrayWidth; i++)
		{
			for (int l = 0; l < 3; l++)
			{
				prevxyz[l] = vertices[(j + 1)*_maxWidth + i].vertex[l] - vertices[j*_maxWidth + i].vertex[l];
				nextxyz[l] = vertices[(j + 2)*_maxWidth + i].vertex[l] - vertices[(j + 1)*_maxWidth + i].vertex[l];
				midxyz[l] = (vertices[j*_maxWidth + i].vertex[l] + vertices[(j + 1)*_maxWidth + i].vertex[l] * 2.0f + vertices[(j + 2)*_maxWidth + i].vertex[l]) * 0.25f;
			}

			if (DEFAULT_CURVE_MAX_LENGTH > 0.0f)
			{
				// if the span length is too long, force a subdivision
				if (prevxyz.getLengthSquared() > maxLengthSqr || nextxyz.getLengthSquared() > maxLengthSqr)
				{
					break;
				}
			}

			// see if this midpoint is off far enough to subdivide
			Vector3 delta = vertices[(j + 1)*_maxWidth + i].vertex - midxyz;

			if (delta.getLengthSquared() > maxVerticalErrorSqr)
			{
				break;
			}
		}

		if (i == m_nArrayWidth)
		{
			continue;	// didn't need subdivision
		}

		if (m_nArrayHeight + 2 >= _maxHeight)
		{
			resizeExpandedMesh(_maxHeight + 4, _maxWidth);
		}

		// insert two columns and replace the peak
		m_nArrayHeight += 2;

		for (i = 0; i < m_nArrayWidth; i++)
		{
			lerpVert(vertices[j*_maxWidth + i], vertices[(j + 1)*_maxWidth + i], prev);
			lerpVert(vertices[(j + 1)*_maxWidth + i], vertices[(j + 2)*_maxWidth + i], next);
			lerpVert(prev, next, mid);

			for (int k = m_nArrayHeight - 1; k > j + 3; k--)
			{
				vertices[k*_maxWidth + i] = vertices[(k - 2)*_maxWidth + i];
			}

			vertices[(j + 1)*_maxWidth + i] = prev;
			vertices[(j + 2)*_maxWidth + i] = mid;
			vertices[(j + 3)*_maxWidth + i] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;
	}

	putOnCurve();

	removeLinearColumnsRows();

	collapseMesh();
}

struct FaceTangents
{
	Vector3 tangents[2];
};

void calculateFaceTangent(FaceTangents& ft, const ArbitraryMeshVertex& a, const ArbitraryMeshVertex& b, const ArbitraryMeshVertex& c)
{
	float		d0[5], d1[5];

	d0[0] = b.vertex[0] - a.vertex[0];
	d0[1] = b.vertex[1] - a.vertex[1];
	d0[2] = b.vertex[2] - a.vertex[2];
	d0[3] = b.texcoord[0] - a.texcoord[0];
	d0[4] = b.texcoord[1] - a.texcoord[1];

	d1[0] = c.vertex[0] - a.vertex[0];
	d1[1] = c.vertex[1] - a.vertex[1];
	d1[2] = c.vertex[2] - a.vertex[2];
	d1[3] = c.texcoord[0] - a.texcoord[0];
	d1[4] = c.texcoord[1] - a.texcoord[1];

	float area = d0[3] * d1[4] - d0[4] * d1[3];

	if (fabs(area) < 1e-20f)
	{
		ft.tangents[0].set(0, 0, 0);
		ft.tangents[1].set(0, 0, 0);
		return;
	}

	float inva = area < 0.0f ? -1 : 1;		// was = 1.0f / area;

	Vector3 temp;

	temp[0] = (d0[0] * d1[4] - d0[4] * d1[0]) * inva;
	temp[1] = (d0[1] * d1[4] - d0[4] * d1[1]) * inva;
	temp[2] = (d0[2] * d1[4] - d0[4] * d1[2]) * inva;
	temp.normalise();
	ft.tangents[0] = temp.getNormalised();

	temp[0] = (d0[3] * d1[0] - d0[0] * d1[3]) * inva;
	temp[1] = (d0[3] * d1[1] - d0[1] * d1[3]) * inva;
	temp[2] = (d0[3] * d1[2] - d0[2] * d1[3]) * inva;
	temp.normalise();
	ft.tangents[1] = temp;
}

void PatchTesselation::deriveFaceTangents(std::vector<FaceTangents>& faceTangents)
{
	assert(m_lenStrips >= 3);

	// calculate tangent vectors for each face in isolation

	// DR is using indices that are sent to openGL as GL_QUAD_STRIPs 
	// It takes N+2 indices to describe N triangles when using QUAD_STRIPs
	std::size_t numFacesPerStrip = m_lenStrips - 2;
	std::size_t numFaces = numFacesPerStrip * m_numStrips;

	faceTangents.resize(numFaces); // one tangent per face

								   // Go through each strip and derive tangents for each triangle like idTech4 does
	const RenderIndex* strip_indices = &indices.front();

	for (std::size_t strip = 0; strip < m_numStrips; strip++, strip_indices += m_lenStrips)
	{
		for (std::size_t i = 0; i < m_lenStrips - 2; i += 2)
		{
			// First tri of the quad (indices 0,1,2)
			calculateFaceTangent(faceTangents[strip*numFacesPerStrip + i],
				vertices[strip_indices[i + 0]],
				vertices[strip_indices[i + 1]],
				vertices[strip_indices[i + 2]]);

			// Second tri of the quad (indices 1,2,3)
			calculateFaceTangent(faceTangents[strip*numFacesPerStrip + i + 1],
				vertices[strip_indices[i + 1]],
				vertices[strip_indices[i + 2]],
				vertices[strip_indices[i + 3]]);
		}
	}
}

void PatchTesselation::deriveTangents()
{
	if (m_lenStrips < 2) return;

	std::vector<FaceTangents> faceTangents;
	deriveFaceTangents(faceTangents);

	// Note: we don't clear the tangent vectors here since the calling code
	// just allocated the mesh which initialises all vectors to 0,0,0

	std::size_t numFacesPerStrip = m_lenStrips - 2;

	// The sum of all tangent vectors is assigned to each vertex of every face
	// Since vertices can be shared across triangles this might very well add
	// tangents of neighbouring triangles too
	const RenderIndex* strip_indices = &indices.front();

	for (std::size_t strip = 0; strip < m_numStrips; strip++, strip_indices += m_lenStrips)
	{
		for (std::size_t i = 0; i < m_lenStrips - 2; i += 2)
		{
			// First tri of the quad
			const FaceTangents& ft1 = faceTangents[strip*numFacesPerStrip + i];

			for (std::size_t j = 0; j < 3; j++)
			{
				ArbitraryMeshVertex& vert = vertices[strip_indices[i + j]];

				vert.tangent += ft1.tangents[0];
				vert.bitangent += ft1.tangents[1];
			}

			// Second tri of the quad
			const FaceTangents& ft2 = faceTangents[strip*numFacesPerStrip + i + 1];

			for (std::size_t j = 0; j < 3; j++)
			{
				ArbitraryMeshVertex& vert = vertices[strip_indices[i + j + 1]];

				vert.tangent += ft2.tangents[0];
				vert.bitangent += ft2.tangents[1];
			}
		}
	}

	// project the summed vectors onto the normal plane
	// and normalize.  The tangent vectors will not necessarily
	// be orthogonal to each other, but they will be orthogonal
	// to the surface normal.
	for (ArbitraryMeshVertex& vert : vertices)
	{
		float d = vert.tangent.dot(vert.normal);
		vert.tangent = vert.tangent - vert.normal * d;
		vert.tangent.normalise();

		d = vert.bitangent.dot(vert.normal);
		vert.bitangent = vert.bitangent - vert.normal * d;
		vert.bitangent.normalise();
	}
}

void PatchTesselation::generateIndices()
{
	const std::size_t numElems = m_nArrayWidth*m_nArrayHeight; // total number of elements in vertex array

	const bool bWidthStrips = (m_nArrayWidth >= m_nArrayHeight); // decide if horizontal strips are longer than vertical

	// allocate vertex, normal, texcoord and primitive-index arrays
	vertices.resize(numElems);
	indices.resize(m_nArrayWidth * 2 * (m_nArrayHeight - 1));

	// set up strip indices
	if (bWidthStrips)
	{
		m_numStrips = m_nArrayHeight - 1;
		m_lenStrips = m_nArrayWidth * 2;

		for (std::size_t i = 0; i<m_nArrayWidth; i++)
		{
			for (std::size_t j = 0; j<m_numStrips; j++)
			{
				indices[(j*m_lenStrips) + i * 2] = RenderIndex(j*m_nArrayWidth + i);
				indices[(j*m_lenStrips) + i * 2 + 1] = RenderIndex((j + 1)*m_nArrayWidth + i);
			}
		}
	}
	else
	{
		m_numStrips = m_nArrayWidth - 1;
		m_lenStrips = m_nArrayHeight * 2;

		for (std::size_t i = 0; i<m_nArrayHeight; i++)
		{
			for (std::size_t j = 0; j<m_numStrips; j++)
			{
				indices[(j*m_lenStrips) + i * 2] = RenderIndex(((m_nArrayHeight - 1) - i)*m_nArrayWidth + j);
				indices[(j*m_lenStrips) + i * 2 + 1] = RenderIndex(((m_nArrayHeight - 1) - i)*m_nArrayWidth + j + 1);
			}
		}
	}
}

void PatchTesselation::generate(const Patch& patch)
{
	m_nArrayWidth = patch.getWidth();
	m_nArrayHeight = patch.getHeight();

	_maxWidth = m_nArrayWidth;
	_maxHeight = m_nArrayHeight;

	// We start off with the control vertex grid, copy it into our tesselation structure
	const PatchControlArray& ctrlTransformed = patch.getControlPointsTransformed();
	this->vertices.resize(ctrlTransformed.size());

	for (std::size_t w = 0; w < m_nArrayWidth; w++)
	{
		for (std::size_t h = 0; h < m_nArrayHeight; h++)
		{
			vertices[h*m_nArrayWidth + w].vertex = ctrlTransformed[h*m_nArrayWidth + w].vertex;
			vertices[h*m_nArrayWidth + w].texcoord = ctrlTransformed[h*m_nArrayWidth + w].texcoord;
		}
	}

	// idtech4 code begin

	// generate normals for the control mesh
	generateNormals();

	if (patch.subdivionsFixed())
	{
		Subdivisions subdivs = patch.getSubdivisions();
		subdivideMeshFixed(subdivs.x(), subdivs.y());
	}
	else
	{
		subdivideMesh();
	}

	// normalize all the lerped normals
	for (ArbitraryMeshVertex& vertex : vertices)
	{
		vertex.normal.normalise();
	}

	// idtech4 code end

	generateIndices();
	deriveTangents();
}
