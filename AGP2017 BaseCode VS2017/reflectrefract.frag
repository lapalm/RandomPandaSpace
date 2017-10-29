#version 330

// Some drivers require the following
precision highp float;

struct lightStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

struct materialStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform lightStruct light;
uniform materialStruct material;

uniform samplerCube texture1_skycube;

uniform float attConst;
uniform float attLinear;
uniform float attQuadratic;

in vec3 ex_WorldNorm;
in vec3 ex_WorldView;

in vec3 ex_N;
in vec3 ex_V;
in vec3 ex_L;

in float ex_D;
layout(location = 0) out vec4 out_Color;
 
void main(void) 
{    
	// Ambient intensity
	vec4 ambientI = light.ambient * material.ambient;

	// Diffuse intensity
	vec4 diffuseI = light.diffuse * material.diffuse;
	diffuseI = diffuseI * max(dot(normalize(ex_N),normalize(ex_L)),0);
	diffuseI.a = 1.0;

	// Specular intensity
	// Calculate R - reflection of light
	vec3 R = normalize(reflect(normalize(-ex_L),normalize(ex_N)));
	vec4 specularI = light.specular * material.specular;
	specularI = specularI * pow(max(dot(R,ex_V),0), material.shininess);
	specularI.a = 1.0;

	// http://sunandblackcat.com/tipFullView.php?l=eng&topicid=16&topic=Environmental-Mapping
	float IOR = 1.517f; // glass = 1.517
	float offset = 0.05f; 
	vec3 refractTexCoord_Tr = refract(-ex_WorldView, normalize(ex_WorldNorm), IOR + offset);
	vec3 refractTexCoord_Tg = refract(-ex_WorldView, normalize(ex_WorldNorm), IOR);
	vec3 refractTexCoord_Tb = refract(-ex_WorldView, normalize(ex_WorldNorm), IOR - offset);

	vec3 refractedColor;
    refractedColor.r = texture(texture1_skycube, refractTexCoord_Tr).r;
    refractedColor.g = texture(texture1_skycube, refractTexCoord_Tg).g;
    refractedColor.b = texture(texture1_skycube, refractTexCoord_Tb).b;

	// reflection (env mapping)
	vec3 reflectTexCoord = reflect(-ex_WorldView, normalize(ex_WorldNorm));
	vec3 reflectedColor = texture(texture1_skycube, reflectTexCoord).rgb;

	// attenuation
	float attenuation=1.0f/(attConst + attLinear * ex_D + attQuadratic * ex_D*ex_D);
	vec4 tmp_Color = (diffuseI + specularI);
	//Attenuation does not affect transparency
	vec4 litColour = vec4(tmp_Color.rgb *attenuation, tmp_Color.a);
	vec4 amb=min(ambientI,vec4(1.0f));

	//////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    float frenel = clamp(dot(ex_WorldView, normalize(ex_WorldNorm))/-1.0f,0.0f,1.0f);
    if(length(refractTexCoord_Tb)<0.5f){ // total internal reflection
       frenel = 0.0f;
    }
    frenel = pow( frenel, 1.55f);

    vec3 col = reflectedColor * frenel + refractedColor * (1.0f - frenel);
	out_Color = litColour*amb + vec4(col, 1.0);
}