#version 330 core
layout(location = 0) in vec3 position; // VERTEX in rt3d.h
layout(location = 2) in vec3 normal; // NORMAL in rt3d.h
layout(location = 3) in vec2 in_UV; // TEXCOORD in rt3d.h

out vec3 FragPos;
out vec3 Normal;
out vec2 ex_UV;

uniform mat4 modelview;
//uniform mat4 projection;
uniform mat3 normalMatrix;

void main()
{
	FragPos = vec3(modelview * vec4(position, 1.0f));
	gl_Position = projection * FragPos;
	
	// the transpose of the inverse of the upper-left corner of the model matrix : 
	// see http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/ // <- try to do in CPU instead. Less expensive
	mat3 normalworldmatrix = transpose(inverse(mat3(modelMatrix)));

	Normal = normalize(normalMatrix * normal);
	ex_UV = in_UV;

}