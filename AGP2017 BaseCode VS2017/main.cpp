// MD2 animation renderer
// This demo will load and render an animated MD2 model, an OBJ model and a skybox
// Most of the OpenGL code for dealing with buffer objects, etc has been moved to a 
// utility library, to make creation and display of mesh objects as simple as possible

// Windows specific: Uncomment the following line to open a console window for debug output
#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

#include "rt3d.h"
#include "rt3dObjLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#include "md2model.h"

#include <SDL_ttf.h>

using namespace std;

#define DEG_TO_RADIAN 0.017453293

// Globals
// Real programs don't use globals :-D

GLuint meshIndexCount = 0;
GLuint toonIndexCount = 0;
GLuint statueIndexCount = 0;
GLuint groundIndexCount = 0;
GLuint md2VertCount = 0;
GLuint meshObjects[5];

GLuint textureProgram;
GLuint shaderProgram;
GLuint skyboxProgram;
GLuint toonShaderProgram;
GLuint envmapProgram;
GLuint reflectrefractShaderProgram;
GLuint HSVShaderProgram;


//////////////////
/// FBO example - globals
//////////////////
GLuint fboID;
GLuint depthBufID;
GLuint reflectionTex;
GLuint screenWidth = 800;
GLuint screenHeight = 600;
GLuint reflectionWidth = 512;
GLuint reflectionHeight = 512;
static const GLenum fboAttachments[] = { GL_COLOR_ATTACHMENT0 };
static const GLenum frameBuff[] = { GL_BACK_LEFT };
glm::vec3 mirror_pos(-8.0f, 1.5f, -5.0f);
GLfloat mirrorAngle = 0.0f;

GLfloat r = 0.0f;

glm::vec3 eye(-5.0f, 4.0f, 9.0f); // left, up, forward
glm::vec3 at(0.0f, 1.0f, -1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);

stack<glm::mat4> mvStack;

// TEXTURE STUFF
GLuint textures[7];
GLuint skybox[5];
GLuint labels[5];

rt3d::lightStruct light0 = {
	{0.4f, 0.4f, 0.4f, 1.0f}, // ambient
	{1.0f, 1.0f, 1.0f, 1.0f}, // diffuse
	{1.0f, 1.0f, 1.0f, 1.0f}, // specular
	{-5.0f, 2.0f, 2.0f, 1.0f}  // position
};

glm::vec4 lightPos(-5.0f, 2.0f, 2.0f, 1.0f); //light position
float hueShift = 0.0f; // HSV hue shift
glm::vec3 sunPos(0.0f, 100.0f, 0.0f); // Position of the sun

rt3d::materialStruct material0 = {
	{0.4f, 0.4f, 0.4f, 1.0f}, // ambient
	{0.4f, 0.4f, 0.4f, 1.0f}, // diffuse
	{0.2f, 0.2f, 0.2f, 1.0f}, // specular
	2.0f  // shininess
};
rt3d::materialStruct material1 = {
	{0.4f, 0.4f, 4.0f, 1.0f}, // ambient
	{0.8f, 0.8f, 8.0f, 1.0f}, // diffuse
	{0.8f, 0.8f, 0.8f, 1.0f}, // specular
	1.0f  // shininess
};

// light attenuation
float attConstant = 1.0f;
float attLinear = 0.0f;
float attQuadratic = 0.0f;


float theta = 0.0f;
float theta_refxct = 0.0f;



// md2 stuff
md2model tmpModel;
int currentAnim = 0;

TTF_Font * textFont;

// textToTexture
GLuint textToTexture(const char * str, GLuint textID/*, TTF_Font *font, SDL_Color colour, GLuint &w,GLuint &h */) {
	TTF_Font *font = textFont;
	SDL_Color colour = { 255, 255, 255 };
	SDL_Color bg = { 0, 0, 0 };

	SDL_Surface *stringImage;
	stringImage = TTF_RenderText_Blended(font, str, colour);

	if (stringImage == NULL)
		//exitFatalError("String surface not created.");
		std::cout << "String surface not created." << std::endl;

	GLuint w = stringImage->w;
	GLuint h = stringImage->h;
	GLuint colours = stringImage->format->BytesPerPixel;

	GLuint format, internalFormat;
	if (colours == 4) {   // alpha
		if (stringImage->format->Rmask == 0x000000ff)
			format = GL_RGBA;
		else
			format = GL_BGRA;
	}
	else {             // no alpha
		if (stringImage->format->Rmask == 0x000000ff)
			format = GL_RGB;
		else
			format = GL_BGR;
	}
	internalFormat = (colours == 4) ? GL_RGBA : GL_RGB;

	GLuint texture = textID;

	if (texture == 0) {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	} //Do this only when you initialise the texture to avoid memory leakage

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, stringImage->w, stringImage->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, stringImage->pixels);
	glBindTexture(GL_TEXTURE_2D, NULL);

	SDL_FreeSurface(stringImage);
	return texture;
}


