#include "AseModel.h"

#include <fmt/format.h>
#include "parser/ParseException.h"

/* -----------------------------------------------------------------------------

PicoModel Library

Copyright (c) 2002, Randy Reddig & seaw0lf
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other aseMaterialList provided with the distribution.

Neither the names of the copyright holders nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------------- */

/* dependencies */
#include "../picomodel/lib/picointernal.h"

/* plain white */
static picoColor_t white = { 255, 255, 255, 255 };

/* jhefty - multi-subobject material support */

/* Material/SubMaterial management */
/* A material should have 1..n submaterials assigned to it */

typedef struct aseSubMaterial_s
{
	struct aseSubMaterial_s* next;
	int subMtlId;
	picoShader_t* shader;

	float uOffset; /* UVW_U_OFFSET */
	float vOffset; /* UVW_V_OFFSET */
	float uScale;  /* UVW_U_TILING */
	float vScale;  /* UVW_V_TILING */
	float uvAngle; /* UVW_ANGLE */
} aseSubMaterial_t;

typedef struct aseMaterial_s
{
	struct aseMaterial_s* next;
	struct aseSubMaterial_s* subMtls;
	int mtlId;
} aseMaterial_t;

/* Material/SubMaterial management functions */
static aseMaterial_t* _ase_get_material ( aseMaterial_t* list , int mtlIdParent )
{
	aseMaterial_t* mtl = list;

	while ( mtl )
	{
		if ( mtlIdParent == mtl->mtlId )
		{
			break;
		}
		mtl = mtl->next;
	}
	return mtl;
}

static aseSubMaterial_t* _ase_get_submaterial ( aseMaterial_t* list, int  mtlIdParent , int subMtlId )
{
	aseMaterial_t* parent = _ase_get_material ( list , mtlIdParent );
	aseSubMaterial_t* subMtl = NULL;

	if ( !parent )
	{
		_pico_printf ( PICO_ERROR , "No ASE material exists with id %i\n" , mtlIdParent );
		return NULL;
	}

	subMtl = parent->subMtls;
	while ( subMtl )
	{
		if ( subMtlId == subMtl->subMtlId )
		{
			break;
		}
		subMtl = subMtl->next;
	}
	return subMtl;
}

aseSubMaterial_t* _ase_get_submaterial_or_default ( aseMaterial_t* materials, int  mtlIdParent , int subMtlId )
{
	aseSubMaterial_t* subMtl = _ase_get_submaterial( materials, mtlIdParent, subMtlId );
	if(subMtl != NULL)
	{
		return subMtl;
	}

	/* ydnar: trying default submaterial */
	subMtl = _ase_get_submaterial( materials, mtlIdParent, 0 );
	if( subMtl != NULL )
	{
		return subMtl;
	}

	_pico_printf( PICO_ERROR, "Could not find material/submaterial for id %d/%d\n", mtlIdParent, subMtlId );
	return NULL;
}




static aseMaterial_t* _ase_add_material( aseMaterial_t **list, int mtlIdParent )
{
	aseMaterial_t *mtl = (aseMaterial_t*)_pico_calloc( 1, sizeof( aseMaterial_t ) );
	mtl->mtlId = mtlIdParent;
	mtl->subMtls = NULL;
	mtl->next = *list;
	*list = mtl;

	return mtl;
}

static aseSubMaterial_t* _ase_add_submaterial( aseMaterial_t **list, int mtlIdParent, int subMtlId, picoShader_t* shader )
{
	aseMaterial_t *parent = _ase_get_material( *list,  mtlIdParent );
	aseSubMaterial_t *subMtl = (aseSubMaterial_t*)_pico_calloc( 1, sizeof ( aseSubMaterial_t ) );

	/* Initialise some values */
	subMtl->uOffset = 0.0f;
	subMtl->vOffset = 0.0f;
	subMtl->uScale = 1.0f;
	subMtl->vScale = 1.0f;
	subMtl->uvAngle = 0.0f;

	if ( !parent )
	{
		parent = _ase_add_material ( list , mtlIdParent );
	}

	subMtl->shader = shader;
	subMtl->subMtlId = subMtlId;
	subMtl->next = parent->subMtls;
	parent->subMtls = subMtl;

	return subMtl;
}

