#version 330

uniform mat4 modelview;
uniform mat4 projection;
uniform vec4 lightPosition;

uniform mat4 modelMatrix;
uniform vec3 cameraPos;


in  vec3 in_Position;
in  vec3 in_Normal;

out vec3 ex_N;
out vec3 ex_V;
out vec3 ex_L;
out vec3 ex_WorldNorm;
out vec3 ex_WorldView;


out float ex_D;

// multiply each vertex position by the MVP matrix
// and find V, L, N vectors for the fragment shader

void main(void) {

	// vertex into eye coordinates
	vec4 vertexPosition = modelview * vec4(in_Position,1.0);
	ex_D = distance(vertexPosition,lightPosition);

	// Find V - in eye coordinates, eye is at (0,0,0)
	ex_V = normalize(-vertexPosition).xyz;

	// surface normal in eye coordinates
	// taking the rotation part of the modelview matrix to generate the normal matrix
	// (if scaling is includes, should use transpose inverse modelview matrix!)
	// this is somewhat wasteful in compute time and should really be part of the cpu program,
	// giving an additional uniform input

	mat3 normalmatrix = transpose(inverse(mat3(modelview)));
	ex_N = normalize(normalmatrix * in_Normal);

	// check if this is a directional light
	if(lightPosition.w == 0.0) {
		// it is a directional light.
		// get the direction by converting to vec3 (ignore w) and negate it
		ex_L = -lightPosition.xyz;
	} else 
	{
		// L - to light source from vertex
		ex_L = normalize(lightPosition.xyz - vertexPosition.xyz);
	}

	vec3 worldPos = (modelMatrix * vec4(in_Position,1.0)).xyz;
	mat3 normalworldmatrix=transpose(inverse(mat3(modelMatrix))); //This is expensive, better to compute it on the CPU and set it as an uniform as suggested in lab 2

	ex_WorldNorm = normalworldmatrix * in_Normal;
	ex_WorldView = cameraPos - worldPos;

    gl_Position = projection * vertexPosition;
}