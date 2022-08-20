#version 130

uniform sampler2D u_LightProjectionTexture; // light projection texture
uniform sampler2D u_LightFallOffTexture;    // light falloff texture
uniform vec4 u_BlendColour;                 // light colour

varying vec4 var_tex_atten_xy_z;

void main()
{
    // Light texture lookups
    vec4 attenuation_xy = vec4(0,0,0,0);
    
    // Discard s/t coords outside the light texture (clamp)
    if (var_tex_atten_xy_z.x < 0 || var_tex_atten_xy_z.x > 1 || 
        var_tex_atten_xy_z.y < 0 || var_tex_atten_xy_z.y > 1)
    {
        discard;
    }

    if (var_tex_atten_xy_z.w > 0.0)
    {
        attenuation_xy	= texture2DProj(u_LightProjectionTexture, var_tex_atten_xy_z.xyw);
    }

    vec4 attenuation_z	= texture2D(u_LightFallOffTexture, vec2(var_tex_atten_xy_z.z, 0.5));
    
    gl_FragColor = u_BlendColour * attenuation_xy * attenuation_z;
}

