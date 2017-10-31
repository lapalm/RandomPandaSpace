#version 330 core

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	sampler2D emission;
	float shininess;
};

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

in vec3 FragPos; // VertexPosition
in vec3 ex_Normal; // WorldNormals <-- Normalized in vertex shader
in vec2 ex_UV; // ex_TexCoords

layout(location = 0) out vec4 out_color; //The result to be outputted

// Put uniforms here:
uniform vec3 viewPos; // Currently replaced by ex_L
uniform PointLight pointLight;
uniform Material material;

uniform float attConst;
uniform float attLinear;
uniform float attQuadratic;

uniform float hueShift;
uniform float satBoost;

// Function prototypes
vec3 calcPointLight(PointLight light, vec3 normal, vec3 FragPos, vec3 viewDir);
vec3 rgb2hsv(vec3 rgbColor);
vec3 hsv2rgb(vec3 hsvColor);

void main() {

	// Properties
	vec3 normal = normalize(ex_Normal);
	vec3 viewDir = normalize(viewPos - FragPos);

	// Phase 1: Point lights
	vec3 result = calcPointLight(pointLight, normal, FragPos, viewDir);

	// phase 2: emission + hsv

	// sample the image
	vec3 rgb = vec3(texture(material.emission, ex_UV));
	
	// look up the corresponding hsv value
	vec3 hsv = rgb2hsv(rgb);

	// manipulate hue and saturation
	hsv.x = fract(hsv.x + hueShift);
	hsv.y *= satBoost;

	// look up the corresponding rgb value
	vec3 finalemission = vec3(hsv2rgb(hsv));

	vec3 emission = finalemission;

	result += emission;

	//phase 3: gamma correct
	float gammaValue = 2.2;

	if (ex_UV.x < 0.5) {
		result += pow(result, vec3(1/ gammaValue)); 
	}

	//phase 4: Output Result

	// Each light type adds it's contribution to the resulting output color until all light sources are processed.
	// The resulting color contains the color impact of all the light sources in the scene combined. 
	out_color = vec4(result, 1.0);
	out_color.a = 1.0;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 FragPos, vec3 viewDir) {

	vec3 lightDir = normalize(light.position.xyz - FragPos.xyz); // Normalize the resulting direction vector

	// Diffuse
	float diff = max(dot(normal, lightDir), 0.0); // Use Max to avoid dot product going negative when vector is greater than 90 degrees.

	// Specular 
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

	// Attenuation
	float distance = length(light.position - FragPos);

	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	// Combine results
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, ex_UV));
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, ex_UV));
	vec3 specular = light.specular * spec * vec3(texture(material.specular, ex_UV));

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	return (ambient + diffuse + specular);
}

/*Reference: From Sam from Lolengine.net*/
vec3 rgb2hsv(vec3 rgbColor)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(rgbColor.bg, K.wz), vec4(rgbColor.gb, K.xy), step(rgbColor.b, rgbColor.g));
	vec4 q = mix(vec4(p.xyw, rgbColor.r), vec4(rgbColor.r, p.yzx), step(p.x, rgbColor.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

/*Reference: From Sam from Lolengine.net*/
vec3 hsv2rgb(vec3 hsvColor)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(hsvColor.xxx + K.xyz) * 6.0 - K.www);
	return hsvColor.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), hsvColor.y);
}

