/// ============================================================================
/*
Copyright (C) 2004 Robert Beckebans <trebor_7@users.sourceforge.net>
Please see the file "CONTRIBUTORS" for a list of contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
/// ============================================================================
#version 120

in vec4 attr_Position;  // bound to attribute 0 in source, in object space
in vec4 attr_TexCoord;  // bound to attribute 8 in source
in vec4 attr_Tangent;   // bound to attribute 9 in source
in vec4 attr_Bitangent; // bound to attribute 10 in source
in vec4 attr_Normal;    // bound to attribute 11 in source
in vec4 attr_Colour;    // bound to attribute 12 in source

uniform vec4 u_ColourModulation;    // vertex colour weight
uniform vec4 u_ColourAddition;      // constant additive vertex colour value
uniform mat4 u_ModelViewProjection; // combined modelview and projection matrix
uniform mat4 u_ObjectTransform;     // object to world

// Texture Matrices (the two top rows of each)
uniform vec4 u_DiffuseTextureMatrix[2];

// Calculated texture coords
varying vec2 var_TexDiffuse;

varying vec3		var_vertex;
varying vec4		var_tex_diffuse_bump;
varying vec2		var_tex_specular;
varying vec4		var_tex_atten_xy_z;
varying mat3		var_mat_os2ts;
varying vec4		var_Colour; // colour to be multiplied on the final fragment

void main()
{
    vec4 worldVertex = u_ObjectTransform * attr_Position;

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjection * worldVertex;

	// assign position in world space
	var_vertex = worldVertex.xyz;

    // Apply the texture matrix to get the texture coords for this vertex
    var_TexDiffuse.x = dot(u_DiffuseTextureMatrix[0], attr_TexCoord);
    var_TexDiffuse.y = dot(u_DiffuseTextureMatrix[1], attr_TexCoord);

	// transform texcoords into diffusemap texture space
	var_tex_diffuse_bump.st = (gl_TextureMatrix[0] * attr_TexCoord).st;

	// transform texcoords into bumpmap texture space
	var_tex_diffuse_bump.pq = (gl_TextureMatrix[1] * attr_TexCoord).st;

	// transform texcoords into specularmap texture space
	var_tex_specular = (gl_TextureMatrix[2] * attr_TexCoord).st;

	// calc light xy,z attenuation in light space
	var_tex_atten_xy_z = gl_TextureMatrix[3] * attr_Position;

	// construct object-space-to-tangent-space 3x3 matrix
	var_mat_os2ts = mat3(
         attr_Tangent.x, attr_Bitangent.x, attr_Normal.x,
         attr_Tangent.y, attr_Bitangent.y, attr_Normal.y,
         attr_Tangent.z, attr_Bitangent.z, attr_Normal.z
    );

    // Vertex colour factor
    var_Colour = (attr_Colour * u_ColourModulation + u_ColourAddition);
}

