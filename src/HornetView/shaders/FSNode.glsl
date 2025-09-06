#version 330 core

in vec4 vCol;
out vec4 FragColor;

uniform vec3 uColor;
uniform int  uUseVertexColor;

void main()
{
    vec4 c = (uUseVertexColor==1)? vCol : vec4(uColor,1.0);
    FragColor = c;
}