static void _ase_free_materials( aseMaterial_t **list )
{
	aseMaterial_t* mtl = *list;
	aseSubMaterial_t* subMtl = NULL;

	aseMaterial_t* mtlTemp = NULL;
	aseSubMaterial_t* subMtlTemp = NULL;

	while ( mtl )
	{
		subMtl = mtl->subMtls;
		while ( subMtl )
		{
			subMtlTemp = subMtl->next;
			_pico_free ( subMtl );
			subMtl = subMtlTemp;
		}
		mtlTemp = mtl->next;
		_pico_free ( mtl );
		mtl = mtlTemp;
	}
	(*list) = NULL;
}

/* todo:
 * - apply material specific uv offsets to uv coordinates
 */

/* _ase_canload:
 *  validates a 3dsmax ase model file.
 */
static int _ase_canload( PM_PARAMS_CANLOAD )
{
	picoParser_t *p;


	/* quick data length validation */
	if( bufSize < 80 )
		return PICO_PMV_ERROR_SIZE;

	/* keep the friggin compiler happy */
	*fileName = *fileName;

	/* create pico parser */
	p = _pico_new_parser( (picoByte_t*) buffer, bufSize );
	if( p == NULL )
		return PICO_PMV_ERROR_MEMORY;

	/* get first token */
	if( _pico_parse_first( p ) == NULL)
	{
		return PICO_PMV_ERROR_IDENT;
	}

	/* check first token */
	if( _pico_stricmp( p->token, "*3dsmax_asciiexport" ) )
	{
		_pico_free_parser( p );
		return PICO_PMV_ERROR_IDENT;
	}

	/* free the pico parser object */
	_pico_free_parser( p );

	/* file seems to be a valid ase file */
	return PICO_PMV_OK;
}

typedef struct aseVertex_s aseVertex_t;
struct aseVertex_s
{
	picoVec3_t xyz;
	picoVec3_t normal;
	picoIndex_t id;
};

typedef struct aseTexCoord_s aseTexCoord_t;
struct aseTexCoord_s
{
	picoVec2_t texcoord;
};

typedef struct aseColor_s aseColor_t;
struct aseColor_s
{
	picoColor_t color;
};

typedef struct aseFace_s aseFace_t;
struct aseFace_s
{
	picoIndex_t indices[9];
	picoIndex_t smoothingGroup;
	picoIndex_t materialId;
	picoIndex_t subMaterialId;
};
typedef aseFace_t* aseFacesIter_t;

picoSurface_t* PicoModelFindOrAddSurface( picoModel_t *model, picoShader_t* shader )
{
	/* see if a surface already has the shader */
	int i = 0;
	for ( ; i < model->numSurfaces ; i++ )
	{
		picoSurface_t* workSurface = model->surface[i];
		if ( workSurface->shader == shader )
		{
			return workSurface;
		}
	}

	/* no surface uses this shader yet, so create a new surface */

	{
		/* create a new surface in the model for the unique shader */
		picoSurface_t* workSurface = PicoNewSurface(model);
		if ( !workSurface )
		{
			_pico_printf ( PICO_ERROR , "Could not allocate a new surface!\n" );
			return 0;
		}

		/* do surface setup */
		PicoSetSurfaceType( workSurface, PICO_TRIANGLES );
		PicoSetSurfaceName( workSurface, shader->name );
		PicoSetSurfaceShader( workSurface, shader );

		return workSurface;
	}
}