// Set up rendering context
SDL_Window * setupRC(SDL_GLContext &context) {
	SDL_Window * window;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) // Initialize video
		rt3d::exitFatalError("Unable to initialize SDL");

	// Request an OpenGL 3.0 context.

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // double buffering on
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // 8 bit alpha buffering
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // Turn on x4 multisampling anti-aliasing (MSAA)

	// Create 800x600 window
	window = SDL_CreateWindow("SDL/GLM/OpenGL Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!window) // Check window was created OK
		rt3d::exitFatalError("Unable to create window");

	context = SDL_GL_CreateContext(window); // Create opengl context and attach to window
	SDL_GL_SetSwapInterval(1); // set swap buffers to sync with monitor's vertical refresh rate
	return window;
}

// A simple texture loading function
// lots of room for improvement - and better error checking!
GLuint loadBitmap(char *fname) {
	GLuint texID;
	glGenTextures(1, &texID); // generate texture ID

	// load file - using core SDL library
	SDL_Surface *tmpSurface;
	tmpSurface = SDL_LoadBMP(fname);
	if (!tmpSurface) {
		std::cout << "Error loading bitmap" << std::endl;
	}

	// bind texture and set parameters
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SDL_PixelFormat *format = tmpSurface->format;

	GLuint externalFormat, internalFormat;
	if (format->Amask) {
		internalFormat = GL_SRGB_ALPHA;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGBA : GL_BGRA;
	}
	else {
		internalFormat = GL_RGB;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGB : GL_BGR;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tmpSurface->w, tmpSurface->h, 0, externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(tmpSurface); // texture loaded, free the temporary buffer
	return texID;	// return value of texture ID
}


// A simple cubemap loading function
// lots of room for improvement - and better error checking!
GLuint loadCubeMap(const char *fname[6], GLuint *texID)
{
	glGenTextures(1, texID); // generate texture ID
	GLenum sides[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
						GL_TEXTURE_CUBE_MAP_POSITIVE_X,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
						GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Y };
	SDL_Surface *tmpSurface;

	glBindTexture(GL_TEXTURE_CUBE_MAP, *texID); // bind texture and set parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	GLuint externalFormat;
	for (int i = 0; i < 6; i++)
	{
		// load file - using core SDL library
		tmpSurface = SDL_LoadBMP(fname[i]);
		if (!tmpSurface)
		{
			std::cout << "Error loading bitmap" << std::endl;
			return *texID;
		}

		// skybox textures should not have alpha (assuming this is true!)
		SDL_PixelFormat *format = tmpSurface->format;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGBA : GL_BGR;

		glTexImage2D(sides[i], 0, GL_SRGB_ALPHA, tmpSurface->w, tmpSurface->h, 0, externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
		// texture loaded, free the temporary buffer
		SDL_FreeSurface(tmpSurface);
	}
	return *texID;	// return value of texure ID, redundant really
}


void init(void) {

	// Basic Shader Program
	shaderProgram = rt3d::initShaders("phong-tex.vert", "phong-tex.frag");
	rt3d::setLight(shaderProgram, light0);
	rt3d::setMaterial(shaderProgram, material0);
	// set light attenuation shader uniforms
	GLuint uniformIndex = glGetUniformLocation(shaderProgram, "attConst");
	glUniform1f(uniformIndex, attConstant);
	uniformIndex = glGetUniformLocation(shaderProgram, "attLinear");
	glUniform1f(uniformIndex, attLinear);
	uniformIndex = glGetUniformLocation(shaderProgram, "attQuadratic");
	glUniform1f(uniformIndex, attQuadratic);

	// Enviroment Shader Program
	envmapProgram = rt3d::initShaders("phongEnvMap.vert", "phongEnvMap.frag");
	rt3d::setLight(envmapProgram, light0);
	rt3d::setMaterial(envmapProgram, material1);
	// set light attenuation shader uniforms
	uniformIndex = glGetUniformLocation(envmapProgram, "attConst");
	glUniform1f(uniformIndex, attConstant); 
	uniformIndex = glGetUniformLocation(envmapProgram, "attLinear");
	glUniform1f(uniformIndex, attLinear);
	uniformIndex = glGetUniformLocation(envmapProgram, "attQuadratic");
	glUniform1f(uniformIndex, attQuadratic);

	// Binding tex handles to tex units to samplers under programmer control
	// set cubemap sampler to texture unit 1, arbitrarily
	uniformIndex = glGetUniformLocation(envmapProgram, "textureUnit1");
	glUniform1i(uniformIndex, 1);
	// set tex sampler to texture unit 0, arbitrarily
	uniformIndex = glGetUniformLocation(envmapProgram, "textureUnit0");
	glUniform1i(uniformIndex, 0);

	// Toon Shader Program
	toonShaderProgram = rt3d::initShaders("toon.vert", "toon.frag");
	rt3d::setLight(toonShaderProgram, light0);
	rt3d::setMaterial(toonShaderProgram, material0);
	uniformIndex = glGetUniformLocation(toonShaderProgram, "attConst");
	glUniform1f(uniformIndex, attConstant);
	uniformIndex = glGetUniformLocation(toonShaderProgram, "attLinear");
	glUniform1f(uniformIndex, attLinear);
	uniformIndex = glGetUniformLocation(toonShaderProgram, "attQuadratic");
	glUniform1f(uniformIndex, attQuadratic);

	// reflection and refraction
	reflectrefractShaderProgram = rt3d::initShaders("reflectrefract.vert", "reflectrefract.frag");
	rt3d::setLight(reflectrefractShaderProgram, light0);
	rt3d::setMaterial(reflectrefractShaderProgram, material0);
	uniformIndex = glGetUniformLocation(reflectrefractShaderProgram, "attConst");
	glUniform1f(uniformIndex, attConstant);
	uniformIndex = glGetUniformLocation(reflectrefractShaderProgram, "attLinear");
	glUniform1f(uniformIndex, attLinear);
	uniformIndex = glGetUniformLocation(reflectrefractShaderProgram, "attQuadratic");
	glUniform1f(uniformIndex, attQuadratic);
	// set cubemap sampler to texture unit 1, arbitrarily
	uniformIndex = glGetUniformLocation(reflectrefractShaderProgram, "texture1_skycube");
	glUniform1i(uniformIndex, 1);

	// HSV ShaderProgram (w/Gamma Correction <--- To be added soon!)

	HSVShaderProgram = rt3d::initShaders("HSVVert.shader", "HSVFrag.shader");

	// Don't forget to 'use' the corresponding shader program first (to set the uniform)

	GLint objectColorLoc = glGetUniformLocation(HSVShaderProgram, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(HSVShaderProgram, "lightColor");
	GLint lightPosLoc = glGetUniformLocation(HSVShaderProgram, "lightPos"); 
	GLint lightPositionLoc = glGetUniformLocation(HSVShaderProgram, "light.position");
	GLint viewPosLoc = glGetUniformLocation(HSVShaderProgram, "viewPos");

	

	GLint lightAmbientLoc = glGetUniformLocation(HSVShaderProgram, "light.ambient");
	GLint lightDiffuseLoc = glGetUniformLocation(HSVShaderProgram, "light.diffuse");
	GLint lightSpecularLoc = glGetUniformLocation(HSVShaderProgram, "light.specular");
	GLint lightDirPos = glGetUniformLocation(HSVShaderProgram, "light.direction");
	GLint lightConstantPos = glGetUniformLocation(HSVShaderProgram, "light.constant");
	GLint lightLinearPos = glGetUniformLocation(HSVShaderProgram, "light.linear");
	GLint lightQuadraticPos = glGetUniformLocation(HSVShaderProgram, "light.quadratic");
	
	
	GLint lightSpotLoc = glGetUniformLocation(HSVShaderProgram, "light.spotPosition");
	GLint lightSpotdirLoc = glGetUniformLocation(HSVShaderProgram, "light.spotDirection");
	GLint lightSpotCutOffLoc = glGetUniformLocation(HSVShaderProgram, "light.cutOff");
	GLint lightSpotOuterCutOffLoc = glGetUniformLocation(HSVShaderProgram, "light.outerCutOff");

	// Multi-light - dirLight
	GLint dirLightLoc = glGetUniformLocation(HSVShaderProgram, "dirLight.direction");
	GLint ambientDirLightLoc = glGetUniformLocation(HSVShaderProgram, "dirLight.ambient");
	GLint diffuseDirLightLoc = glGetUniformLocation(HSVShaderProgram, "dirLight.diffuse");
	GLint specularDirLightLoc = glGetUniformLocation(HSVShaderProgram, "dirLight.specular");

	// Multi-light - pointLight
	GLint pointLightLoc = glGetUniformLocation(HSVShaderProgram, "pointLight.direction");
	GLint ambientPointLightLoc = glGetUniformLocation(HSVShaderProgram, "pointLight.ambient");
	GLint diffusePointLightLoc = glGetUniformLocation(HSVShaderProgram, "pointLight.diffuse");
	GLint specularPointLightLoc = glGetUniformLocation(HSVShaderProgram, "pointLight.specular");
	GLint constantPointLightLoc = glGetUniformLocation(HSVShaderProgram, "pointLight.constant");
	GLint linearPointLightLoc = glGetUniformLocation(HSVShaderProgram, "pointLight.linear");
	GLint quadraticPointLightLoc = glGetUniformLocation(HSVShaderProgram, "pointLight.quadratic");




	//glUniform3f(objectColorLoc, color.x, color.y, color.z);
	glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // Also set light's color (white)
	glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(viewPosLoc, eye.x, eye.y, eye.z); 

	// Set Material Properties

	GLint matAmbientLoc = glGetUniformLocation(HSVShaderProgram, "material.ambient");
	GLint matDiffuseLoc = glGetUniformLocation(HSVShaderProgram, "material.diffuse");
	GLint matSpecularLoc = glGetUniformLocation(HSVShaderProgram, "material.specular");
	GLint matShineLoc = glGetUniformLocation(HSVShaderProgram, "material.shininess");
	glUniform3f(matAmbientLoc, 1.0f, 0.5f, 0.31f);
	glUniform3f(matDiffuseLoc, 1.0f, 0.5f, 0.31f);
	glUniform3f(matSpecularLoc, 0.5f, 0.5f, 0.5f);
	glUniform1f(matShineLoc, 32.0f);

	// Set Light Properties
	glUniform3f(lightAmbientLoc, 0.2f, 0.2f, 0.2f);
	glUniform3f(lightDiffuseLoc, 0.5f, 0.5f, 0.5f); // Darken the light a bit to fit the scene
	glUniform3f(lightSpecularLoc, 1.0f, 1.0f, 1.0f);
	glUniform3f(lightDirPos, lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(lightPositionLoc, lightPos.x, lightPos.y, lightPos.z);

	// Set Light attenuation properties <- See for value reference: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=-Point+Light+Attenuation
	// These values are for attenuation distance: 50
	//glUniform1f(lightConstantPos, 1.0f); 
	//glUniform1f(lightLinearPos, 0.022f);
	//glUniform1f(lightQuadraticPos, 0.0019f);

	
	

	// Set Spotlight Properties

	// Set Directional Light Properties for multi-light
	//glUniform3f(dirLightLoc, sunPos.x, sunPos.y, sunPos.z); <---- Fix back to sun after debug from lightPos.
	glUniform3f(dirLightLoc, lightPos.x, lightPos.y, lightPos.z);

	glUniform3f(ambientDirLightLoc, 0.05f, 0.05f, 0.05f);
	glUniform3f(diffuseDirLightLoc, 0.05f, 0.05f, 0.05f); // Darken the light a bit to fit the scene
	glUniform3f(specularDirLightLoc, 0.1f, 0.1f, 0.1f);



	// Set Point Light Properties for multi-light
	glUniform3f(pointLightLoc, lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(ambientPointLightLoc, 0.05f, 0.05f, 0.05f);
	glUniform3f(diffusePointLightLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(specularPointLightLoc, 0.5f, 0.5f, 0.5f);
	glUniform1f(constantPointLightLoc, 1.0f);
	glUniform1f(linearPointLightLoc, 0.045f);
	glUniform1f(quadraticPointLightLoc, 0.0075f);


	textureProgram = rt3d::initShaders("textured.vert", "textured.frag");
	skyboxProgram = rt3d::initShaders("cubeMap.vert", "cubeMap.frag");
	HSVShaderProgram = rt3d::initShaders("HSVVert.shader", "HSVFrag.shader");


	const char *cubeTexFiles[6] = {
		"cloudy-skybox/back.bmp", 
		"cloudy-skybox/front.bmp", 
		"cloudy-skybox/right.bmp", 
		"cloudy-skybox/left.bmp", 
		"cloudy-skybox/up.bmp", 
		"cloudy-skybox/down.bmp"
	};
	loadCubeMap(cubeTexFiles, &skybox[0]);


	vector<GLfloat> verts;
	vector<GLfloat> norms;
	vector<GLfloat> tex_coords;
	vector<GLuint> indices;
	rt3d::loadObj("cube.obj", verts, norms, tex_coords, indices);
	meshIndexCount = indices.size();
	
	meshObjects[0] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), meshIndexCount, indices.data());

	
	meshObjects[1] = tmpModel.ReadMD2Model("tris.MD2");
	md2VertCount = tmpModel.getVertDataCount();


	verts.clear(); norms.clear(); tex_coords.clear(); indices.clear();
	rt3d::loadObj("bunny-5000.obj", verts, norms, tex_coords, indices);
	toonIndexCount = indices.size();
	meshObjects[2] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), nullptr, toonIndexCount, indices.data());

	verts.clear(); norms.clear(); tex_coords.clear(); indices.clear();
	rt3d::loadObj("statue.obj", verts, norms, tex_coords, indices);
	statueIndexCount = indices.size();
	meshObjects[3] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), nullptr, statueIndexCount, indices.data());

	verts.clear(); norms.clear(); tex_coords.clear(); indices.clear();
	rt3d::loadObj("ground2.obj", verts, norms, tex_coords, indices);
	groundIndexCount = indices.size();
	meshObjects[4] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), groundIndexCount, indices.data());

	// Textures:
	textures[0] = loadBitmap("concrete.bmp");
	textures[1] = loadBitmap("hobgoblin2.bmp");
	textures[2] = loadBitmap("studdedmetal.bmp");
	textures[3] = loadBitmap("lavaTexture.bmp");
	textures[4] = loadBitmap("container2.bmp");
	textures[5] = loadBitmap("container2_specular.bmp");


	///////////////////
	// Initialise FBO
	///////////////////

	// Generate FBO, RBO & Texture handles
	glGenFramebuffers(1, &fboID);
	glGenRenderbuffers(1, &depthBufID);
	glGenTextures(1, &reflectionTex);

	// Bind FBO, RBO & Texture & init storage and params
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboID);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBufID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, reflectionWidth, reflectionHeight);
	glBindTexture(GL_TEXTURE_2D, reflectionTex);
	// Need to set mag and min params
	// otherwise mipmaps are assumed
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, reflectionWidth, reflectionHeight, 0, GL_RGBA, GL_FLOAT, NULL);

	// random comment test

	// Map COLOR_ATTACHMENT0 to texture & DEPTH_ATTACHMENT to depth buffer RBO
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTex, 0);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufID);

	// Check for errors
	GLenum valid = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (valid != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer Object not complete" << std::endl;
	if (valid == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
		std::cout << "Framebuffer incomplete attachment" << std::endl;
	if (valid == GL_FRAMEBUFFER_UNSUPPORTED)
		std::cout << "FBO attachments unsupported" << std::endl;




	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	
	/*
	GL_FRAMEBUFFER_SRGB:
	Tell OpenGL that each subsequent drawing commands should first gamma correct colors from the sRGB color space before storing them in color buffer(s).
	sRGB is the color space, which roughly corresponds to a gamma of 2,2 and a standard for most home devices.
	*/
	glEnable(GL_FRAMEBUFFER_SRGB); 

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	


	// set up TrueType / SDL_ttf
	if (TTF_Init() == -1)
		cout << "TTF failed to initialise." << endl;

	textFont = TTF_OpenFont("MavenPro-Regular.ttf", 48);
	if (textFont == NULL)
		cout << "Failed to open font." << endl;

	labels[0] = 0;//First init to 0 to avoid memory leakage if it is dynamic
	labels[0] = textToTexture(" Hello ", labels[0]);//Actual set up of label. If dynamic, this should go in draw function
}

glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::sin(r*DEG_TO_RADIAN), pos.y, pos.z - d*std::cos(r*DEG_TO_RADIAN));
}

