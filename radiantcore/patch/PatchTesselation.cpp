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

	extent[0] = vertices[width - 1].vertex - vertices[0].vertex;
	extent[1] = vertices[(height - 1) * width + width - 1].vertex - vertices[0].vertex;
	extent[2] = vertices[(height - 1) * width].vertex - vertices[0].vertex;

	Vector3 norm = extent[0].cross(extent[1]);

	if (norm.getLengthSquared() == 0.0f)
	{
		norm = extent[0].cross(extent[2]);

		if (norm.getLengthSquared() == 0.0f)
		{
			norm = extent[1].cross(extent[2]);
		}
	}

	// wrapped patched may not get a valid normal here
	if (norm.normalise() != 0.0f)
	{
		auto offset = vertices[0].vertex.dot(norm);

		std::size_t i = 0;

		for (i = 1; i < width * height; i++)
		{
			auto d = vertices[i].vertex.dot(norm);

			if (fabs(d - offset) > COPLANAR_EPSILON)
			{
				break;
			}
		}

		if (i == width * height)
		{
			// all are coplanar
			for (i = 0; i < width * height; i++)
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

		for (i = 0; i < height; i++)
		{
			Vector3 delta = vertices[i * width].vertex - vertices[i * width + width - 1].vertex;

			if (delta.getLengthSquared() > 1.0f)
			{
				break;
			}
		}

		if (i == height)
		{
			wrapWidth = true;
		}
	}

	bool wrapHeight = false;

	{
		std::size_t i = 0;

		for (i = 0; i < width; i++)
		{
			Vector3 delta = vertices[i].vertex - vertices[(height - 1) * width + i].vertex;

			if (delta.getLengthSquared() > 1.0f)
			{
				break;
			}
		}

		if (i == width)
		{
			wrapHeight = true;
		}
	}

	Vector3 around[8];
	bool good[8];
	static int neighbors[8][2] = { { 0,1 },{ 1,1 },{ 1,0 },{ 1,-1 },{ 0,-1 },{ -1,-1 },{ -1,0 },{ -1,1 } };

	for (std::size_t i = 0; i < width; i++)
	{
		for (std::size_t j = 0; j < height; j++)
		{
			Vector3 base = vertices[j * width + i].vertex;

			for (std::size_t k = 0; k < 8; k++)
			{
				around[k] = Vector3(0, 0, 0);
				good[k] = false;

				for (int dist = 1; dist <= 3; dist++)
				{
					int x = static_cast<int>(i) + neighbors[k][0] * dist;
					int y = static_cast<int>(j) + neighbors[k][1] * dist;

					if (wrapWidth)
					{
						if (x < 0)
						{
							x = static_cast<int>(width) - 1 + x;
						}
						else if (x >= static_cast<int>(width))
						{
							x = 1 + x - static_cast<int>(width);
						}
					}

					if (wrapHeight)
					{
						if (y < 0)
						{
							y = static_cast<int>(height) - 1 + y;
						}
						else if (y >= static_cast<int>(height))
						{
							y = 1 + y - static_cast<int>(height);
						}
					}

					if (x < 0 || x >= static_cast<int>(width) || y < 0 || y >= static_cast<int>(height))
					{
						break;					// edge of patch
					}

					Vector3 temp = vertices[y * width + x].vertex - base;

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

				Vector3 tempNormal = around[(k + 1) & 7].cross(around[k]);
				if (tempNormal.normalise() == 0.0f)
				{
					continue;
				}

				sum += tempNormal;
			}

			vertices[j * width + i].normal = sum;

			// Catch cases where normal turns out as (0,0,0)
			if (sum.getLengthSquared() > 0)
			{
				vertices[j * width + i].normal.normalise();
			}
		}
	}
}

void PatchTesselation::sampleSinglePatchPoint(const MeshVertex ctrl[3][3], float u, float v, MeshVertex& out) const
{
	double vCtrl[3][8];

	// find the control points for the v coordinate
	for (std::size_t vPoint = 0; vPoint < 3; vPoint++)
	{
		for (std::size_t axis = 0; axis < 8; axis++)
		{
			double a, b, c;

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

			double qA = a - 2.0 * b + c;
			double qB = 2.0 * b - 2.0 * a;
			double qC = a;

			vCtrl[vPoint][axis] = qA * u * u + qB * u + qC;
		}
	}

	// interpolate the v value
	for (std::size_t axis = 0; axis < 8; axis++)
	{
		double a = vCtrl[0][axis];
		double b = vCtrl[1][axis];
		double c = vCtrl[2][axis];
		double qA = a - 2.0 * b + c;
		double qB = 2.0 * b - 2.0 * a;
		double qC = a;

		if (axis < 3)
		{
			out.vertex[axis] = qA * v * v + qB * v + qC;
		}
		else if (axis < 6)
		{
			out.normal[axis - 3] = qA * v * v + qB * v + qC;
		}
		else
		{
			out.texcoord[axis - 6] = qA * v * v + qB * v + qC;
		}
	}
}

void PatchTesselation::sampleSinglePatch(const MeshVertex ctrl[3][3],
	std::size_t baseCol, std::size_t baseRow,
	std::size_t w, std::size_t horzSub, std::size_t vertSub,
	std::vector<MeshVertex>& outVerts) const
{
	horzSub++;
	vertSub++;

	for (std::size_t i = 0; i < horzSub; i++)
	{
		for (std::size_t j = 0; j < vertSub; j++)
		{
			float u = static_cast<float>(i) / (horzSub - 1);
			float v = static_cast<float>(j) / (vertSub - 1);

			sampleSinglePatchPoint(ctrl, u, v, outVerts[((baseRow + j) * w) + i + baseCol]);
		}
	}
}

void PatchTesselation::subdivideMeshFixed(std::size_t subdivX, std::size_t subdivY)
{
	std::size_t outWidth = ((width - 1) / 2 * subdivX) + 1;
	std::size_t outHeight = ((height - 1) / 2 * subdivY) + 1;

	std::vector<MeshVertex> dv(outWidth * outHeight);

	std::size_t baseCol = 0;
	MeshVertex sample[3][3];

	for (std::size_t i = 0; i + 2 < width; i += 2)
	{
		std::size_t baseRow = 0;

		for (std::size_t j = 0; j + 2 < height; j += 2)
		{
			for (std::size_t k = 0; k < 3; k++)
			{
				for (std::size_t l = 0; l < 3; l++)
				{
					sample[k][l] = vertices[((j + l) * width) + i + k];
				}
			}

			sampleSinglePatch(sample, baseCol, baseRow, outWidth, subdivX, subdivY, dv);

			baseRow += subdivY;
		}

		baseCol += subdivX;
	}

	vertices.swap(dv);

	width = _maxWidth = outWidth;
	height = _maxHeight = outHeight;
}

void PatchTesselation::collapseMesh()
{
	if (width != _maxWidth)
	{
		for (std::size_t j = 0; j < height; j++)
		{
			for (std::size_t i = 0; i < width; i++)
			{
				vertices[j*width + i] = vertices[j*_maxWidth + i];
			}
		}
	}

	vertices.resize(width * height);
}

void PatchTesselation::expandMesh()
{
	vertices.resize(_maxWidth * _maxHeight);

	if (width != _maxWidth)
	{
		for (int j = static_cast<int>(height) - 1; j >= 0; j--)
		{
			for (int i = static_cast<int>(width) - 1; i >= 0; i--)
			{
				vertices[j*_maxWidth + i] = vertices[j*width + i];
			}
		}
	}
}

void PatchTesselation::resizeExpandedMesh(std::size_t newHeight, std::size_t newWidth)
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
	for (int j = static_cast<int>(_maxHeight) - 1; j >= 0; j--)
	{
		for (int i = static_cast<int>(_maxWidth) - 1; i >= 0; i--)
		{
			vertices[j*newWidth + i] = vertices[j*_maxWidth + i];
		}
	}

	_maxHeight = newHeight;
	_maxWidth = newWidth;
}

