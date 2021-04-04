#include "AseModel.h"

#include <fmt/format.h>
#include "parser/DefTokeniser.h"
#include "parser/ParseException.h"
#include "string/case_conv.h"
#include "string/trim.h"

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

namespace model
{

struct AseFace
{
    AseFace()
    {
        vertexIndices[0] = vertexIndices[1] = vertexIndices[2] = 0;
        texcoordIndices[0] = texcoordIndices[1] = texcoordIndices[2] = 0;
        colourIndices[0] = colourIndices[1] = colourIndices[2] = 0;
    }

    std::size_t vertexIndices[3];
    std::size_t texcoordIndices[3];
    std::size_t colourIndices[3];
};

struct AseMaterial
{
    AseMaterial() :
        uOffset(0),
        vOffset(0),
        uTiling(1),
        vTiling(1),
        angle(0)
    {}

    std::string materialName;   // *MATERIAL_NAME
    std::string diffuseBitmap;  // *BITMAP

    float uOffset;              // * UVW_U_OFFSET
    float vOffset;              // * UVW_V_OFFSET
    float uTiling;              // * UVW_U_TILING
    float vTiling;              // * UVW_V_TILING
    float angle;                // * UVW_ANGLE
};

}

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

void _ase_submit_triangles(model::AseModel& model, std::vector<model::AseMaterial>& materials, 
    std::vector<ArbitraryMeshVertex>& vertices, std::vector<TexCoord2f>& texcoords, 
    std::vector<Vector4>& colors, std::vector<model::AseFace>& faces)
{
#if 0
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
#endif
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

    parser::BasicDefTokeniser tokeniser(stream);

    std::vector<AseMaterial> materials;
    std::vector<ArbitraryMeshVertex> vertices;
    std::vector<AseFace> faces;
    std::vector<TexCoord2f> texcoords;
    std::vector<Vector4> colours;

    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        string::to_lower(token);

        if (token.length() == 0) continue;

        /* we skip invalid ase statements */
        if (token[0] != '*' && token[0] != '{' && token[0] != '}')
        {
            continue;
        }

        /* model mesh (originally contained within geomobject) */
        if (token == "*mesh")
        {
            /* finish existing surface */
            _ase_submit_triangles(*model, materials, vertices, texcoords, colours, faces);
            colours.clear();
            texcoords.clear();
            faces.clear();
            vertices.clear();
        }
        else if (token == "*mesh_numvertex")
        {
            // Parse the number to allocate space in the vertex vector
            auto numVertices = string::convert<std::size_t>(tokeniser.nextToken());
            vertices.resize(numVertices);
        }
        else if (token == "*mesh_numfaces")
        {
            auto numFaces = string::convert<std::size_t>(tokeniser.nextToken());
            faces.resize(numFaces);
        }
        else if (token == "*mesh_numtvertex")
        {
            auto numTextureVertices = string::convert<std::size_t>(tokeniser.nextToken());
            texcoords.resize(numTextureVertices);
        }
        else if (token == "*mesh_numcvertex")
        {
            auto numColorVertices = string::convert<std::size_t>(tokeniser.nextToken());
            colours.resize(numColorVertices, Vector3(1.0, 1.0, 1.0));
        }
        /* mesh material reference. this usually comes at the end of 
         * geomobjects after the mesh blocks. we must assume that the
         * new mesh was already created so all we can do here is assign
         * the material reference id (shader index) now. */
        else if (token == "*material_ref")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= materials.size()) throw parser::ParseException("MATERIAL_REF index out of bounds >= MATERIAL_COUNT");
        }
        /* model mesh vertex */
        else if (token == "*mesh_vertex")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= vertices.size()) throw parser::ParseException("MESH_VERTEX index out of bounds >= MESH_NUMVERTEX");

            auto& vertex = vertices[index].vertex;
            vertex.x() = string::convert<double>(tokeniser.nextToken());
            vertex.y() = string::convert<double>(tokeniser.nextToken());
            vertex.z() = string::convert<double>(tokeniser.nextToken());
        }
        /* model mesh vertex normal */
        else if (token == "*mesh_vertexnormal")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= vertices.size()) throw parser::ParseException("MESH_VERTEXNORMAL index out of bounds >= MESH_NUMVERTEX");

            auto& normal = vertices[index].normal;
            normal.x() = string::convert<double>(tokeniser.nextToken());
            normal.y() = string::convert<double>(tokeniser.nextToken());
            normal.z() = string::convert<double>(tokeniser.nextToken());
        }
        /* model mesh face */
        else if (token == "*mesh_face")
        {
            // *MESH_FACE    0:    A:    3 B:    1 C:    2 AB:    0 BC:    0 CA:    0	 *MESH_SMOOTHING 0	 *MESH_MTLID 0
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= faces.size()) throw parser::ParseException("MESH_FACE index out of bounds >= MESH_NUMFACES");

            tokeniser.assertNextToken(":");

            tokeniser.assertNextToken("A");
            tokeniser.assertNextToken(":");

            auto& face = faces[index];
            
            face.vertexIndices[0] = string::convert<std::size_t>(tokeniser.nextToken());

            tokeniser.assertNextToken("B");
            tokeniser.assertNextToken(":");

            face.vertexIndices[1] = string::convert<std::size_t>(tokeniser.nextToken());

            tokeniser.assertNextToken("C");
            tokeniser.assertNextToken(":");

            face.vertexIndices[2] = string::convert<std::size_t>(tokeniser.nextToken());

            tokeniser.skipTokens(13);

            if (face.vertexIndices[0] >= vertices.size()) throw parser::ParseException("MESH_FACE vertex index 0 out of bounds >= MESH_NUMFACES");
            if (face.vertexIndices[1] >= vertices.size()) throw parser::ParseException("MESH_FACE vertex index 1 out of bounds >= MESH_NUMFACES");
            if (face.vertexIndices[2] >= vertices.size()) throw parser::ParseException("MESH_FACE vertex index 2 out of bounds >= MESH_NUMFACES");
        }
        /* model texture vertex */
        else if (token == "*mesh_tvert")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= texcoords.size()) throw parser::ParseException("MESH_TVERT index out of bounds >= MESH_NUMTVERTEX");

            auto& texcoord = texcoords[index];
            texcoord.x() = string::convert<double>(tokeniser.nextToken());
            /* ydnar: invert t */
            texcoord.y() = 1.0 - string::convert<double>(tokeniser.nextToken());
            // ignore the third texcoord value
            tokeniser.nextToken();
        }
        /* ydnar: model mesh texture face */
        else if (token == "*mesh_tface")
        {
            // *MESH_TFACE 0    0   1   2
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= faces.size()) throw parser::ParseException("MESH_TFACE index out of bounds >= MESH_NUMFACES");

            auto& face = faces[index];

            face.texcoordIndices[0] = string::convert<std::size_t>(tokeniser.nextToken());
            face.texcoordIndices[1] = string::convert<std::size_t>(tokeniser.nextToken());
            face.texcoordIndices[2] = string::convert<std::size_t>(tokeniser.nextToken());

            if (face.texcoordIndices[0] >= texcoords.size()) throw parser::ParseException("MESH_TFACE texcoord index 0 out of bounds >= MESH_NUMTVERTEX");
            if (face.texcoordIndices[1] >= texcoords.size()) throw parser::ParseException("MESH_TFACE texcoord index 1 out of bounds >= MESH_NUMTVERTEX");
            if (face.texcoordIndices[2] >= texcoords.size()) throw parser::ParseException("MESH_TFACE texcoord index 2 out of bounds >= MESH_NUMTVERTEX");
        }
        /* model color vertex */
        else if (token == "*mesh_vertcol")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= colours.size()) throw parser::ParseException("MESH_VERTCOL index out of bounds >= MESH_NUMCVERTEX");

            auto& colour = colours[index];
            colour.x() = string::convert<double>(tokeniser.nextToken());
            colour.y() = string::convert<double>(tokeniser.nextToken());
            colour.z() = string::convert<double>(tokeniser.nextToken());
            /* leave alpha alone since we don't get any data from the ASE format */
            colour.w() = 1.0;
        }
        /* model color face */
        else if (token == "*mesh_cface")
        {
            // *MESH_CFACE 0    0   1   2
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= faces.size()) throw parser::ParseException("MESH_CFACE index out of bounds >= MESH_NUMFACES");

            auto& face = faces[index];

            face.colourIndices[0] = string::convert<std::size_t>(tokeniser.nextToken());
            face.colourIndices[1] = string::convert<std::size_t>(tokeniser.nextToken());
            face.colourIndices[2] = string::convert<std::size_t>(tokeniser.nextToken());

            if (face.colourIndices[0] >= colours.size()) throw parser::ParseException("MESH_CFACE colour index 0 out of bounds >= MESH_NUMCVERTEX");
            if (face.colourIndices[1] >= colours.size()) throw parser::ParseException("MESH_CFACE colour index 1 out of bounds >= MESH_NUMCVERTEX");
            if (face.colourIndices[2] >= colours.size()) throw parser::ParseException("MESH_CFACE colour index 2 out of bounds >= MESH_NUMCVERTEX");
        }
        else if (token == "*material_count")
        {
            auto numMaterials = string::convert<std::size_t>(tokeniser.nextToken());
            materials.resize(numMaterials);
        }
        /* model material */
        else if (token == "*material")
        {
            auto index = string::convert<std::size_t>(tokeniser.nextToken());

            if (index >= materials.size()) throw parser::ParseException("MATERIAL index out of bounds >= MATERIAL_COUNT");

            tokeniser.assertNextToken("{");
            int level = 1;

            /* parse material block */
            while (tokeniser.hasMoreTokens())
            {
                token = tokeniser.nextToken();
                string::to_lower(token);

                if (token.empty()) continue;

                /* handle levels */
                if (token[0] == '{') level++;
                if (token[0] == '}') level--;
                
                if (level == 0) break;

                /* parse material name */
                if (token == "*material_name")
                {
                    materials[index].materialName = string::trim_copy(tokeniser.nextToken(), "\"");
                }
                /* material diffuse map */
                else if (token == "*map_diffuse")
                {
                    int sublevel = 0;

                    /* parse material block */
                    while (tokeniser.hasMoreTokens())
                    {
                        token = tokeniser.nextToken();
                        string::to_lower(token);

                        if (token.empty()) continue;

                        /* handle levels */
                        if (token[0] == '{') sublevel++;
                        if (token[0] == '}') sublevel--;

                        if (sublevel == 0) break;

                        /* parse diffuse map bitmap */
                        if (token == "*bitmap")
                        {
                            materials[index].diffuseBitmap = string::trim_copy(tokeniser.nextToken(), "\"");
                        }
                        else if (token ==  "*uvw_u_offset")
                        {
                            // Negate the u offset value
                            materials[index].uOffset = -string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_v_offset")
                        {
                            materials[index].vOffset = string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_u_tiling")
                        {
                            materials[index].uTiling = string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_v_tiling")
                        {
                            materials[index].vTiling = string::convert<float>(tokeniser.nextToken());
                        }
                        else if (token == "*uvw_angle")
                        {
                            materials[index].angle = string::convert<float>(tokeniser.nextToken());
                        }
                    }
                } /* end map_diffuse block */
            } /* end material block */
        }
    }

    /* ydnar: finish existing surface */
    _ase_submit_triangles(*model, materials, vertices, texcoords, colours, faces);

    return model;
}

} // namespace
