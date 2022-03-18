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

varying vec3 var_vertex; // in world space
varying vec4 var_tex_atten_xy_z;
varying mat3 var_mat_os2ts;
varying vec4 var_Colour; // colour to be multiplied on the final fragment

void main()
{
    // Ported from TDM interaction.common.fs.glsl

    vec4 fresnelParms = vec4(1.0, .23, .5, 1.0);
    vec4 fresnelParms2 = vec4(.2, .023, 120.0, 4.0);
    vec4 lightParms = vec4(.7, 1.8, 10.0, 30.0);

    vec4 diffuse = texture2D(u_Diffusemap, var_TexDiffuse);
    vec3 specular = texture2D(u_Specularmap, var_TexSpecular).rgb;

    // compute view direction in tangent space
	vec3 localV = normalize(var_mat_os2ts * (u_view_origin - var_vertex));
	
    // compute light direction in tangent space
	vec3 localL = normalize(var_mat_os2ts * (u_light_origin - var_vertex));

    vec4 bumpTexel = texture2D(u_Bumpmap, var_TexBump) * 2. - 1.;
	vec3 RawN = normalize( bumpTexel.xyz ); 
	vec3 N = var_mat_os2ts * RawN;

	//must be done in tangent space, otherwise smoothing will suffer (see #4958)
	float NdotL = clamp(dot(RawN, localL), 0.0, 1.0);
	float NdotV = clamp(dot(RawN, localV), 0.0, 1.0);
	float NdotH = clamp(dot(RawN, normalize(localV + localL)), 0.0, 1.0);

	// fresnel part
	float fresnelTerm = pow(1.0 - NdotV, fresnelParms2.w);
	float rimLight = fresnelTerm * clamp(NdotL - 0.3, 0.0, fresnelParms.z) * lightParms.y;
	float specularPower = mix(lightParms.z, lightParms.w, specular.z);
	float specularCoeff = pow(NdotH, specularPower) * fresnelParms2.z;
	float fresnelCoeff = fresnelTerm * fresnelParms.y + fresnelParms2.y;

	vec3 specularColor = specularCoeff * fresnelCoeff * specular * (diffuse.rgb * 0.25 + vec3(0.75));
	float R2f = clamp(localL.z * 4.0, 0.0, 1.0);

	float NdotL_adjusted = NdotL;
	float light = rimLight * R2f + NdotL_adjusted;

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

	vec3 totalColor = (specularColor * u_light_color * R2f + diffuse.rgb) * light * (u_light_color * u_light_scale) * attenuation_xy * attenuation_z * var_Colour.rgb;

	gl_FragColor.rgb = totalColor;
	gl_FragColor.a = diffuse.a;
}

