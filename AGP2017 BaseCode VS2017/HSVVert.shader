#version 330 core
layout(location = 0) in vec3 in_Position; // VERTEX in rt3d.h
layout(location = 2) in vec3 in_Normal; // NORMAL in rt3d.h
layout(location = 3) in vec2 in_UV; // TEXCOORD in rt3d.h

uniform mat4 modelview;
uniform mat4 projection;
uniform vec3 cameraPos;
uniform mat4 modelMatrix; 
uniform mat3 normalMatrix;
uniform vec4 lightPosition; // this


out vec3 FragPos;
out vec3 normal;

out vec3 ex_V; // This
out vec3 ex_L; // This

out vec2 ex_UV;
out vec3 ex_WorldNorm; 
out vec3 ex_WorldView; 

void main()
{
	ex_UV = in_UV;

	
	FragPos = vec3(modelview * vec4(in_Position, 1.0f));
	
	vec3 worldPos = (modelMatrix * vec4(in_Position, 1.0)).xyz;

	// Find V - in eye coordinates, eye is at (0,0,0)
	ex_V = normalize(-FragPos).xyz;


	// the transpose of the inverse of the upper-left corner of the model matrix : 
	// see http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/ // <- try to do in CPU instead. Less expensive
	mat3 normalmatrix = transpose(inverse(mat3(modelview)));
	

	normal = normalize(normalmatrix * in_Normal); // ex_N

	// L - to light source from vertex
	ex_L = normalize(lightPosition.xyz - FragPos.xyz);

	// For reflection/Envmapping
	mat3 normalworldmatrix = transpose(inverse(mat3(modelMatrix)));
	ex_WorldNorm = normalworldmatrix * in_Normal;
	ex_WorldView = cameraPos - worldPos;

	gl_Position = projection * modelview * vec4(in_Position, 1.0f);

}