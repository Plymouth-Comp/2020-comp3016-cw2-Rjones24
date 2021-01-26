#version 450 core

out vec4 fColor;
in vec2 TexCoord;

uniform sampler2D texture_diffuse1;

void main()
{
	fColor = texture(texture_diffuse1, TexCoord);
	
}

