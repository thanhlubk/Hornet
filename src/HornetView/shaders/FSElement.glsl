#version 330 core

in vec3 vPos; in vec3 vNrm;
out vec4 FragColor;

uniform vec3 uColor;
uniform int  uLit;

// Lighting uniforms provided by HViewLighting::applyTo(...)
uniform int  uAmbOn, uDifOn, uSpeOn;
uniform vec3 uAmbColor, uDifColor, uSpeColor;
uniform float uAmbI, uDifI, uSpeI, uShininess;

void main()
{
    vec3 col = uColor;
    if (uLit==1)
    {
        vec3 N = normalize(vNrm);
        vec3 V = normalize(-vPos);
        vec3 L = V; // headlight
        float ndl = max(dot(N,L), 0.0);

        float amb = (uAmbOn==1?uAmbI:0.0);
        float dif = (uDifOn==1?uDifI*ndl:0.0);
        float spe = 0.0;

        if (uSpeOn==1 && ndl>0.0)
        {
            vec3 R = reflect(-L,N);
            spe = uSpeI * pow(max(dot(R,V),0.0), uShininess);
        }
        col = col * (uAmbColor*amb + uDifColor*dif) + uSpeColor*spe;
    }
    FragColor = vec4(col,1.0);
}