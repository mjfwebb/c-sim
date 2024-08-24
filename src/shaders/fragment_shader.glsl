#version 330 core
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D textureSampler;

void main()
{
    vec4 texColor = texture(textureSampler, TexCoord);
    float gray = (texColor.r + texColor.g + texColor.b) / 3.0;
    FragColor = vec4(vec3(gray), texColor.a);  // Convert to grayscale
}
