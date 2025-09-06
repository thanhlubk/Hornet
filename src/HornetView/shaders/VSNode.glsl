#version 330 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aCol;

uniform mat4 uMVP;
uniform float uPointSize;
uniform int  uUseVertexColor;

out vec4 vCol;

void main()
{
    gl_Position = uMVP * vec4(aPos,1.0);
    gl_PointSize = uPointSize;
    vCol = (uUseVertexColor==1) ? aCol : vec4(1,1,1,1);
}