glm::vec3 moveRight(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::cos(r*DEG_TO_RADIAN), pos.y, pos.z + d*std::sin(r*DEG_TO_RADIAN));
}

void update(void) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_W]) eye = moveForward(eye, r, 0.1f);
	if (keys[SDL_SCANCODE_S]) eye = moveForward(eye, r, -0.1f);
	if (keys[SDL_SCANCODE_A]) eye = moveRight(eye, r, -0.1f);
	if (keys[SDL_SCANCODE_D]) eye = moveRight(eye, r, 0.1f);
	if (keys[SDL_SCANCODE_R]) eye.y += 0.1;
	if (keys[SDL_SCANCODE_F]) eye.y -= 0.1;

	if (keys[SDL_SCANCODE_I]) lightPos = glm::vec4(moveForward(glm::vec3(lightPos), r, +0.1f), lightPos.w);
	if (keys[SDL_SCANCODE_J]) lightPos = glm::vec4(moveRight(glm::vec3(lightPos), r, -0.1f), lightPos.w);
	if (keys[SDL_SCANCODE_K]) lightPos = glm::vec4(moveForward(glm::vec3(lightPos), r, -0.1f), lightPos.w);
	if (keys[SDL_SCANCODE_L]) lightPos = glm::vec4(moveRight(glm::vec3(lightPos), r, 0.1f), lightPos.w);
	if (keys[SDL_SCANCODE_U]) lightPos[1] += 0.1;
	if (keys[SDL_SCANCODE_H]) lightPos[1] -= 0.1;

	if (keys[SDL_SCANCODE_COMMA]) r -= 1.0f;
	if (keys[SDL_SCANCODE_PERIOD]) r += 1.0f;

	if (keys[SDL_SCANCODE_1]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
	}
	if (keys[SDL_SCANCODE_2]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}
	if (keys[SDL_SCANCODE_Z]) {
		if (--currentAnim < 0) currentAnim = 19;
		cout << "Current animation: " << currentAnim << endl;
	}
	if (keys[SDL_SCANCODE_X]) {
		if (++currentAnim >= 20) currentAnim = 0;
		cout << "Current animation: " << currentAnim << endl;
	}
}


