
uniform sampler2D	u_diffusemap;
uniform sampler2D	u_bumpmap;
uniform sampler2D	u_specularmap;
uniform sampler2D	u_attenuationmap_xy;
uniform sampler2D	u_attenuationmap_z;
uniform vec3		u_view_origin;
uniform vec3		u_light_origin;
uniform vec3		u_light_color;
uniform float		u_light_scale;

// Invert vertex colour
uniform bool uInvertVCol;

// Activate ambient light mode (brightness unaffected by direction)
uniform bool uAmbientLight;

varying vec3		var_vertex;
varying vec4		var_tex_diffuse_bump;
varying vec2		var_tex_specular;
varying vec4		var_tex_atten_xy_z;
varying mat3		var_mat_os2ts;

void	main()
{
	gl_FragColor.rgb = vec3(1, 0, 1);
}