static void _ase_submit_triangles( model::AseModel& model , aseMaterial_t* materials , std::vector<ArbitraryMeshVertex>& vertices, aseTexCoord_t* texcoords, aseColor_t* colors, aseFace_t* faces, int numFaces )
{
	aseFacesIter_t i = faces, end = faces + numFaces;
	for(; i != end; ++i)
	{
		/* look up the shader for the material/submaterial pair */
		aseSubMaterial_t* subMtl = _ase_get_submaterial_or_default( materials, (*i).materialId, (*i).subMaterialId );
		if( subMtl == NULL )
		{
			return;
		}

		{
			Vertex3f* xyz[3];
			Normal3f* normal[3];
			picoVec2_t* stRef[3];
			picoVec2_t st[3];
			picoColor_t* color[3];
			//picoIndex_t smooth[3];
			double u,v;

			double materialSin = sin(subMtl->uvAngle);
			double materialCos = cos(subMtl->uvAngle);

			int j;
			/* we pull the data from the vertex, color and texcoord arrays using the face index data */
			for ( j = 0 ; j < 3 ; j ++ )
			{
				xyz[j]    = &vertices[(*i).indices[j]].vertex;
				normal[j] = &vertices[(*i).indices[j]].normal;

                /* greebo: Apply shift, scale and rotation */
				/* Also check for NULL texcoords pointer, some models surfaces don't have any tverts */
				u = texcoords != NULL ? texcoords[(*i).indices[j + 3]].texcoord[0] * subMtl->uScale + subMtl->uOffset : 0.0;
				v = texcoords != NULL ? texcoords[(*i).indices[j + 3]].texcoord[1] * subMtl->vScale + subMtl->vOffset : 0.0;

				st[j][0] = u * materialCos + v * materialSin;
				st[j][1] = u * -materialSin + v * materialCos;

				stRef[j] = &st[j];

				if( colors != NULL && (*i).indices[j + 6] >= 0 )
				{
					color[j] = &colors[(*i).indices[j + 6]].color;
				}
				else
				{
					color[j] = &white;
				}

				//smooth[j] = (vertices[(*i).indices[j]].id * (1 << 16)) + (*i).smoothingGroup; /* don't merge vertices */

			}

			/* submit the triangle to the model */
            auto& surface = model.ensureSurface(subMtl->shader->mapName ? subMtl->shader->mapName : "");

            auto nextIndex = static_cast<unsigned int>(surface.indices.size());

            surface.vertices.emplace_back(ArbitraryMeshVertex{ Vertex3f(*(xyz[0])), Normal3f(*(normal[0])), TexCoord2f(st[0]) });
            surface.vertices.emplace_back(ArbitraryMeshVertex{ Vertex3f(*(xyz[1])), Normal3f(*(normal[1])), TexCoord2f(st[1]) });
            surface.vertices.emplace_back(ArbitraryMeshVertex{ Vertex3f(*(xyz[2])), Normal3f(*(normal[2])), TexCoord2f(st[2]) });

            surface.indices.emplace_back(nextIndex++);
            surface.indices.emplace_back(nextIndex++);
            surface.indices.emplace_back(nextIndex++);
            
			// TODO PicoAddTriangleToModel ( model , xyz , normal , 1 , stRef, 1 , color , subMtl->shader, smooth );
		}
	}
}

static void shadername_convert(char* shaderName)
{
  /* unix-style path separators */
  char* s = shaderName;
  for(; *s != '\0'; ++s)
  {
    if(*s == '\\')
    {
      *s = '/';
    }
  }
}

