/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "brush_primit.h"

#include "debugging/debugging.h"

#include "brush/TexDef.h"
#include "itextures.h"

#include <algorithm>

#include "stringio.h"
#include "texturelib.h"
#include "math/matrix.h"
#include "math/Plane3.h"
#include "math/aabb.h"

#include "winding.h"
#include "preferences.h"


void ComputeAxisBase(const Vector3& normal, Vector3& texS, Vector3& texT);

inline void DebugAxisBase(const Vector3& normal)
{
  Vector3 x, y;
  ComputeAxisBase(normal, x, y);
  globalOutputStream() << "BP debug: " << x << y << normal << "\n";
}

void Texdef_EmitTextureCoordinates(const TextureProjection& projection, std::size_t width, std::size_t height, Winding& w, const Vector3& normal, const Matrix4& localToWorld)
{
  if(w.numpoints < 3)
  {
    return;
  }
  //globalOutputStream() << "normal: " << normal << "\n";

  Matrix4 local2tex = projection.getTransform((float)width, (float)height);
  //globalOutputStream() << "texdef: " << static_cast<const Vector3&>(local2tex.x()) << static_cast<const Vector3&>(local2tex.y()) << "\n";

#if 0
  {
    TextureProjection tmp;
    Texdef_fromTransform(tmp, (float)width, (float)height, local2tex);
    Matrix4 tmpTransform;
    TexDefoTransform(tmp, (float)width, (float)height, tmpTransform);
    ASSERT_MESSAGE(matrix4_equal_epsilon(local2tex, tmpTransform, 0.0001f), "bleh");
  }
#endif
  
  {
    Matrix4 xyz2st; 
    // we don't care if it's not normalised...
    xyz2st = projection.getBasisForNormal(matrix4_transformed_direction(localToWorld, normal));
    //globalOutputStream() << "basis: " << static_cast<const Vector3&>(xyz2st.x()) << static_cast<const Vector3&>(xyz2st.y()) << static_cast<const Vector3&>(xyz2st.z()) << "\n";
    matrix4_multiply_by_matrix4(local2tex, xyz2st);
  }

  Vector3 tangent(local2tex.getTransposed().x().getVector3().getNormalised());
  Vector3 bitangent(local2tex.getTransposed().y().getVector3().getNormalised());
  
  matrix4_multiply_by_matrix4(local2tex, localToWorld);

  for(Winding::iterator i = w.begin(); i != w.end(); ++i)
  {
    Vector3 texcoord = matrix4_transformed_point(local2tex, (*i).vertex);
    (*i).texcoord[0] = texcoord[0];
    (*i).texcoord[1] = texcoord[1];

    (*i).tangent = tangent;
    (*i).bitangent = bitangent;
  }
}

/*!
\brief Provides the axis-base of the texture ST space for this normal,
as they had been transformed to world XYZ space.
*/
void TextureAxisFromNormal(const Vector3& normal, Vector3& s, Vector3& t)
{
  switch (projectionaxis_for_normal(normal))
  {
  case eProjectionAxisZ:
    s[0]  =  1;
    s[1]  =  0;
    s[2]  =  0;
    
    t[0]  =  0;
    t[1]  = -1;
    t[2]  =  0;

    break;
  case eProjectionAxisY:
    s[0]  =  1;
    s[1]  =  0;
    s[2]  =  0;
    
    t[0]  =  0;
    t[1]  =  0;
    t[2]  = -1;

    break;
  case eProjectionAxisX:
    s[0]  =  0;
    s[1]  =  1;
    s[2]  =  0;
    
    t[0]  =  0;
    t[1]  =  0;
    t[2]  = -1;

    break;
  }
}

