#version 120

uniform sampler2D	u_Diffusemap;
uniform sampler2D	u_Bumpmap;
uniform sampler2D	u_Specularmap;
uniform sampler2D	u_attenuationmap_xy;
uniform sampler2D	u_attenuationmap_z;
uniform vec3		u_view_origin;
uniform vec3		u_light_origin;
uniform vec3		u_light_color;
uniform float		u_light_scale;
uniform vec4		u_ColourModulation;
uniform vec4		u_ColourAddition;

// Activate ambient light mode (brightness unaffected by direction)
uniform bool uAmbientLight;

// Texture coords as calculated by the vertex program
varying vec2 var_TexDiffuse;
varying vec2 var_TexBump;
varying vec2 var_TexSpecular;

varying vec3		var_vertex;
varying vec4		var_tex_atten_xy_z;
varying mat3		var_mat_os2ts;
varying vec4		var_Colour; // colour to be multiplied on the final fragment

void	main()
{
	// compute view direction in tangent space
	vec3 V = normalize(var_mat_os2ts * (u_view_origin - var_vertex));

	// compute light direction in tangent space
	vec3 L = normalize(var_mat_os2ts * (u_light_origin - var_vertex));

	// compute half angle in tangent space
	vec3 H = normalize(L + V);

	// compute normal in tangent space from bumpmap
    vec2 normalRG = texture2D(u_Bumpmap, var_TexBump).rg;
    float normalB = sqrt(1.0 - pow(normalRG.r, 2.0) - pow(normalRG.g, 2.0));
	vec3 N = 2.0 * (vec3(normalRG, normalB) - 0.5);
	N = normalize(N);

	// compute the diffuse term
	vec4 diffuse = texture2D(u_Diffusemap, var_TexDiffuse);
    float lightBrightness = uAmbientLight ? 1.0 : clamp(dot(N, L), 0.0, 1.0);
	diffuse.rgb *= u_light_color * u_light_scale * lightBrightness;

	// compute the specular term
    float specIntensity = clamp(dot(N, H), 0.0, 1.0);
    specIntensity = pow(specIntensity, 32.0);
	vec3 specular = texture2D(u_Specularmap, var_TexSpecular).rgb
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
    gl_FragColor = diffuse * var_Colour;
    if (!uAmbientLight)
        gl_FragColor.rgb += specular;
	gl_FragColor.rgb *= attenuation_xy;
	gl_FragColor.rgb *= attenuation_z;
}

