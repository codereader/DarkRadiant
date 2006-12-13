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


/*!
\brief Construct a transform from XYZ space to ST space (3d to 2d).
This will be one of three axis-aligned spaces, depending on the surface normal.
NOTE: could also be done by swapping values.
*/
void Normal_GetTransform(const Vector3& normal, Matrix4& transform)
{
  switch (projectionaxis_for_normal(normal))
  {
  case eProjectionAxisZ:
    transform[0]  =  1;
    transform[1]  =  0;
    transform[2]  =  0;
    
    transform[4]  =  0;
    transform[5]  =  1;
    transform[6]  =  0;
    
    transform[8]  =  0;
    transform[9]  =  0;
    transform[10] =  1;
    break;
  case eProjectionAxisY:
    transform[0]  =  1;
    transform[1]  =  0;
    transform[2]  =  0;
    
    transform[4]  =  0;
    transform[5]  =  0;
    transform[6]  = -1;
    
    transform[8]  =  0;
    transform[9]  =  1;
    transform[10] =  0;
    break;
  case eProjectionAxisX:
    transform[0]  =  0;
    transform[1]  =  0;
    transform[2]  =  1;
    
    transform[4]  =  1;
    transform[5]  =  0;
    transform[6]  =  0;
    
    transform[8]  =  0;
    transform[9]  =  1;
    transform[10] =  0;
    break;
  }
  transform[3] = transform[7] = transform[11] = transform[12] = transform[13] = transform[14] = 0;
  transform[15] = 1;
}

void ComputeAxisBase(const Vector3& normal, Vector3& texS, Vector3& texT);

inline void DebugAxisBase(const Vector3& normal)
{
  Vector3 x, y;
  ComputeAxisBase(normal, x, y);
  globalOutputStream() << "BP debug: " << x << y << normal << "\n";
}

