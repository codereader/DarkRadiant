#version 120

uniform sampler2D   u_Diffuse;
uniform float       u_AlphaTest;
uniform mat4        u_ObjectTransform;

// The final diffuse texture coordinate at this vertex, calculated in the vertex shader
varying vec2 var_TexDiffuse;

void main()
{
    if (u_AlphaTest < 0)
    {
        gl_FragColor.a = 1.0;
        gl_FragColor.rgb = vec3(1.0, 1.0, 0.0);
    }
    else
    {
        vec4 tex = texture2D(u_Diffuse, var_TexDiffuse);

        if (tex.a <= u_AlphaTest)
        {
            discard;
        }

        gl_FragColor = tex;
    }
}
