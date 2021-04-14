#version 120

uniform sampler2D   u_diffuse;
uniform float       u_alpha_test;

varying vec2        var_tex_diffuse;

void main()
{
    if (u_alpha_test < 0)
    {
        gl_FragColor.a = 1.0;
        gl_FragColor.rgb = vec3(1.0, 1.0, 0.0);
    }
    else
    {
        vec4 tex = texture2D(u_diffuse, var_tex_diffuse);

        if (tex.a <= u_alpha_test)
        {
            discard;
        }

        gl_FragColor = tex;
    }
}
