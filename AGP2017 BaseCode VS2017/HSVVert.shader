#version 330 core
layout(location = 0) in vec3 in_Position; // VERTEX in rt3d.h
layout(location = 2) in vec3 in_Normal; // NORMAL in rt3d.h
layout(location = 3) in vec2 in_UV; // TEXCOORD in rt3d.h

uniform mat4 modelview;
uniform mat4 projection;
uniform mat4 modelMatrix; 

out vec3 FragPos;
out vec3 ex_Normal;
out vec2 ex_UV;

void main()
{
	ex_UV = in_UV;

	// the transpose of the inverse of the upper-left corner of the model matrix : 
	// see http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/ // <- try to do in CPU instead. Less expensive.
	ex_Normal = mat3(transpose(inverse(modelMatrix))) * in_Normal; // World Normal

	FragPos = vec3(modelMatrix * vec4(in_Position, 1.0f));
	vec3 vPos = vec3(modelview * vec4(in_Position, 1.0f));
	gl_Position = projection * vec4(vPos, 1.0f);

}