void PatchTesselation::lerpVert(const MeshVertex& a, const MeshVertex& b, MeshVertex&out)
{
	out.vertex = math::midPoint(a.vertex, b.vertex);
	out.normal = math::midPoint(a.normal, b.normal);
	out.texcoord = a.texcoord.mid(b.texcoord);
}

void PatchTesselation::putOnCurve()
{
	MeshVertex prev, next;

	// put all the approximating points on the curve
	for (std::size_t i = 0; i < width; i++)
	{
		for (std::size_t j = 1; j < height; j += 2)
		{
			lerpVert(vertices[j*_maxWidth + i], vertices[(j + 1)*_maxWidth + i], prev);
			lerpVert(vertices[j*_maxWidth + i], vertices[(j - 1)*_maxWidth + i], next);
			lerpVert(prev, next, vertices[j*_maxWidth + i]);
		}
	}

	for (std::size_t j = 0; j < height; j++)
	{
		for (std::size_t i = 1; i < width; i += 2)
		{
			lerpVert(vertices[j*_maxWidth + i], vertices[j*_maxWidth + i + 1], prev);
			lerpVert(vertices[j*_maxWidth + i], vertices[j*_maxWidth + i - 1], next);
			lerpVert(prev, next, vertices[j*_maxWidth + i]);
		}
	}
}

