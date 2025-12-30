#version 460

layout(location = 0) in float Speed;
layout(location = 0) out vec4 outColor;

void main()
{
    // tweak speed constant to get different gradients
    float t = clamp(Speed * Speed * 2.74, 0.0, 1.0);

    vec3 slowColor = vec3(0.0, 0.4, 0.3);
    vec3 fastColor = vec3(0.9, 0.8, 0.15);

    vec3 color = mix(slowColor, fastColor, t);
    outColor = vec4(color, 1.0);
}
