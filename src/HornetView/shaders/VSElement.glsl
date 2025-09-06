#version 330 core

layout(location=0) in vec3 aPos;
layout(location=2) in vec3 aNrm;

uniform mat4 uMVP, uView;
out vec3 vPos; out vec3 vNrm;

void main()
{
    vPos = (uView * vec4(aPos,1)).xyz;
    vNrm = mat3(uView) * aNrm;
    gl_Position = uMVP * vec4(aPos,1);
}