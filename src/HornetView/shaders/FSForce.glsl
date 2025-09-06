#version 330 core

in vec4 vColor;
in vec3 vN;
in vec3 vPosV;

uniform int uLit;
uniform int uUseVertexColor;
uniform int uAmbOn, uDifOn, uSpeOn;
uniform vec3 uAmbColor, uDifColor, uSpeColor;
uniform float uAmbI, uDifI, uSpeI, uShininess;
uniform float uAlpha;

out vec4 FragColor;

void main()
{
    vec3 base = vColor.rgb;
    float a = vColor.a * uAlpha;

    if (uLit == 0)
    {
        FragColor = vec4(base, a);
        return;
    }

    vec3 N = normalize(vN);
    vec3 L = normalize(-vPosV);  // camera at origin in view space
    vec3 V = normalize(-vPosV);

    float ndl = max(dot(N,L), 0.0);
    vec3 R = reflect(-L, N);
    float rv = max(dot(R,V), 0.0);

    vec3 amb = (uAmbOn != 0 ? uAmbColor * uAmbI : vec3(0.0));
    vec3 dif = (uDifOn != 0 ? uDifColor * uDifI * ndl : vec3(0.0));
    vec3 spe = (uSpeOn != 0 ? uSpeColor * uSpeI * pow(rv, max(1.0, uShininess)) : vec3(0.0));

    vec3 lit = (amb + dif) * base + spe;
    FragColor = vec4(lit, a);
}