// NOTE: added these from Ritual's Q3Radiant
void ClearBounds(Vector3& mins, Vector3& maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds(const Vector3& v, Vector3& mins, Vector3& maxs)
{
	int		i;
	float	val;
	
	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

// low level functions .. put in mathlib?
#define BPMatCopy(a,b) {b[0][0] = a[0][0]; b[0][1] = a[0][1]; b[0][2] = a[0][2]; b[1][0] = a[1][0]; b[1][1] = a[1][1]; b[1][2] = a[1][2];}
// apply a scale transformation to the BP matrix
#define BPMatScale(m,sS,sT) {m[0][0]*=sS; m[1][0]*=sS; m[0][1]*=sT; m[1][1]*=sT;}
// apply a translation transformation to a BP matrix
#define BPMatTranslate(m,s,t) {m[0][2] += m[0][0]*s + m[0][1]*t; m[1][2] += m[1][0]*s+m[1][1]*t;}
// 2D homogeneous matrix product C = A*B
void BPMatMul(float A[2][3], float B[2][3], float C[2][3]);
// apply a rotation (degrees)
void BPMatRotate(float A[2][3], float theta);
#ifdef _DEBUG
void BPMatDump(float A[2][3]);
#endif

#ifdef _DEBUG
//#define DBG_BP
#endif

// compute a determinant using Sarrus rule
//++timo "inline" this with a macro
// NOTE : the three vectors are understood as columns of the matrix
inline float SarrusDet(const Vector3& a, const Vector3& b, const Vector3& c)
{
	return a[0]*b[1]*c[2]+b[0]*c[1]*a[2]+c[0]*a[1]*b[2]
		-c[0]*b[1]*a[2]-a[1]*b[0]*c[2]-a[0]*b[2]*c[1];
}

// in many case we know three points A,B,C in two axis base B1 and B2
// and we want the matrix M so that A(B1) = T * A(B2)
// NOTE: 2D homogeneous space stuff
// NOTE: we don't do any check to see if there's a solution or we have a particular case .. need to make sure before calling
// NOTE: the third coord of the A,B,C point is ignored
// NOTE: see the commented out section to fill M and D
//++timo TODO: update the other members to use this when possible
void MatrixForPoints( Vector3 M[3], Vector3 D[2], BrushPrimitTexDef *T )
{
//	Vector3 M[3]; // columns of the matrix .. easier that way (the indexing is not standard! it's column-line .. later computations are easier that way)
	float det;
//	Vector3 D[2];
	M[2][0]=1.0f; M[2][1]=1.0f; M[2][2]=1.0f;
#if 0
	// fill the data vectors
	M[0][0]=A2[0]; M[0][1]=B2[0]; M[0][2]=C2[0];
	M[1][0]=A2[1]; M[1][1]=B2[1]; M[1][2]=C2[1];
	M[2][0]=1.0f; M[2][1]=1.0f; M[2][2]=1.0f;
	D[0][0]=A1[0];
	D[0][1]=B1[0];
	D[0][2]=C1[0];
	D[1][0]=A1[1];
	D[1][1]=B1[1];
	D[1][2]=C1[1];
#endif
	// solve
	det = SarrusDet( M[0], M[1], M[2] );
	T->coords[0][0] = SarrusDet( D[0], M[1], M[2] ) / det;
	T->coords[0][1] = SarrusDet( M[0], D[0], M[2] ) / det;
	T->coords[0][2] = SarrusDet( M[0], M[1], D[0] ) / det;
	T->coords[1][0] = SarrusDet( D[1], M[1], M[2] ) / det;
	T->coords[1][1] = SarrusDet( M[0], D[1], M[2] ) / det;
	T->coords[1][2] = SarrusDet( M[0], M[1], D[1] ) / det;
}

typedef float texmat_t[2][3];

void TexMat_Scale(texmat_t texmat, float s, float t)
{
	texmat[0][0] *= s;
	texmat[0][1] *= s;
	texmat[0][2] *= s;
	texmat[1][0] *= t;
	texmat[1][1] *= t;
	texmat[1][2] *= t;
}

void TexMat_Assign(texmat_t texmat, const texmat_t other)
{
	texmat[0][0] = other[0][0];
	texmat[0][1] = other[0][1];
	texmat[0][2] = other[0][2];
	texmat[1][0] = other[1][0];
	texmat[1][1] = other[1][1];
	texmat[1][2] = other[1][2];
}

void ConvertTexMatWithDimensions(const texmat_t texmat1, std::size_t w1, std::size_t h1,
                                 texmat_t texmat2, std::size_t w2, std::size_t h2)
{
  TexMat_Assign(texmat2, texmat1);
  TexMat_Scale(texmat2, static_cast<float>(w1) / static_cast<float>(w2), static_cast<float>(h1) / static_cast<float>(h2));
}

// TTimo: FIXME: I don't like that, it feels broken
//   (and it's likely that it's not used anymore)
// best fitted 2D vector is x.X+y.Y
void ComputeBest2DVector( Vector3& v, Vector3& X, Vector3& Y, int &x, int &y )
{
	double sx,sy;
	sx = v.dot( X );
	sy = v.dot( Y );
	if ( fabs(sy) > fabs(sx) )
  {
		x = 0;
		if ( sy > 0.0 )
			y =  1;
		else
			y = -1;
	}
	else
	{
		y = 0;
		if ( sx > 0.0 )
			x =  1;
		else
			x = -1;
	}
}


// don't do C==A!
void BPMatMul(float A[2][3], float B[2][3], float C[2][3])
{
  C[0][0] = A[0][0]*B[0][0]+A[0][1]*B[1][0];
  C[1][0] = A[1][0]*B[0][0]+A[1][1]*B[1][0];
  C[0][1] = A[0][0]*B[0][1]+A[0][1]*B[1][1];
  C[1][1] = A[1][0]*B[0][1]+A[1][1]*B[1][1];
  C[0][2] = A[0][0]*B[0][2]+A[0][1]*B[1][2]+A[0][2];
  C[1][2] = A[1][0]*B[0][2]+A[1][1]*B[1][2]+A[1][2];
}

void BPMatDump(float A[2][3])
{
  globalOutputStream() << "" << A[0][0]
    << " " << A[0][1]
    << " " << A[0][2]
    << "\n" << A[1][0]
    << " " << A[1][2]
    << " " << A[1][2]
    << "\n0 0 1\n";
}

void BPMatRotate(float A[2][3], float theta)
{
  float m[2][3];
  float aux[2][3];
  memset(&m, 0, sizeof(float)*6);
  m[0][0] = static_cast<float>(cos(degrees_to_radians(theta)));
  m[0][1] = static_cast<float>(-sin(degrees_to_radians(theta)));
  m[1][0] = -m[0][1];
  m[1][1] = m[0][0];
  BPMatMul(A, m, aux);
  BPMatCopy(aux,A);
}

#if 0 // camera-relative texture shift
// get the relative axes of the current texturing
void BrushPrimit_GetRelativeAxes(face_t *f, Vector3& vecS, Vector3& vecT)
{
  float vS[2],vT[2];
  // first we compute them as expressed in plane axis base
  // BP matrix has coordinates of plane axis base expressed in geometric axis base
  // so we use the line vectors
  vS[0] = f->brushprimit_texdef.coords[0][0];
  vS[1] = f->brushprimit_texdef.coords[0][1];
  vT[0] = f->brushprimit_texdef.coords[1][0];
  vT[1] = f->brushprimit_texdef.coords[1][1];
  // now compute those vectors in geometric space
  Vector3 texS, texT; // axis base of the plane (geometric)
  ComputeAxisBase(f->plane.normal, texS, texT);
  // vecS[] = vS[0].texS[] + vS[1].texT[]
  // vecT[] = vT[0].texS[] + vT[1].texT[]
  vecS[0] = vS[0]*texS[0] + vS[1]*texT[0];
  vecS[1] = vS[0]*texS[1] + vS[1]*texT[1];
  vecS[2] = vS[0]*texS[2] + vS[1]*texT[2];
  vecT[0] = vT[0]*texS[0] + vT[1]*texT[0];
  vecT[1] = vT[0]*texS[1] + vT[1]*texT[1];
  vecT[2] = vT[0]*texS[2] + vT[1]*texT[2];
}

// brush primitive texture adjustments, use the camera view to map adjustments
// ShiftTextureRelative_BrushPrimit ( s , t ) will shift relative to the texture
void ShiftTextureRelative_Camera(face_t *f, int x, int y)
{
  Vector3 vecS, vecT;
  float XY[2]; // the values we are going to send for translation
  float sgn[2]; // +1 or -1
  int axis[2];
  CamWnd* pCam;

  // get the two relative texture axes for the current texturing
  BrushPrimit_GetRelativeAxes(f, vecS, vecT);

  // center point of the face, project it on the camera space
  Vector3 C;
  VectorClear(C);
  int i;
  for (i=0; i<f->face_winding->numpoints; i++)
  {
    VectorAdd(C,f->face_winding->point_at(i),C);
  }
  VectorScale(C,1.0/f->face_winding->numpoints,C);

  pCam = g_pParentWnd->GetCamWnd();
  pCam->MatchViewAxes(C, vecS, axis[0], sgn[0]);
  pCam->MatchViewAxes(C, vecT, axis[1], sgn[1]);
  
  // this happens when the two directions can't be mapped on two different directions on the screen
  // then the move will occur against a single axis
  // (i.e. the user is not positioned well enough to send understandable shift commands)
  // NOTE: in most cases this warning is not very relevant because the user would use one of the two axes
  // for which the solution is easy (the other one being unknown)
  // so this warning could be removed
  if (axis[0] == axis[1])
    globalOutputStream() << "Warning: degenerate in ShiftTextureRelative_Camera\n";

  // compute the X Y geometric increments
  // those geometric increments will be applied along the texture axes (the ones we computed above)
  XY[0] = 0;
  XY[1] = 0;
  if (x!=0)
  {
    // moving right/left
    XY[axis[0]] += sgn[0]*x;
  }
  if (y!=0)
  {
    XY[axis[1]] += sgn[1]*y;
  }
  // we worked out a move along vecS vecT, and we now it's geometric amplitude
  // apply it
  ShiftTextureRelative_BrushPrimit(f, XY[0], XY[1]);
}
#endif

void ShiftScaleRotate_fromFace(TexDef& shiftScaleRotate, const TextureProjection& projection) {
	shiftScaleRotate = projection.m_brushprimit_texdef.getFakeTexCoords();
}

void ShiftScaleRotate_toFace(const TexDef& shiftScaleRotate, TextureProjection& projection) {
	// compute texture matrix
	// the matrix returned must be understood as a qtexture_t with width=2 height=2
	projection.m_brushprimit_texdef = BrushPrimitTexDef(shiftScaleRotate);
}

#if 1
void Q3_to_matrix(const TexDef& texdef, float width, float height, const Vector3& normal, Matrix4& matrix)
{
  Normal_GetTransform(normal, matrix);

  Matrix4 transform = texdef.getTransform(width, height);

  matrix4_multiply_by_matrix4(matrix, transform);
}

void BP_from_matrix(BrushPrimitTexDef& bp_texdef, const Vector3& normal, const Matrix4& transform)
{
  Matrix4 basis;
  basis = g_matrix4_identity;
  ComputeAxisBase(normal, basis.x().getVector3(), basis.y().getVector3());
  basis.z().getVector3() = normal;
  basis.transpose();
  matrix4_affine_invert(basis);

  Matrix4 basis2texture = matrix4_multiplied_by_matrix4(basis, transform);

  bp_texdef = BrushPrimitTexDef(basis2texture);
}

void Q3_to_BP(const TexDef& texdef, float width, float height, const Vector3& normal, BrushPrimitTexDef& bp_texdef)
{
  Matrix4 matrix;
  Q3_to_matrix(texdef, width, height, normal, matrix);
  BP_from_matrix(bp_texdef, normal, matrix);
}
#endif
