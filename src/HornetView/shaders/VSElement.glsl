#version 330 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aCol;
layout(location=2) in vec3 aNrm;

uniform mat4 uMVP, uView;
out vec3 vPos;
out vec3 vNrm;
out vec4 vCol;

void main()
{
    vPos = (uView * vec4(aPos, 1.0)).xyz;
    vNrm = mat3(uView) * aNrm;
    vCol = aCol;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