Vector3 PatchTesselation::projectPointOntoVector(const Vector3& point, const Vector3& vStart, const Vector3& vEnd)
{
	Vector3 pVec = point - vStart;
	Vector3 vec = vEnd - vStart;

	vec.normalise();

	// project onto the directional vector for this segment
	return vStart + vec * pVec.dot(vec);
}

void PatchTesselation::removeLinearColumnsRows()
{
	for (std::size_t j = 1; j < width - 1; j++)
	{
		double maxLength = 0;

		for (std::size_t i = 0; i < height; i++)
		{
			Vector3 proj = projectPointOntoVector(vertices[i*_maxWidth + j].vertex,
				vertices[i*_maxWidth + j - 1].vertex,
				vertices[i*_maxWidth + j + 1].vertex);

			Vector3 dir = vertices[i*_maxWidth + j].vertex - proj;

			auto len = dir.getLengthSquared();

			if (len > maxLength)
			{
				maxLength = len;
			}
		}

		if (maxLength < 0.2*0.2)
		{
			width--;

			for (std::size_t i = 0; i < height; i++)
			{
				for (std::size_t k = j; k < width; k++)
				{
					vertices[i*_maxWidth + k] = vertices[i*_maxWidth + k + 1];
				}
			}

			j--;
		}
	}

	for (std::size_t j = 1; j < height - 1; j++)
	{
		double maxLength = 0;

		for (std::size_t i = 0; i < width; i++)
		{
			Vector3 proj = projectPointOntoVector(vertices[j*_maxWidth + i].vertex,
				vertices[(j - 1)*_maxWidth + i].vertex,
				vertices[(j + 1)*_maxWidth + i].vertex);

			Vector3 dir = vertices[j*_maxWidth + i].vertex - proj;

			auto len = dir.getLengthSquared();

			if (len > maxLength)
			{
				maxLength = len;
			}
		}

		if (maxLength < 0.2*0.2)
		{
			height--;

			for (std::size_t i = 0; i < width; i++)
			{
				for (std::size_t k = j; k < height; k++)
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
	MeshVertex prev, next, mid;

	static float maxHorizontalErrorSqr = DEFAULT_CURVE_MAX_ERROR * DEFAULT_CURVE_MAX_ERROR;
	static float maxVerticalErrorSqr = DEFAULT_CURVE_MAX_ERROR * DEFAULT_CURVE_MAX_ERROR;
	static float maxLengthSqr = DEFAULT_CURVE_MAX_LENGTH * DEFAULT_CURVE_MAX_LENGTH;

	expandMesh();

	// horizontal subdivisions
	for (std::size_t j = 0; j + 2 < width; j += 2)
	{
		std::size_t i;

		// check subdivided midpoints against control points
		for (i = 0; i < height; i++)
		{
			for (std::size_t l = 0; l < 3; l++)
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

		if (i == height)
		{
			continue;	// didn't need subdivision
		}

		if (width + 2 >= _maxWidth)
		{
			resizeExpandedMesh(_maxHeight, _maxWidth + 4);
		}

		// insert two columns and replace the peak
		width += 2;

		for (i = 0; i < height; i++)
		{
			lerpVert(vertices[i*_maxWidth + j], vertices[i*_maxWidth + j + 1], prev);
			lerpVert(vertices[i*_maxWidth + j + 1], vertices[i*_maxWidth + j + 2], next);
			lerpVert(prev, next, mid);

			for (int k = static_cast<int>(width) - 1; k > static_cast<int>(j) + 3; k--)
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
	for (std::size_t j = 0; j + 2 < height; j += 2)
	{
		std::size_t i;

		// check subdivided midpoints against control points
		for (i = 0; i < width; i++)
		{
			for (std::size_t l = 0; l < 3; l++)
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

		if (i == width)
		{
			continue;	// didn't need subdivision
		}

		if (height + 2 >= _maxHeight)
		{
			resizeExpandedMesh(_maxHeight + 4, _maxWidth);
		}

		// insert two columns and replace the peak
		height += 2;

		for (i = 0; i < width; i++)
		{
			lerpVert(vertices[j*_maxWidth + i], vertices[(j + 1)*_maxWidth + i], prev);
			lerpVert(vertices[(j + 1)*_maxWidth + i], vertices[(j + 2)*_maxWidth + i], next);
			lerpVert(prev, next, mid);

			for (int k = static_cast<int>(height) - 1; k > static_cast<int>(j) + 3; k--)
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

namespace
{

void calculateFaceTangent(FaceTangents& ft, const MeshVertex& a, const MeshVertex& b, const MeshVertex& c)
{
	double d0[5], d1[5];

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

	double area = d0[3] * d1[4] - d0[4] * d1[3];

	if (fabs(area) < 1e-20)
	{
		ft.tangents[0].set(0, 0, 0);
		ft.tangents[1].set(0, 0, 0);
		return;
	}

	double inva = area < 0.0 ? -1 : 1;		// was = 1.0f / area;

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

} // namespace

void PatchTesselation::deriveFaceTangents(std::vector<FaceTangents>& faceTangents)
{
	assert(lenStrips >= 3);

	// calculate tangent vectors for each face in isolation

	// DR is using indices that are sent to openGL as GL_QUAD_STRIPs
	// It takes N+2 indices to describe N triangles when using QUAD_STRIPs
	std::size_t numFacesPerStrip = lenStrips - 2;
	std::size_t numFaces = numFacesPerStrip * numStrips;

	faceTangents.resize(numFaces); // one tangent per face

	// Go through each strip and derive tangents for each triangle like idTech4 does
	const RenderIndex* strip_indices = &indices.front();

	for (std::size_t strip = 0; strip < numStrips; strip++, strip_indices += lenStrips)
	{
		for (std::size_t i = 0; i < lenStrips - 2; i += 2)
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
	if (lenStrips < 2) return;

	std::vector<FaceTangents> faceTangents;
	deriveFaceTangents(faceTangents);

	// Note: we don't clear the tangent vectors here since the calling code
	// just allocated the mesh which initialises all vectors to 0,0,0

	std::size_t numFacesPerStrip = lenStrips - 2;

	// The sum of all tangent vectors is assigned to each vertex of every face
	// Since vertices can be shared across triangles this might very well add
	// tangents of neighbouring triangles too
	const RenderIndex* strip_indices = &indices.front();

	for (std::size_t strip = 0; strip < numStrips; strip++, strip_indices += lenStrips)
	{
		for (std::size_t i = 0; i < lenStrips - 2; i += 2)
		{
			// First tri of the quad
			const FaceTangents& ft1 = faceTangents[strip*numFacesPerStrip + i];

			for (std::size_t j = 0; j < 3; j++)
			{
				MeshVertex& vert = vertices[strip_indices[i + j]];

				vert.tangent += ft1.tangents[0];
				vert.bitangent += ft1.tangents[1];
			}

			// Second tri of the quad
			const FaceTangents& ft2 = faceTangents[strip*numFacesPerStrip + i + 1];

			for (std::size_t j = 0; j < 3; j++)
			{
				MeshVertex& vert = vertices[strip_indices[i + j + 1]];

				vert.tangent += ft2.tangents[0];
				vert.bitangent += ft2.tangents[1];
			}
		}
	}

	// project the summed vectors onto the normal plane
	// and normalize.  The tangent vectors will not necessarily
	// be orthogonal to each other, but they will be orthogonal
	// to the surface normal.
	for (MeshVertex& vert : vertices)
	{
		auto d = vert.tangent.dot(vert.normal);
		vert.tangent = vert.tangent - vert.normal * d;
		vert.tangent.normalise();

		d = vert.bitangent.dot(vert.normal);
		vert.bitangent = vert.bitangent - vert.normal * d;
		vert.bitangent.normalise();
	}
}

void PatchTesselation::generateIndices()
{
	const std::size_t numElems = width*height; // total number of elements in vertex array

	const bool bWidthStrips = (width >= height); // decide if horizontal strips are longer than vertical

	// allocate vertex, normal, texcoord and primitive-index arrays
	vertices.resize(numElems);
	indices.resize(width * 2 * (height - 1));

	// set up strip indices
	if (bWidthStrips)
	{
		numStrips = height - 1;
		lenStrips = width * 2;

		for (std::size_t i = 0; i<width; i++)
		{
			for (std::size_t j = 0; j<numStrips; j++)
			{
				indices[(j*lenStrips) + i * 2] = RenderIndex(j*width + i);
				indices[(j*lenStrips) + i * 2 + 1] = RenderIndex((j + 1)*width + i);
			}
		}
	}
	else
	{
		numStrips = width - 1;
		lenStrips = height * 2;

		for (std::size_t i = 0; i<height; i++)
		{
			for (std::size_t j = 0; j<numStrips; j++)
			{
				indices[(j*lenStrips) + i * 2] = RenderIndex(((height - 1) - i)*width + j);
				indices[(j*lenStrips) + i * 2 + 1] = RenderIndex(((height - 1) - i)*width + j + 1);
			}
		}
	}
}

void PatchTesselation::generate(std::size_t patchWidth, std::size_t patchHeight,
	const PatchControlArray& controlPoints, bool subdivionsFixed, const Subdivisions& subdivs,
    IRenderEntity* renderEntity)
{
	width = patchWidth;
	height = patchHeight;

	_maxWidth = width;
	_maxHeight = height;

	// We start off with the control vertex grid, copy it into our tesselation structure
	vertices.resize(controlPoints.size());

	for (std::size_t w = 0; w < width; w++)
	{
		for (std::size_t h = 0; h < height; h++)
		{
			vertices[h*width + w].vertex = controlPoints[h*width + w].vertex;
			vertices[h*width + w].texcoord = controlPoints[h*width + w].texcoord;
		}
	}

	// generate normals for the control mesh
	generateNormals();

	if (subdivionsFixed)
	{
		subdivideMeshFixed(subdivs.x(), subdivs.y());
	}
	else
	{
		subdivideMesh();
	}

    // Final update: assign colours and normalise normals
    auto colour = renderEntity ? renderEntity->getEntityColour() : Vector4(1, 1, 1, 1);

	for (MeshVertex& vertex : vertices)
	{
	    // normalize all the lerped normals
		if (vertex.normal.getLengthSquared() > 0)
		{
			vertex.normal.normalise();
		}

        // Assign vertex colours using the colour of the entity
        vertex.colour = colour;
	}

	// Build the strip indices for rendering the quads
	generateIndices();

	// With indices in place we can derive the tangent/bitangent vectors
	deriveTangents();
}