namespace model
{

AseModel::Surface& AseModel::addSurface(const std::string& name)
{
    return _surfaces.emplace_back(Surface{name});
}

AseModel::Surface& AseModel::ensureSurface(const std::string& name)
{
    for (auto& surface : _surfaces)
    {
        if (surface.material == name)
        {
            return surface;
        }
    }

    return addSurface(name);
}

std::vector<AseModel::Surface>& AseModel::getSurfaces()
{
    return _surfaces;
}

std::shared_ptr<AseModel> AseModel::CreateFromStream(std::istream& stream)
{
    auto model = std::make_shared<AseModel>();

	picoParser_t   *p;

    std::vector<ArbitraryMeshVertex> vertices;
	
	aseTexCoord_t* texcoords = NULL;
	aseColor_t* colors = NULL;
	aseFace_t* faces = NULL;
	int numVertices = 0;
	int numFaces = 0;
	int numTextureVertices = 0;
	int numTextureVertexFaces = 0;
	int numColorVertices = 0;
	int numColorVertexFaces = 0;
	int vertexId = 0;

	aseMaterial_t* materials = NULL;

	/* helper */
	#define _ase_error_return(m) \
	{ \
		throw parser::ParseException(fmt::format("{0} in ASE, line {0:d}.", m, p->curLine)); \
		_pico_free_parser( p ); \
		return model; \
	}

    // Temporary buffer to get the pico parser up and running
    std::stringstream buffer;
    buffer << stream.rdbuf();
    std::string stringBuffer = buffer.str();

	/* create a new pico parser */
	p = _pico_new_parser( (picoByte_t *)stringBuffer.c_str(), static_cast<int>(stringBuffer.length()));
	if (p == NULL) return NULL;

	/* parse ase model file */
	while( 1 )
	{
		/* get first token on line */
		if (_pico_parse_first( p ) == NULL)
			break;

		/* we just skip empty lines */
		if (p->token == NULL || !strlen( p->token ))
			continue;

		/* we skip invalid ase statements */
		if (p->token[0] != '*' && p->token[0] != '{' && p->token[0] != '}')
		{
			_pico_parse_skip_rest( p );
			continue;
		}
		/* remember node name */
		if (!_pico_stricmp(p->token,"*node_name"))
		{
			/* read node name */
			char *ptr = _pico_parse( p,0 );
			if (ptr == NULL)
				_ase_error_return("Node name parse error");
		}
		/* model mesh (originally contained within geomobject) */
		else if (!_pico_stricmp(p->token,"*mesh"))
		{
			/* finish existing surface */
			_ase_submit_triangles(*model, materials, vertices, texcoords, colors, faces, numFaces);
			_pico_free(faces);
			_pico_free(texcoords);
			_pico_free(colors);
			colors = NULL; /* OrbWeaver: reset all pointers to avoid double-free */
			faces = NULL;
			vertices.clear();
			texcoords = NULL;
		}
		else if (!_pico_stricmp(p->token,"*mesh_numvertex"))
		{
			if (!_pico_parse_int( p, &numVertices) )
				_ase_error_return("Missing MESH_NUMVERTEX value");

			vertices.resize(numVertices);
		}
		else if (!_pico_stricmp(p->token,"*mesh_numfaces"))
		{
			if (!_pico_parse_int( p, &numFaces) )
				_ase_error_return("Missing MESH_NUMFACES value");

			faces = (aseFace_t*)_pico_calloc(numFaces, sizeof(aseFace_t));
		}
		else if (!_pico_stricmp(p->token,"*mesh_numtvertex"))
		{
			if (!_pico_parse_int( p, &numTextureVertices) )
				_ase_error_return("Missing MESH_NUMTVERTEX value");

			texcoords = (aseTexCoord_t*)_pico_calloc(numTextureVertices, sizeof(aseTexCoord_t));
		}
		else if (!_pico_stricmp(p->token,"*mesh_numtvfaces"))
		{
			if (!_pico_parse_int( p, &numTextureVertexFaces) )
				_ase_error_return("Missing MESH_NUMTVFACES value");
		}
		else if (!_pico_stricmp(p->token,"*mesh_numcvertex"))
		{
			if (!_pico_parse_int( p, &numColorVertices) )
				_ase_error_return("Missing MESH_NUMCVERTEX value");

			colors = (aseColor_t*)_pico_calloc(numColorVertices, sizeof(aseColor_t));
			memset( colors, 255, numColorVertices * sizeof( aseColor_t ) );	/* ydnar: force colors to white initially */
		}
		else if (!_pico_stricmp(p->token,"*mesh_numcvfaces"))
		{
			if (!_pico_parse_int( p, &numColorVertexFaces) )
				_ase_error_return("Missing MESH_NUMCVFACES value");
		}
		/* mesh material reference. this usually comes at the end of */
		/* geomobjects after the mesh blocks. we must assume that the */
		/* new mesh was already created so all we can do here is assign */
		/* the material reference id (shader index) now. */
		else if (!_pico_stricmp(p->token,"*material_ref"))
		{
			int mtlId;

			/* get the material ref (0..n) */
			if (!_pico_parse_int( p,&mtlId) )
				_ase_error_return("Missing material reference ID");

			{
				int i = 0;
				/* fix up all of the aseFaceList in the surface to point to the parent material */
				/* we've already saved off their subMtl */
				for(; i < numFaces; ++i)
				{
					faces[i].materialId = mtlId;
				}
			}
		}
		/* model mesh vertex */
		else if (!_pico_stricmp(p->token,"*mesh_vertex"))
		{
			int			index;

			if( numVertices == 0 )
				_ase_error_return("Vertex parse error");

			/* get vertex data (orig: index +y -x +z) */
			if (!_pico_parse_int( p,&index ))
				_ase_error_return("Vertex parse error");
			if (!_pico_parse_vec( p,vertices[index].vertex ))
				_ase_error_return("Vertex parse error");

			if (index >= numVertices)
			{
				_ase_error_return("Error: MESH_VERTEX index out of bounds (is >= MESH_NUMVERTEX)");
			}

			//vertices[index].id = vertexId++;
		}
		/* model mesh vertex normal */
		else if (!_pico_stricmp(p->token,"*mesh_vertexnormal"))
		{
			int			index;

			if( numVertices == 0 )
				_ase_error_return("Vertex parse error");

			/* get vertex data (orig: index +y -x +z) */
			if (!_pico_parse_int( p,&index ))
				_ase_error_return("Vertex parse error");

			if (index >= numVertices)
			{
				_ase_error_return("Error: MESH_VERTEXNORMAL index out of bounds (is >= MESH_NUMVERTEX)");
			}

			if (!_pico_parse_vec( p,vertices[index].normal ))
				_ase_error_return("Vertex parse error");
		}
		/* model mesh face */
		else if (!_pico_stricmp(p->token,"*mesh_face"))
		{
			picoIndex_t indexes[3];
			int index;

			if( numFaces == 0 )
				_ase_error_return("Face parse error");

			/* get face index */
			if (!_pico_parse_int( p,&index ))
				_ase_error_return("Face parse error");

			if (index >= numFaces)
			{
				_ase_error_return("Error: MESH_FACE index out of bounds (is >= MESH_NUMFACES)");
			}

			/* get 1st vertex index */
			_pico_parse( p,0 );
			if (!_pico_parse_int( p,&indexes[0] ))
				_ase_error_return("Face parse error");

			if (indexes[0] >= numVertices)
			{
				_ase_error_return("Error: MESH_FACE: vertex index 0 out of bounds (is >= MESH_NUMVERTEX)");
			}

			/* get 2nd vertex index */
			_pico_parse( p,0 );
			if (!_pico_parse_int( p,&indexes[1] ))
				_ase_error_return("Face parse error");

			if (indexes[1] >= numVertices)
			{
				_ase_error_return("Error: MESH_FACE: vertex index 1 out of bounds (is >= MESH_NUMVERTEX)");
			}

			/* get 3rd vertex index */
			_pico_parse( p,0 );
			if (!_pico_parse_int( p,&indexes[2] ))
				_ase_error_return("Face parse error");

			if (indexes[2] >= numVertices)
			{
				_ase_error_return("Error: MESH_FACE: vertex index 2 out of bounds (is >= MESH_NUMVERTEX)");
			}

			/* parse to the subMaterial ID */
			while ( 1 )
			{
				if (!_pico_parse (p,0)) /* EOL */
				{
					break;
				}
				if (!_pico_stricmp (p->token,"*MESH_SMOOTHING" ))
				{
					_pico_parse_int ( p , &faces[index].smoothingGroup );
				}
				if (!_pico_stricmp (p->token,"*MESH_MTLID" ))
				{
					_pico_parse_int ( p , &faces[index].subMaterialId );
				}
			}

			faces[index].materialId = 0;
			faces[index].indices[0] = indexes[2];
			faces[index].indices[1] = indexes[1];
			faces[index].indices[2] = indexes[0];
		}
		/* model texture vertex */
		else if (!_pico_stricmp(p->token,"*mesh_tvert"))
		{
			int			index;

			if( numVertices == 0 )
				_ase_error_return("Texture Vertex parse error");

			/* get uv vertex index */
			if (!_pico_parse_int( p,&index ) || index >= numTextureVertices)
				_ase_error_return("Texture vertex parse error");

			/* get uv vertex s */
			if (!_pico_parse_double( p,&texcoords[index].texcoord[0] ))
				_ase_error_return("Texture vertex parse error");

			/* get uv vertex t */
			if (!_pico_parse_double( p,&texcoords[index].texcoord[1] ))
				_ase_error_return("Texture vertex parse error");

			/* ydnar: invert t */
			texcoords[index].texcoord[ 1 ] = 1.0f - texcoords[index].texcoord[ 1 ];
		}
		/* ydnar: model mesh texture face */
		else if( !_pico_stricmp( p->token, "*mesh_tface" ) )
		{
			picoIndex_t indexes[3];
			int			index;

			if( numFaces == 0 )
				_ase_error_return("Texture face parse error");

			/* get face index */
			if (!_pico_parse_int( p,&index ))
				_ase_error_return("Texture face parse error");

			if (index >= numFaces)
			{
				_ase_error_return("Error: MESH_TFACE index out of bounds (is >= MESH_NUMFACES)");
			}

			/* get 1st vertex index */
			if (!_pico_parse_int( p,&indexes[0] ))
				_ase_error_return("Texture face parse error");

			/* get 2nd vertex index */
			if (!_pico_parse_int( p,&indexes[1] ))
				_ase_error_return("Texture face parse error");

			/* get 3rd vertex index */
			if (!_pico_parse_int( p,&indexes[2] ))
				_ase_error_return("Texture face parse error");

			faces[index].indices[3] = indexes[2];
			faces[index].indices[4] = indexes[1];
			faces[index].indices[5] = indexes[0];
		}
		/* model color vertex */
		else if (!_pico_stricmp(p->token,"*mesh_vertcol"))
		{
			int			index;
			float		colorInput;

			if( numVertices == 0 )
				_ase_error_return("Color Vertex parse error");

			/* get color vertex index */
			if (!_pico_parse_int( p,&index ))
				_ase_error_return("Color vertex parse error");

			if (index >= numColorVertices)
			{
				_ase_error_return("Error: MESH_VERTCOL index out of bounds (is >= MESH_NUMCVERTEX)");
			}

			/* get R component */
			if (!_pico_parse_float( p,&colorInput ))
				_ase_error_return("Color vertex parse error");
			colors[index].color[0] = (picoByte_t)(colorInput * 255);

			/* get G component */
			if (!_pico_parse_float( p,&colorInput ))
				_ase_error_return("Color vertex parse error");
			colors[index].color[1] = (picoByte_t)(colorInput * 255);

			/* get B component */
			if (!_pico_parse_float( p,&colorInput ))
				_ase_error_return("Color vertex parse error");
			colors[index].color[2] = (picoByte_t)(colorInput * 255);

			/* leave alpha alone since we don't get any data from the ASE format */
			colors[index].color[3] = 255;
		}
		/* model color face */
		else if (!_pico_stricmp(p->token,"*mesh_cface"))
		{
			picoIndex_t indexes[3];
			int			index;

			if( numFaces == 0 )
				_ase_error_return("Face parse error");

			/* get face index */
			if (!_pico_parse_int( p,&index ))
				_ase_error_return("Face parse error");

			if (index >= numFaces)
			{
				_ase_error_return("Error: MESH_CFACE index out of bounds (is >= MESH_NUMFACES)");
			}

			/* get 1st cvertex index */
			//			_pico_parse( p,0 );
			if (!_pico_parse_int( p,&indexes[0] ))
				_ase_error_return("Face parse error");

			/* get 2nd cvertex index */
			//			_pico_parse( p,0 );
			if (!_pico_parse_int( p,&indexes[1] ))
				_ase_error_return("Face parse error");

			/* get 3rd cvertex index */
			//			_pico_parse( p,0 );
			if (!_pico_parse_int( p,&indexes[2] ))
				_ase_error_return("Face parse error");

			faces[index].indices[6] = indexes[2];
			faces[index].indices[7] = indexes[1];
			faces[index].indices[8] = indexes[0];
		}
		/* model material */
		else if( !_pico_stricmp( p->token, "*material" ) )
		{
			aseSubMaterial_t*	subMaterial = NULL;
			picoShader_t		*shader = NULL;
			int					level = 1, index;
			char				materialName[ 1024 ];
			float				transValue = 0.0f, shineValue = 1.0f;
			picoColor_t			ambientColor, diffuseColor, specularColor;
			char				*mapname = NULL;
			int					subMtlId = -1, subMaterialLevel = -1;
			float				uOffset = 0.0f;
			float				vOffset = 0.0f;
			float				uScale = 1.0f;
			float				vScale = 1.0f;
			float				uvAngle = 0;

			/* get material index */
			_pico_parse_int( p,&index );

			/* check brace */
			if (!_pico_parse_check(p,1,"{"))
				_ase_error_return("Material missing opening brace");

			/* parse material block */
			while( 1 )
			{
				/* get next token */
				if (_pico_parse(p,1) == NULL) break;
				if (!strlen(p->token)) continue;

				/* handle levels */
				if (p->token[0] == '{') level++;
				if (p->token[0] == '}') level--;
				if (!level) break;

				if( level == subMaterialLevel )
				{
					/* set material name */
					_pico_first_token( materialName );

					shadername_convert(materialName);

					PicoSetShaderName( shader, materialName);

					/* set shader's transparency */
					PicoSetShaderTransparency( shader,transValue );

					/* set shader's ambient color */
					PicoSetShaderAmbientColor( shader,ambientColor );

					/* set diffuse alpha to transparency */
					diffuseColor[3] = (picoByte_t)( transValue * 255.0 );

					/* set shader's diffuse color */
					PicoSetShaderDiffuseColor( shader,diffuseColor );

					/* set shader's specular color */
					PicoSetShaderSpecularColor( shader,specularColor );

					/* set shader's shininess */
					PicoSetShaderShininess( shader,shineValue );

					/* set material map name */
					PicoSetShaderMapName( shader, mapname );

					subMaterial = _ase_add_submaterial( &materials, index, subMtlId, shader );

					// Assign offsets
					subMaterial->uOffset = uOffset;
					subMaterial->vOffset = vOffset;
					subMaterial->uScale = uScale;
					subMaterial->vScale = vScale;
					subMaterial->uvAngle = uvAngle;

					subMaterialLevel = -1;
				}

				/* parse submaterial index */
#if 0
				if (!_pico_stricmp(p->token,"*submaterial"))
				{
					/* allocate new pico shader */
					_pico_parse_int( p , &subMtlId );

					shader = PicoNewShader( model );
					if (shader == NULL)
					{
						PicoFreeModel( model );
						return NULL;
					}
					subMaterialLevel = level;
				}
				else
#endif
				/* parse material name */
                if (!_pico_stricmp(p->token,"*material_name"))
				{
					char* name = _pico_parse(p,0);
					if ( name == NULL)
						_ase_error_return("Missing material name");

					strcpy ( materialName , name );
					/* skip rest and continue with next token */
					_pico_parse_skip_rest( p );
					continue;
				}
				/* parse material transparency */
				else if (!_pico_stricmp(p->token,"*material_transparency"))
				{
					/* get transparency value from ase */
					if (!_pico_parse_float( p,&transValue ))
						_ase_error_return("Material transparency parse error");

					/* skip rest and continue with next token */
					_pico_parse_skip_rest( p );
					continue;
				}
				/* parse material shininess */
				else if (!_pico_stricmp(p->token,"*material_shine"))
				{
					/* remark:
					 * - not sure but instead of '*material_shine' i might
					 *   need to use '*material_shinestrength' */

					/* get shine value from ase */
					if (!_pico_parse_float( p,&shineValue ))
						_ase_error_return("Material shine parse error");

					/* scale ase shine range 0..1 to pico range 0..127 */
					shineValue *= 128.0;

					/* skip rest and continue with next token */
					_pico_parse_skip_rest( p );
					continue;
				}
				/* parse ambient material color */
				else if (!_pico_stricmp(p->token,"*material_ambient"))
				{
					picoVec3_t  vec;
					/* get r,g,b float values from ase */
					if (!_pico_parse_vec( p,vec ))
						_ase_error_return("Material color parse error");

					/* setup 0..255 range color values */
					ambientColor[ 0 ] = (picoByte_t)( vec[ 0 ] * 255.0 );
					ambientColor[ 1 ] = (picoByte_t)( vec[ 1 ] * 255.0 );
					ambientColor[ 2 ] = (picoByte_t)( vec[ 2 ] * 255.0 );
					ambientColor[ 3 ] = (picoByte_t)( 255 );

					/* skip rest and continue with next token */
					_pico_parse_skip_rest( p );
					continue;
				}
				/* parse diffuse material color */
				else if (!_pico_stricmp(p->token,"*material_diffuse"))
				{
					picoVec3_t  vec;

					/* get r,g,b float values from ase */
					if (!_pico_parse_vec( p,vec ))
						_ase_error_return("Material color parse error");

					/* setup 0..255 range color */
					diffuseColor[ 0 ] = (picoByte_t)( vec[ 0 ] * 255.0 );
					diffuseColor[ 1 ] = (picoByte_t)( vec[ 1 ] * 255.0 );
					diffuseColor[ 2 ] = (picoByte_t)( vec[ 2 ] * 255.0 );
					diffuseColor[ 3 ] = (picoByte_t)( 255 );

					/* skip rest and continue with next token */
					_pico_parse_skip_rest( p );
					continue;
				}
				/* parse specular material color */
				else if (!_pico_stricmp(p->token,"*material_specular"))
				{
					picoVec3_t  vec;

					/* get r,g,b float values from ase */
					if (!_pico_parse_vec( p,vec ))
						_ase_error_return("Material color parse error");

					/* setup 0..255 range color */
					specularColor[ 0 ] = (picoByte_t)( vec[ 0 ] * 255 );
					specularColor[ 1 ] = (picoByte_t)( vec[ 1 ] * 255 );
					specularColor[ 2 ] = (picoByte_t)( vec[ 2 ] * 255 );
					specularColor[ 3 ] = (picoByte_t)( 255 );

					/* skip rest and continue with next token */
					_pico_parse_skip_rest( p );
					continue;
				}
				/* material diffuse map */
				else if (!_pico_stricmp(p->token,"*map_diffuse") )
				{
					int sublevel = 0;

					/* parse material block */
					while( 1 )
					{
						/* get next token */
						if (_pico_parse(p,1) == NULL) break;
						if (!strlen(p->token)) continue;

						/* handle levels */
						if (p->token[0] == '{') sublevel++;
						if (p->token[0] == '}') sublevel--;
						if (!sublevel) break;

						/* parse diffuse map bitmap */
						if (!_pico_stricmp(p->token,"*bitmap"))
						{
							char* name = _pico_parse(p,0);
							if (name == NULL)
								_ase_error_return("Missing material map bitmap name");
							mapname = (char*)_pico_alloc ( strlen ( name ) + 1 );
							strcpy ( mapname, name );
							/* skip rest and continue with next token */
							_pico_parse_skip_rest( p );
							continue;
						}

						if (!_pico_stricmp(p->token, "*uvw_u_offset"))
						{
							if (!_pico_parse_float(p, &uOffset))
								_ase_error_return("Material UVW_U_OFFSET error");

							// Negate the u offset value
							uOffset = -uOffset;

							/* skip rest and continue with next token */
							_pico_parse_skip_rest(p);
							continue;
						}

						if (!_pico_stricmp(p->token, "*uvw_v_offset"))
						{
							if (!_pico_parse_float(p, &vOffset))
								_ase_error_return("Material UVW_V_OFFSET error");

							/* skip rest and continue with next token */
							_pico_parse_skip_rest(p);
							continue;
						}

						if (!_pico_stricmp(p->token, "*uvw_u_tiling"))
						{
							if (!_pico_parse_float(p, &uScale))
								_ase_error_return("Material UVW_U_TILING error");

							/* skip rest and continue with next token */
							_pico_parse_skip_rest(p);
							continue;
						}

						if (!_pico_stricmp(p->token, "*uvw_v_tiling"))
						{
							if (!_pico_parse_float(p, &vScale))
								_ase_error_return("Material UVW_V_TILING error");

							/* skip rest and continue with next token */
							_pico_parse_skip_rest(p);
							continue;
						}

						if (!_pico_stricmp(p->token, "*uvw_angle"))
						{
							if (!_pico_parse_float(p, &uvAngle))
								_ase_error_return("Material UVW_ANGLE error");

							/* skip rest and continue with next token */
							_pico_parse_skip_rest(p);
							continue;
						}
					}
				}
				/* end map_diffuse block */
			}
			/* end material block */

			if( subMaterial == NULL )
			{
                /* allocate and clear */
                shader = (picoShader_t*)_pico_alloc(sizeof(picoShader_t));
                memset(shader, 0, sizeof(picoShader_t));

				/* set material name */
				shadername_convert(materialName);
				PicoSetShaderName( shader,materialName );

				/* set shader's transparency */
				PicoSetShaderTransparency( shader,transValue );

				/* set shader's ambient color */
				PicoSetShaderAmbientColor( shader,ambientColor );

				/* set diffuse alpha to transparency */
				diffuseColor[3] = (picoByte_t)( transValue * 255.0 );

				/* set shader's diffuse color */
				PicoSetShaderDiffuseColor( shader,diffuseColor );

				/* set shader's specular color */
				PicoSetShaderSpecularColor( shader,specularColor );

				/* set shader's shininess */
				PicoSetShaderShininess( shader,shineValue );

				/* set material map name */
				PicoSetShaderMapName( shader, mapname );

				/* extract shadername from bitmap path */
				if(mapname != NULL)
				{
					char* mp = mapname;

					/* convert to shader-name format */
					shadername_convert(mapname);
					{
						/* remove extension */
						char* last_period = strrchr(mp, '.');
						if (last_period != NULL)
						{
							*last_period = '\0';
						}
					}

					/* find game root */
					for(; *mp != '\0'; ++mp)
					{
						if (_pico_strnicmp(mp, "quake", 5) == 0 || _pico_strnicmp(mp, "doom", 4) == 0)
						{
							break;
						}
					}
					/* root-relative */
					for (; *mp != '\0'; ++mp)
					{
						if (*mp == '/')
						{
							++mp;
							break;
						}
					}
					/* game-relative */
					for (; *mp != '\0'; ++mp)
					{
						if (*mp == '/')
						{
							++mp;
							break;
						}
					}

					if (*mp != '\0')
					{
						/* set material name */
						PicoSetShaderName(shader, mp);
					}
				}

				/* this is just a material with 1 submaterial */
				subMaterial = _ase_add_submaterial( &materials, index, 0, shader );

				// Assign offsets
				subMaterial->uOffset = uOffset;
				subMaterial->vOffset = vOffset;
				subMaterial->uScale = uScale;
				subMaterial->vScale = vScale;
				subMaterial->uvAngle = uvAngle;
			}

			/* ydnar: free mapname */
			if( mapname != NULL )
				_pico_free( mapname );
		}	// !_pico_stricmp ( "*material" )

		/* skip unparsed rest of line and continue */
		_pico_parse_skip_rest( p );
	}

	/* ydnar: finish existing surface */
	_ase_submit_triangles(*model, materials, vertices, texcoords, colors, faces, numFaces);
	_pico_free(faces);
	_pico_free(texcoords);
	_pico_free(colors);

	_ase_free_materials(&materials);

  _pico_free_parser( p );

    return model;
}

} // namespace
