#version 330 core

layout (location=0) in vec3 aPos;
layout (location=1) in vec4 aColor;
layout (location=2) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uView;

out vec4 vColor;
out vec3 vN;
out vec3 vPosV;

void main()
{
    gl_Position = uMVP * vec4(aPos,1.0);
    vColor = aColor;
    vN = mat3(uView) * aNormal;          // to view space
    vPosV = (uView * vec4(aPos,1.0)).xyz;   // view-space position
}