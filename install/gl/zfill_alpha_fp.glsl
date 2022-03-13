#version 120

uniform sampler2D   u_diffuse;
uniform float       u_alphaTest;
uniform mat4        u_objectTransform;

varying vec2 var_TexDiffuse;

void main()
{
    if (u_alphaTest < 0)
    {
        gl_FragColor.a = 1.0;
        gl_FragColor.rgb = vec3(1.0, 1.0, 0.0);
    }
    else
    {
        vec4 tex = texture2D(u_diffuse, var_TexDiffuse);

        if (tex.a <= u_alphaTest)
        {
            discard;
        }

        gl_FragColor = tex;
    }
}