void Texdef_basisForNormal(const TextureProjection& projection, const Vector3& normal, Matrix4& basis)
{
  if(g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_BRUSHPRIMITIVES)
  {
    basis = g_matrix4_identity;
    ComputeAxisBase(normal, basis.x().getVector3(), basis.y().getVector3());
    basis.z().getVector3() = normal;
    basis.transpose();
    //DebugAxisBase(normal);
  }
  else if(g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_HALFLIFE)
  {
    basis = g_matrix4_identity;
    basis.x().getVector3() = projection.m_basis_s;
    basis.y().getVector3() = -projection.m_basis_t;
    basis.z().getVector3() = basis.x().getVector3().crossProduct(basis.y().getVector3()).getNormalised();
    matrix4_multiply_by_matrix4(basis, matrix4_rotation_for_z_degrees(-projection.m_texdef._rotate));
    //globalOutputStream() << "debug: " << projection.m_basis_s << projection.m_basis_t << normal << "\n";
    basis.transpose();
  }
  else
  {
    Normal_GetTransform(normal, basis);
  }
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
    Texdef_basisForNormal(projection, matrix4_transformed_direction(localToWorld, normal), xyz2st);
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


bp_globals_t g_bp_globals;
float g_texdef_default_scale;

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

//++timo replace everywhere texX by texS etc. ( ----> and in q3map !) 
// NOTE : ComputeAxisBase here and in q3map code must always BE THE SAME !
// WARNING : special case behaviour of atan2(y,x) <-> atan(y/x) might not be the same everywhere when x == 0
// rotation by (0,RotY,RotZ) assigns X to normal
void ComputeAxisBase(const Vector3& normal, Vector3& texS, Vector3& texT)
{
#if 1
  const Vector3 up(0, 0, 1);
  const Vector3 down(0, 0, -1);

  if(vector3_equal_epsilon(normal, up, float(1e-6)))
  {
    texS = Vector3(0, 1, 0);
    texT = Vector3(1, 0, 0);
  }
  else if(vector3_equal_epsilon(normal, down, float(1e-6)))
  {
    texS = Vector3(0, 1, 0);
    texT = Vector3(-1, 0, 0);
  }
  else
  {
    texS = normal.crossProduct(up).getNormalised();
    texT = normal.crossProduct(texS).getNormalised();
    texS = -texS;
  }

#else
	float RotY,RotZ;
	// do some cleaning
  /*
	if (fabs(normal[0])<1e-6)
		normal[0]=0.0f;
	if (fabs(normal[1])<1e-6)
		normal[1]=0.0f;
	if (fabs(normal[2])<1e-6)
		normal[2]=0.0f;
    */
	RotY=-atan2(normal[2],sqrt(normal[1]*normal[1]+normal[0]*normal[0]));
	RotZ=atan2(normal[1],normal[0]);
	// rotate (0,1,0) and (0,0,1) to compute texS and texT
	texS[0]=-sin(RotZ);
	texS[1]=cos(RotZ);
	texS[2]=0;
	// the texT vector is along -Z ( T texture coorinates axis )
	texT[0]=-sin(RotY)*cos(RotZ);
	texT[1]=-sin(RotY)*sin(RotZ);
	texT[2]=-cos(RotY);
#endif
}

#if 0 // texdef conversion
void FaceToBrushPrimitFace(face_t *f)
{
	Vector3 texX,texY;
	Vector3 proj;
	// ST of (0,0) (1,0) (0,1)
	float ST[3][5]; // [ point index ] [ xyz ST ]
	//++timo not used as long as brushprimit_texdef and texdef are static
/*	f->brushprimit_texdef.contents=f->texdef.contents;
	f->brushprimit_texdef.flags=f->texdef.flags;
	f->brushprimit_texdef.value=f->texdef.value;
	strcpy(f->brushprimit_texdef.name,f->texdef.name); */
#ifdef DBG_BP
	if ( f->plane.normal[0]==0.0f && f->plane.normal[1]==0.0f && f->plane.normal[2]==0.0f )
	{
		globalOutputStream() << "Warning : f->plane.normal is (0,0,0) in FaceToBrushPrimitFace\n";
	}
	// check d_texture
	if (!f->d_texture)
	{
		globalOutputStream() << "Warning : f.d_texture is 0 in FaceToBrushPrimitFace\n";
		return;
	}
#endif
	// compute axis base
	ComputeAxisBase(f->plane.normal,texX,texY);
	// compute projection vector
	VectorCopy(f->plane.normal,proj);
	VectorScale(proj,f->plane.dist,proj);
	// (0,0) in plane axis base is (0,0,0) in world coordinates + projection on the affine plane
	// (1,0) in plane axis base is texX in world coordinates + projection on the affine plane
	// (0,1) in plane axis base is texY in world coordinates + projection on the affine plane
	// use old texture code to compute the ST coords of these points
	VectorCopy(proj,ST[0]);
	EmitTextureCoordinates(ST[0], f->pShader->getTexture(), f);
	VectorCopy(texX,ST[1]);
	VectorAdd(ST[1],proj,ST[1]);
	EmitTextureCoordinates(ST[1], f->pShader->getTexture(), f);
	VectorCopy(texY,ST[2]);
	VectorAdd(ST[2],proj,ST[2]);
	EmitTextureCoordinates(ST[2], f->pShader->getTexture(), f);
	// compute texture matrix
	f->brushprimit_texdef.coords[0][2]=ST[0][3];
	f->brushprimit_texdef.coords[1][2]=ST[0][4];
	f->brushprimit_texdef.coords[0][0]=ST[1][3]-f->brushprimit_texdef.coords[0][2];
	f->brushprimit_texdef.coords[1][0]=ST[1][4]-f->brushprimit_texdef.coords[1][2];
	f->brushprimit_texdef.coords[0][1]=ST[2][3]-f->brushprimit_texdef.coords[0][2];
	f->brushprimit_texdef.coords[1][1]=ST[2][4]-f->brushprimit_texdef.coords[1][2];
}

// compute texture coordinates for the winding points
void EmitBrushPrimitTextureCoordinates(face_t * f, Winding * w)
{
	Vector3 texX,texY;
	float x,y;
	// compute axis base
	ComputeAxisBase(f->plane.normal,texX,texY);
	// in case the texcoords matrix is empty, build a default one
	// same behaviour as if scale[0]==0 && scale[1]==0 in old code
	if (f->brushprimit_texdef.coords[0][0]==0 && f->brushprimit_texdef.coords[1][0]==0 && f->brushprimit_texdef.coords[0][1]==0 && f->brushprimit_texdef.coords[1][1]==0)
	{
		f->brushprimit_texdef.coords[0][0] = 1.0f;
		f->brushprimit_texdef.coords[1][1] = 1.0f;
		ConvertTexMatWithQTexture( &f->brushprimit_texdef, 0, &f->brushprimit_texdef, f->pShader->getTexture() );
	}
	int i;
    for (i=0 ; i<w.numpoints ; i++)
	{
		x=w.point_at(i).dot(texX);
		y=w.point_at(i).dot(texY);
#if 0
#ifdef DBG_BP
		if (g_bp_globals.bNeedConvert)
		{
			// check we compute the same ST as the traditional texture computation used before
			float S=f->brushprimit_texdef.coords[0][0]*x+f->brushprimit_texdef.coords[0][1]*y+f->brushprimit_texdef.coords[0][2];
			float T=f->brushprimit_texdef.coords[1][0]*x+f->brushprimit_texdef.coords[1][1]*y+f->brushprimit_texdef.coords[1][2];
			if ( fabs(S-w.point_at(i)[3])>1e-2 || fabs(T-w.point_at(i)[4])>1e-2 )
			{
				if ( fabs(S-w.point_at(i)[3])>1e-4 || fabs(T-w.point_at(i)[4])>1e-4 )
					globalOutputStream() << "Warning : precision loss in brush -> brush primitive texture computation\n";
				else
					globalOutputStream() << "Warning : brush -> brush primitive texture computation bug detected\n";
			}
		}
#endif
#endif
		w.point_at(i)[3]=f->brushprimit_texdef.coords[0][0]*x+f->brushprimit_texdef.coords[0][1]*y+f->brushprimit_texdef.coords[0][2];
		w.point_at(i)[4]=f->brushprimit_texdef.coords[1][0]*x+f->brushprimit_texdef.coords[1][1]*y+f->brushprimit_texdef.coords[1][2];
	}
}
#endif

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

void BPTexdef_Construct(BrushPrimitTexDef& bp_td, std::size_t width, std::size_t height)
{
	bp_td.coords[0][0] = 1.0f;
	bp_td.coords[1][1] = 1.0f;
	ConvertTexMatWithDimensions(bp_td.coords, 2, 2, bp_td.coords, width, height);
}

void Texdef_Assign(TextureProjection& projection, const TextureProjection& other)
{
  if (g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_BRUSHPRIMITIVES)
  {
    projection.m_brushprimit_texdef = other.m_brushprimit_texdef;
  }
  else
  {
    projection.m_texdef = other.m_texdef;
    if(g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_HALFLIFE)
    {
      projection.m_basis_s = other.m_basis_s;
      projection.m_basis_t = other.m_basis_t;
    }
  }
}

void Texdef_Shift(TextureProjection& projection, float s, float t)
{
  if (g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_BRUSHPRIMITIVES)
  {
    projection.m_brushprimit_texdef.shift(s, t);
  }
  else
  {
		projection.m_texdef.shift(s, t);
  }
}

void Texdef_Scale(TextureProjection& projection, float s, float t)
{
	if (g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_BRUSHPRIMITIVES)
	{
    projection.m_brushprimit_texdef.scale(s, t);
	}
	else
	{
		projection.m_texdef.scale(s, t);
	}
}

void Texdef_Rotate(TextureProjection& projection, float angle)
{
	if (g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_BRUSHPRIMITIVES)
	{
     projection.m_brushprimit_texdef.rotate(angle);
	}
	else
	{
		projection.m_texdef.rotate(angle);
	}
}

void Texdef_FitTexture(TextureProjection& projection, std::size_t width, std::size_t height, const Vector3& normal, const Winding& w, float s_repeat, float t_repeat)
{
  if(w.numpoints < 3)
  {
    return;
  }

  Matrix4 st2tex = projection.getTransform((float)width, (float)height);

  // the current texture transform
  Matrix4 local2tex = st2tex;
  {
    Matrix4 xyz2st; 
    Texdef_basisForNormal(projection, normal, xyz2st);
    matrix4_multiply_by_matrix4(local2tex, xyz2st);
  }

  // the bounds of the current texture transform
  AABB bounds;
  for(Winding::const_iterator i = w.begin(); i != w.end(); ++i)
  {
    Vector3 texcoord = matrix4_transformed_point(local2tex, (*i).vertex);
    bounds.includePoint(texcoord);
  }
  bounds.origin.z() = 0;
  bounds.extents.z() = 1;

  // the bounds of a perfectly fitted texture transform
  AABB perfect(Vector3(s_repeat * 0.5, t_repeat * 0.5, 0), Vector3(s_repeat * 0.5, t_repeat * 0.5, 1));

  // the difference between the current texture transform and the perfectly fitted transform
  Matrix4 matrix = Matrix4::getTranslation(bounds.origin - perfect.origin);
  matrix4_pivoted_scale_by_vec3(matrix, bounds.extents / perfect.extents, perfect.origin);
  matrix4_affine_invert(matrix);

  // apply the difference to the current texture transform
  matrix4_premultiply_by_matrix4(st2tex, matrix);

	projection.setTransform((float)width, (float)height, st2tex);
	projection.normalise((float)width, (float)height);
}

float Texdef_getDefaultTextureScale()
{
  return g_texdef_default_scale;
}

void TexDef_Construct_Default(TextureProjection& projection)
{
  projection.m_texdef._scale[0] = Texdef_getDefaultTextureScale();
  projection.m_texdef._scale[1] = Texdef_getDefaultTextureScale();

  if(g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_BRUSHPRIMITIVES)
  {
    projection.m_brushprimit_texdef = BrushPrimitTexDef(projection.m_texdef);
  }
}

void ShiftScaleRotate_fromFace(TexDef& shiftScaleRotate, const TextureProjection& projection) {
	if(g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_BRUSHPRIMITIVES) {
		shiftScaleRotate = projection.m_brushprimit_texdef.getFakeTexCoords();
	}
	else {
		shiftScaleRotate = projection.m_texdef;
	}
}

void ShiftScaleRotate_toFace(const TexDef& shiftScaleRotate, TextureProjection& projection)
{
  if (g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_BRUSHPRIMITIVES)
  {
    // compute texture matrix
    // the matrix returned must be understood as a qtexture_t with width=2 height=2
    projection.m_brushprimit_texdef = BrushPrimitTexDef(shiftScaleRotate);
  }
  else
  {
    projection.m_texdef = shiftScaleRotate;
  }
}


inline Matrix4 matrix4_rotation_for_vector3(const Vector3& x, const Vector3& y, const Vector3& z)
{
  return Matrix4(
    x.x(), x.y(), x.z(), 0,
    y.x(), y.y(), y.z(), 0,
    z.x(), z.y(), z.z(), 0,
    0, 0, 0, 1
  );
}

inline Matrix4 matrix4_swap_axes(const Vector3& from, const Vector3& to)
{
  if(from.x() != 0 && to.y() != 0)
  {
    return matrix4_rotation_for_vector3(to, from, g_vector3_axis_z);
  }

  if(from.x() != 0 && to.z() != 0)
  {
    return matrix4_rotation_for_vector3(to, g_vector3_axis_y, from);
  }

  if(from.y() != 0 && to.z() != 0)
  {
    return matrix4_rotation_for_vector3(g_vector3_axis_x, to, from);
  }

  if(from.y() != 0 && to.x() != 0)
  {
    return matrix4_rotation_for_vector3(from, to, g_vector3_axis_z);
  }

  if(from.z() != 0 && to.x() != 0)
  {
    return matrix4_rotation_for_vector3(from, g_vector3_axis_y, to);
  }

  if(from.z() != 0 && to.y() != 0)
  {
    return matrix4_rotation_for_vector3(g_vector3_axis_x, from, to);
  }

  ERROR_MESSAGE("unhandled axis swap case");

  return g_matrix4_identity;
}

inline Matrix4 matrix4_reflection_for_plane(const Plane3& plane)
{
  return Matrix4(
    static_cast<float>(1 - (2 * plane.a * plane.a)),
    static_cast<float>(-2 * plane.a * plane.b),
    static_cast<float>(-2 * plane.a * plane.c),
    0,
    static_cast<float>(-2 * plane.b * plane.a),
    static_cast<float>(1 - (2 * plane.b * plane.b)),
    static_cast<float>(-2 * plane.b * plane.c),
    0,
    static_cast<float>(-2 * plane.c * plane.a),
    static_cast<float>(-2 * plane.c * plane.b),
    static_cast<float>(1 - (2 * plane.c * plane.c)),
    0,
    static_cast<float>(-2 * plane.d * plane.a),
    static_cast<float>(-2 * plane.d * plane.b),
    static_cast<float>(-2 * plane.d * plane.c),
    1
  );
}

inline Matrix4 matrix4_reflection_for_plane45(const Plane3& plane, const Vector3& from, const Vector3& to)
{
  Vector3 first = from;
  Vector3 second = to;

  if(from.dot(plane.normal()) > 0 == to.dot(plane.normal()) > 0)
  {
    first = -first;
    second = -second;
  }

#if 0
  globalOutputStream() << "normal: ";
  print_vector3(plane.normal());

  globalOutputStream() << "from: ";
  print_vector3(first);

  globalOutputStream() << "to: ";
  print_vector3(second);
#endif

  Matrix4 swap = matrix4_swap_axes(first, second);

  Matrix4 tmp = matrix4_reflection_for_plane(plane);

  swap.tx() = -static_cast<float>(-2 * plane.a * plane.d);
  swap.ty() = -static_cast<float>(-2 * plane.b * plane.d);
  swap.tz() = -static_cast<float>(-2 * plane.c * plane.d);

  return swap;
}

void TexDefransformLocked(TextureProjection& projection, std::size_t width, std::size_t height, const Plane3& plane, const Matrix4& identity2transformed)
{
  //globalOutputStream() << "identity2transformed: " << identity2transformed << "\n";

  //globalOutputStream() << "plane.normal(): " << plane.normal() << "\n";

  Vector3 normalTransformed(matrix4_transformed_direction(identity2transformed, plane.normal()));

  //globalOutputStream() << "normalTransformed: " << normalTransformed << "\n";

  // identity: identity space
  // transformed: transformation
  // stIdentity: base st projection space before transformation
  // stTransformed: base st projection space after transformation
  // stOriginal: original texdef space

  // stTransformed2stOriginal = stTransformed -> transformed -> identity -> stIdentity -> stOriginal

  Matrix4 identity2stIdentity;
  Texdef_basisForNormal(projection, plane.normal(), identity2stIdentity);
  //globalOutputStream() << "identity2stIdentity: " << identity2stIdentity << "\n";

  if(g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_HALFLIFE)
  {
    matrix4_transform_direction(identity2transformed, projection.m_basis_s);
    matrix4_transform_direction(identity2transformed, projection.m_basis_t);
  }

  Matrix4 transformed2stTransformed;
  Texdef_basisForNormal(projection, normalTransformed, transformed2stTransformed);

  Matrix4 stTransformed2identity(matrix4_affine_inverse(matrix4_multiplied_by_matrix4(transformed2stTransformed, identity2transformed)));

  Vector3 originalProjectionAxis(matrix4_affine_inverse(identity2stIdentity).z().getVector3());

  Vector3 transformedProjectionAxis(stTransformed2identity.z().getVector3());

  Matrix4 stIdentity2stOriginal = projection.getTransform((float)width, (float)height);
  Matrix4 identity2stOriginal(matrix4_multiplied_by_matrix4(stIdentity2stOriginal, identity2stIdentity));

  //globalOutputStream() << "originalProj: " << originalProjectionAxis << "\n";
  //globalOutputStream() << "transformedProj: " << transformedProjectionAxis << "\n";
  double dot = originalProjectionAxis.dot(transformedProjectionAxis);
  //globalOutputStream() << "dot: " << dot << "\n";
  if(dot == 0)
  {
    // The projection axis chosen for the transformed normal is at 90 degrees
    // to the transformed projection axis chosen for the original normal.
    // This happens when the projection axis is ambiguous - e.g. for the plane
    // 'X == Y' the projection axis could be either X or Y.
    //globalOutputStream() << "flipped\n";
#if 0
    globalOutputStream() << "projection off by 90\n";
    globalOutputStream() << "normal: ";
    print_vector3(plane.normal());
    globalOutputStream() << "original projection: ";
    print_vector3(originalProjectionAxis);
    globalOutputStream() << "transformed projection: ";
    print_vector3(transformedProjectionAxis);
#endif

    Matrix4 identityCorrected = matrix4_reflection_for_plane45(plane, originalProjectionAxis, transformedProjectionAxis);

    identity2stOriginal = matrix4_multiplied_by_matrix4(identity2stOriginal, identityCorrected);
  }

  Matrix4 stTransformed2stOriginal = matrix4_multiplied_by_matrix4(identity2stOriginal, stTransformed2identity);

  projection.setTransform((float)width, (float)height, stTransformed2stOriginal);
  projection.normalise((float)width, (float)height);
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
