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

uniform sampler2D	u_diffusemap;
uniform sampler2D	u_bumpmap;
uniform sampler2D	u_specularmap;
uniform sampler2D	u_attenuationmap_xy;
uniform sampler2D	u_attenuationmap_z;
uniform vec3		u_view_origin;
uniform vec3		u_light_origin;
uniform vec3		u_light_color;
uniform float		u_light_scale;

varying vec3		var_vertex;
varying vec4		var_tex_diffuse_bump;
varying vec2		var_tex_specular;
varying vec4		var_tex_atten_xy_z;
varying mat3		var_mat_os2ts;

void	main()
{	
	// compute view direction in tangent space
	vec3 V = normalize(var_mat_os2ts * (u_view_origin - var_vertex));
	
	// compute light direction in tangent space
	vec3 L = normalize(var_mat_os2ts * (u_light_origin - var_vertex));
	
	// compute half angle in tangent space
	vec3 H = normalize(L + V);
	
	// compute normal in tangent space from bumpmap
	vec3 N = 2.0 * (texture2D(u_bumpmap, var_tex_diffuse_bump.pq).xyz - 0.5);
	N = normalize(N);
	
	// compute the diffuse term
	vec4 diffuse = texture2D(u_diffusemap, var_tex_diffuse_bump.st);
	diffuse.rgb *= u_light_color * u_light_scale * clamp(dot(N, L), 0.0, 1.0);
	
	// compute the specular term
    float specIntensity = clamp(dot(N, H), 0.0, 1.0);
    specIntensity = pow(specIntensity, 32.0);
	vec3 specular = texture2D(u_specularmap, var_tex_specular.xy).rgb 
                    * u_light_color 
                    * specIntensity;
	
	// compute attenuation
    vec3 attenuation_xy = vec3(0.0, 0.0, 0.0);
    if (var_tex_atten_xy_z.w > 0.0)
        attenuation_xy	= texture2DProj(
            u_attenuationmap_xy,
            var_tex_atten_xy_z.xyw
        ).rgb;

	vec3 attenuation_z	= texture2D(
        u_attenuationmap_z, vec2(var_tex_atten_xy_z.z, 0.5)
    ).rgb;
					
	// compute final color
    gl_FragColor = diffuse * gl_Color;
    gl_FragColor.rgb += specular;
	gl_FragColor.rgb *= attenuation_xy;
	gl_FragColor.rgb *= attenuation_z;
}