void draw(SDL_Window * window) {

	unsigned int diffuseMap = textures[4];
	unsigned int specularMap = textures[5];
	unsigned int emissionMap = textures[3];

	// clear the screen
	glEnable(GL_CULL_FACE);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection(1.0);
	projection = glm::perspective(float(60.0f*DEG_TO_RADIAN), 800.0f / 600.0f, 1.0f, 150.0f);

	GLfloat scale(1.0f); // just to allow easy scaling of complete scene

	glm::mat4 modelview(1.0); // set base position for scene


	// render in two passes
	// first reflection to FBO
	// then scene with reflection on a quad
	for (int pass = 0; pass < 2; pass++) {

		// 'eye' is the world position of the camera will use this later
		at = moveForward(eye, r, 1.0f);

		mvStack.push(modelview);
		mvStack.top() = glm::lookAt(eye, at, up);

		if (pass == 0) {
			// set transformation matrix to construct virtual world, and render to FBO
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboID);  // binds the framebuffer object fboId to the framebuffer target GL_DRAW_FRAMEBUFFER
			glDrawBuffers(1, fboAttachments); // define an array of buffers (size one) into which outputs from the fragment shader data will be written
			glViewport(0, 0, reflectionWidth, reflectionHeight);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear FBO
			/* perform the translates required to reflect the scene (virtual world) */
			mvStack.top() = glm::translate(mvStack.top(), mirror_pos);
			mvStack.top() = glm::rotate(mvStack.top(), mirrorAngle, glm::vec3(0.0f, 1.0f, 0.0f));
			mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.0f, 1.0f, -1.0f));
			mvStack.top() = glm::rotate(mvStack.top(), -mirrorAngle, glm::vec3(0.0f, 1.0f, 0.0f));
			mvStack.top() = glm::translate(mvStack.top(), -mirror_pos);
		}
		else {
			// Render to frame buffer - this pass render the actual scene with the texture mapped (fbo from pass 0) quad (the "mirror")
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); //The value zero is reserved to represent the default framebuffer provided by the windowing system.
			glDrawBuffers(1, frameBuff); // For the default framebuffer, buffer names cannot be the one of the multiple buffer aliases; you must use GL_BACK_LEFT rather than GL_BACK.
			glViewport(0, 0, screenWidth, screenHeight);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear window
																// clear the screen
			glEnable(GL_CULL_FACE);
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		}

		//  render scene....

		// skybox as single cube using cube map
		glUseProgram(skyboxProgram);
		rt3d::setUniformMatrix4fv(skyboxProgram, "projection", glm::value_ptr(projection));
		glDepthMask(GL_FALSE); // make sure writing to update depth test is off
		glm::mat3 mvRotOnlyMat3 = glm::mat3(mvStack.top());
		mvStack.push(glm::mat4(mvRotOnlyMat3));

		/// N.B.
		if (pass) // i.e. != 0	
			glCullFace(GL_FRONT); // drawing inside of cube!
		else 
			glCullFace(GL_BACK);

		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox[0]);
		mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.5f, 1.5f, 1.5f));
		rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
		mvStack.pop();

		// N.B.
		if (pass)	
			glCullFace(GL_BACK); // drawing inside of cube!
		else 
			glCullFace(GL_FRONT);

		// back to remainder of rendering
		glDepthMask(GL_TRUE); // make sure depth test is on

		glm::vec4 tmp = mvStack.top()*lightPos;
		tmp = mvStack.top()*lightPos;
		light0.position[0] = tmp.x;
		light0.position[1] = tmp.y;
		light0.position[2] = tmp.z;

		// draw a small cube around light position to see how light moves
		glUseProgram(shaderProgram);
		rt3d::setUniformMatrix4fv(shaderProgram, "projection", glm::value_ptr(projection));
		rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp));

		mvStack.push(mvStack.top());

		mvStack.top() = glm::translate(mvStack.top(), glm::vec3(lightPos[0], lightPos[1], lightPos[2]));
		mvStack.top() = glm::scale(mvStack.top(), glm::vec3(0.25f, 0.25f, 0.25f));
		rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::setMaterial(shaderProgram, material0);
		rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp));
		rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);

		mvStack.pop();

		
		if (pass == 1) {
			///////////////////
			// Draw mirror & reflection with FBO
			///////////////////
			glUseProgram(textureProgram);
			rt3d::setUniformMatrix4fv(textureProgram, "projection", glm::value_ptr(projection));
			// Reset the model view matrix and draw a textured quad
			mvStack.push(mvStack.top());
			mvStack.top() = glm::translate(mvStack.top(), mirror_pos);//possible offset if you want the mirror over the ground plane for centered cube models
			mvStack.top() = glm::rotate(mvStack.top(), mirrorAngle, glm::vec3(0.0f, 1.0f, 0.0f));
			mvStack.top() = glm::scale(mvStack.top(), glm::vec3(2.0f, -1.5f, 0.0f));

			rt3d::setUniformMatrix4fv(textureProgram, "modelview", glm::value_ptr(mvStack.top()));

			// set tex sampler to texture unit 0, arbitrarily
			GLuint uniformIndex = glGetUniformLocation(textureProgram, "textureUnit0");
			glUniform1i(uniformIndex, 0);
			// Now bind textures to texture unit0 & draw the quad
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, reflectionTex);
			glCullFace(GL_BACK);
			rt3d::drawIndexedMesh(meshObjects[0], 6, GL_TRIANGLES); // Draw a quad using cube data
			glBindTexture(GL_TEXTURE_2D, 0);	// unbind the texture
			mvStack.pop();
		}

		glDisable(GL_CULL_FACE);
		// draw a ground plane
		glUseProgram(shaderProgram);
		rt3d::setUniformMatrix4fv(shaderProgram, "projection", glm::value_ptr(projection));
		rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp));
		glBindTexture(GL_TEXTURE_2D, textures[0]); // concrete texture
		mvStack.push(mvStack.top());
		mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-5.0f, 0.0f, -5.0f));
		mvStack.top() = glm::scale(mvStack.top(), glm::vec3(30.0f, 1.0f, 30.0f));
		rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::setMaterial(shaderProgram, material0);
		rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp));
		rt3d::drawIndexedMesh(meshObjects[4], groundIndexCount, GL_TRIANGLES);
		mvStack.pop();
		glEnable(GL_CULL_FACE);

		// draw the toon shaded bunny
		glUseProgram(toonShaderProgram);
		rt3d::setUniformMatrix4fv(toonShaderProgram, "projection", glm::value_ptr(projection));
		mvStack.push(mvStack.top());
		mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-8.0f, 0.0f, -2.0f));
		mvStack.top() = glm::scale(mvStack.top(), glm::vec3(10.0f, 10.0f, 10.0f));
		rt3d::setUniformMatrix4fv(toonShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::setLight(toonShaderProgram, light0);
		rt3d::setLightPos(toonShaderProgram, glm::value_ptr(tmp));
		rt3d::setUniformMatrix3fv(toonShaderProgram, "normalmatrix", glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(mvStack.top())))));
		rt3d::drawIndexedMesh(meshObjects[2], toonIndexCount, GL_TRIANGLES);
		mvStack.pop();

		

		// draw the statue with mixed reflection and refraction
		glUseProgram(reflectrefractShaderProgram);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox[0]);
		rt3d::setUniformMatrix4fv(reflectrefractShaderProgram, "projection", glm::value_ptr(projection));
		theta_refxct += 0.5f;
		glm::mat4 modelMatrix_refxct(1.0);
		mvStack.push(mvStack.top());
		modelMatrix_refxct = glm::translate(modelMatrix_refxct, glm::vec3(-5.0f, 0.0f, -3.0f));
		modelMatrix_refxct = glm::rotate(modelMatrix_refxct, float(theta*DEG_TO_RADIAN), glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix_refxct = glm::scale(modelMatrix_refxct, glm::vec3(1.0f, 1.0f, 1.0f));
		mvStack.top() = mvStack.top() * modelMatrix_refxct;
		rt3d::setUniformMatrix4fv(reflectrefractShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::setUniformMatrix4fv(reflectrefractShaderProgram, "modelMatrix", glm::value_ptr(modelMatrix_refxct));

		rt3d::setLight(reflectrefractShaderProgram, light0);
		rt3d::setLightPos(reflectrefractShaderProgram, glm::value_ptr(tmp));
		rt3d::setUniformMatrix3fv(reflectrefractShaderProgram, "normalmatrix", glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(mvStack.top())))));
		GLuint uniformIndex = glGetUniformLocation(reflectrefractShaderProgram, "cameraPos");
		glUniform3fv(uniformIndex, 1, glm::value_ptr(eye));
		rt3d::drawIndexedMesh(meshObjects[3], statueIndexCount, GL_TRIANGLES);
		mvStack.pop();


		// draw a rotating cube to be environment/reflection mapped
		glUseProgram(envmapProgram);
		rt3d::setLightPos(envmapProgram, glm::value_ptr(tmp));
		rt3d::setUniformMatrix4fv(envmapProgram, "projection", glm::value_ptr(projection));

		// Now bind textures to texture units
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox[0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[3]); // lava texture
		
		glm::mat4 modelMatrix(1.0);
		mvStack.push(mvStack.top());
		modelMatrix = glm::translate(modelMatrix, glm::vec3(5.0f, 2.5f, -1.0f));
		modelMatrix = glm::rotate(modelMatrix, float(theta*DEG_TO_RADIAN), glm::vec3(1.0f, 1.0f, 1.0f));
		theta += 0.5f;
		modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
		mvStack.top() = mvStack.top() * modelMatrix;
		rt3d::setUniformMatrix4fv(envmapProgram, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::setUniformMatrix4fv(envmapProgram, "modelMatrix", glm::value_ptr(modelMatrix));
		uniformIndex = glGetUniformLocation(envmapProgram, "cameraPos");
		glUniform3fv(uniformIndex, 1, glm::value_ptr(eye));
		rt3d::setMaterial(envmapProgram, material1);
		rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);

		// remember to use at least one pop operation per push...
		mvStack.pop(); // initial matrix
		

		// Draw cube using HSV Shader
		glUseProgram(HSVShaderProgram);
		rt3d::setLightPos(HSVShaderProgram, glm::value_ptr(tmp));
		rt3d::setUniformMatrix4fv(HSVShaderProgram, "projection", glm::value_ptr(projection));

		// Now bind textures to texture units

		// bind diffuse map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		GLint diffuseLocation = glGetUniformLocation(HSVShaderProgram, "material.diffuse");
		glUniform1i(diffuseLocation, 0);

		// bind specular map
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);
		GLint specularLocation = glGetUniformLocation(HSVShaderProgram, "material.specular");
		glUniform1i(diffuseLocation, 1);

		// bind emission map
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, emissionMap);
		GLint emissionLocation = glGetUniformLocation(HSVShaderProgram, "material.emission");
		glUniform1i(emissionLocation, 2);

		// Set HSV Properties
		GLint hueShiftLoc = glGetUniformLocation(HSVShaderProgram, "hueShift");
		GLint satBoostLoc = glGetUniformLocation(HSVShaderProgram, "satBoost");
		GLint ourImageLoc = glGetUniformLocation(HSVShaderProgram, "ourImage");
		glUniform3f(ourImageLoc, 1.0f, 1.0f, 0.0f);
		glUniform1f(satBoostLoc, 1.0f);
		glUniform1f(hueShiftLoc, hueShift);
		hueShift += 0.00005f;

		modelMatrix = glm::mat4(1.0);
		mvStack.push(mvStack.top());
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 2.5f, -1.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));

		mvStack.top() = mvStack.top() * modelMatrix;

		rt3d::setUniformMatrix4fv(HSVShaderProgram, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::setUniformMatrix4fv(HSVShaderProgram, "modelMatrix", glm::value_ptr(modelMatrix));

		//glUniform3fv(uniformIndex, 1, glm::value_ptr(eye));
		//rt3d::setMaterial(HSVShaderProgram, material1);
		rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);

		// remember to use at least one pop operation per push...
		mvStack.pop(); // initial matrix



		glDepthMask(GL_TRUE);


	}//end of for loop

	SDL_GL_SwapWindow(window); // swap buffers


}


// Program entry point - SDL manages the actual WinMain entry point for us
int main(int argc, char *argv[]) {
	SDL_Window * hWindow; // window handle
	SDL_GLContext glContext; // OpenGL context handle
	hWindow = setupRC(glContext); // Create window and render context 

	// Required on Windows *only* init GLEW to access OpenGL beyond 1.1
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) { // glewInit failed, something is seriously wrong
		std::cout << "glewInit failed, aborting." << endl;
		exit(1);
	}
	cout << glGetString(GL_VERSION) << endl;

	init();

	bool running = true; // set running to true
	SDL_Event sdlEvent;  // variable to detect SDL events
	while (running) {	// the event loop
		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_QUIT)
				running = false;
		}
		update();
		draw(hWindow); // call the draw function
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(hWindow);
	SDL_Quit();
	return 0;
}