#version 130

uniform sampler2D u_LightProjectionTexture; // light projection texture
uniform sampler2D u_LightFallOffTexture;    // light falloff texture
uniform vec4 u_BlendColour;                 // light colour

in vec4 var_tex_atten_xy_z;

out vec4 FragColor;

void main()
{
    // Light texture lookups
    vec3 attenuation_xy = vec3(0,0,0);

    if (var_tex_atten_xy_z.w > 0.0)
    {
        attenuation_xy	= texture2DProj(u_LightProjectionTexture, var_tex_atten_xy_z.xyw).rgb;
    }

    vec3 attenuation_z	= texture2D(u_LightFallOffTexture, vec2(var_tex_atten_xy_z.z, 0.5)).rgb;

    FragColor.rgb = u_BlendColour.rgb * attenuation_xy * attenuation_z;
}

