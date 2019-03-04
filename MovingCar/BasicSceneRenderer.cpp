#include "BasicSceneRenderer.h"
#include "Image.h"
#include "Prefabs.h"
#include <GL/glut.h>
#include <stdlib.h>
#include <iostream>

GLfloat d;
float score;
GLfloat p1x;
GLfloat p1y;
GLfloat p1z;
const int p1radius = 1;
const int p2radius = 0;

GLfloat p2x;
GLfloat p2y;
GLfloat p2z;

//var for left wall
GLfloat distLeftWall;
GLfloat p3LWx;
GLfloat p3LWy;
GLfloat p3LWz;

const int p3radius = 90;

//var for right wall
GLfloat distRightWall;
GLfloat p4RWx;
GLfloat p4RWy;
GLfloat p4RWz;

const int p4radius = 1;

//var for finish line
GLfloat distFinishLine;
GLfloat p5FLx;
GLfloat p5FLy;
GLfloat p5FLz;

const int p5radius = 1;

//check for if finish line is hit
bool finishLineHit = false;

// wall hit collision, cause player death
bool wallHit = false;



BasicSceneRenderer::BasicSceneRenderer()
    : mLightingModel(BLINN_PHONG_PER_FRAGMENT_DIR_LIGHT)
    , mCamera(NULL)
    , mProjMatrix(1.0f)
    , mActiveEntityIndex(0)
    , mDbgProgram(NULL)
    , mAxes(NULL)
    , mVisualizePointLights(true)
{}

void calculateDistance() {
	//set the distance to the current distance between car and obstacle
	d = sqrt(((p1x - p2x) * (p1x - p2x)) + ((p1y - p2y) * (p1y - p2y)) + ((p1z - p2z) * (p1z - p2z)));
}

void leftWallCollisionCheck() {
	distLeftWall = sqrt(((p1x - p3LWx) * (p1x - p3LWx)) + ((p1y - p1y) * (p1y - p1y)) + ((p1z - p1z) * (p1z - p1z)));
}

void rightWallCollisionCheck() {
	distRightWall = sqrt(((p1x - p4RWx) * (p1x - p4RWx)) + ((p1y - p1y) * (p1y - p1y)) + ((p1z - p1z) * (p1z - p1z)));
}

void finishLineCollisionCheck() {
	distFinishLine = sqrt(((p1x - p1x) * (p1x - p1x)) + ((p1y - p1y) * (p1y - p1y)) + ((p1z - p5FLz) * (p1z - p5FLz)));
}

void setVSync(bool sync)
{
	// Function pointer for the wgl extention function we need to enable/disable
	// vsync
	typedef BOOL(APIENTRY *PFNWGLSWAPINTERVALPROC)(int);
	PFNWGLSWAPINTERVALPROC wglSwapIntervalEXT = 0;

	const char *extensions = (char*)glGetString(GL_EXTENSIONS);

	if (strstr(extensions, "WGL_EXT_swap_control") == 0)
	{
		return;
	}
	else
	{
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALPROC)wglGetProcAddress("wglSwapIntervalEXT");

		if (wglSwapIntervalEXT)
			wglSwapIntervalEXT(sync);
	}
}

void BasicSceneRenderer::initialize()
{
    // print usage instructions
    std::cout << "Usage:" << std::endl;
    std::cout << "  Camera control:           WASD + Mouse" << std::endl;
    std::cout << "  Rotate active entity:     Arrow keys" << std::endl;
    std::cout << "  Reset entity orientation: R" << std::endl;
    std::cout << "  Translate active entity:  IJKL (world space)" << std::endl;
    std::cout << "  Translate active entity:  TFGH (local space)" << std::endl;
    std::cout << "  Cycle active entity:      X/Z" << std::endl;
    std::cout << "  Toggle point light vis.:  Tab" << std::endl;

    glClearColor(0.494117f, 0.75294f, 0.93333f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // enable blending (needed for textures with alpha channel)
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mPrograms.resize(NUM_LIGHTING_MODELS);

    mPrograms[PER_VERTEX_DIR_LIGHT] = new ShaderProgram("shaders/PerVertexDirLight-vs.glsl",
                                                                "shaders/PerVertexDirLight-fs.glsl");
    
    mPrograms[BLINN_PHONG_PER_FRAGMENT_DIR_LIGHT] = new ShaderProgram("shaders/BlinnPhongPerFragment-vs.glsl",
                                                                      "shaders/BlinnPhongPerFragmentDirLight-fs.glsl");

    mPrograms[BLINN_PHONG_PER_FRAGMENT_POINT_LIGHT] = new ShaderProgram("shaders/BlinnPhongPerFragment-vs.glsl",
                                                                        "shaders/BlinnPhongPerFragmentPointLight-fs.glsl");

    mPrograms[BLINN_PHONG_PER_FRAGMENT_MULTI_LIGHT] = new ShaderProgram("shaders/BlinnPhongPerFragment-vs.glsl",
                                                                        "shaders/BlinnPhongPerFragmentMultiLight-fs.glsl");

    //
    // Create meshes
    //
	//TODO: Mesh Creation starts here
    mMeshes.push_back(CreateTexturedCube(1));
    mMeshes.push_back(CreateChunkyTexturedCylinder(0.5f, 1, 8));
    mMeshes.push_back(CreateSmoothTexturedCylinder(0.5f, 1, 15));

    float roomWidth = 12;
    float roomHeight = 24;
    float roomDepth = 680;
    float roomTilesPerUnit = 0.25f;

    // front and back walls
    Mesh* fbMesh = CreateTexturedQuad(roomWidth, roomHeight, roomWidth * roomTilesPerUnit, roomHeight * roomTilesPerUnit);
    mMeshes.push_back(fbMesh);

    // left and right wall
	mMeshes.push_back(CreateSolidBox(roomDepth, 2, 1));

    // ceiling and floor
    Mesh* cfMesh = CreateTexturedQuad(roomWidth, roomDepth, roomWidth * roomTilesPerUnit, roomDepth * roomTilesPerUnit);
    mMeshes.push_back(cfMesh);

	//finishline banner
	Mesh* flMesh = CreateTexturedQuad(roomWidth, 3, roomDepth * roomTilesPerUnit, 3 * roomTilesPerUnit);
	mMeshes.push_back(flMesh);

	//win/loss banner
	Mesh* winLossMesh = CreateTexturedQuad(3.4, 2.5, 1, 1);
	mMeshes.push_back(winLossMesh);

    //
    // Load textures
    //

    std::vector<std::string> texNames;
    texNames.push_back("textures/CarvedSandstone.tga");
	texNames.push_back("textures/Road.tga");
    texNames.push_back("textures/rocky.tga");
    texNames.push_back("textures/Block.tga");
    texNames.push_back("textures/Eyes.tga");
    texNames.push_back("textures/Rock.tga");
    texNames.push_back("textures/white.tga");
    texNames.push_back("textures/yo.tga");

    for (unsigned i = 0; i < texNames.size(); i++)
        mTextures.push_back(new Texture(texNames[i], GL_REPEAT, GL_LINEAR));

	texNames.push_back("textures/YoureLoser.tga");
	texNames.push_back("textures/YoureWinner.tga");

	mTextures.push_back(new Texture(texNames[8], GL_CLAMP_TO_EDGE, GL_LINEAR));
	mTextures.push_back(new Texture(texNames[9], GL_CLAMP_TO_EDGE, GL_LINEAR));

    //
    // Create materials
    //

    // add a material for each loaded texture (with default tint)
    for (unsigned i = 0; i < texNames.size(); i++)
        mMaterials.push_back(new Material(mTextures[i]));

    //
    // set extra material properties
    //

    // water drops (sharp and strong specular highlight)
    mMaterials[3]->specular = glm::vec3(1.0f, 1.0f, 1.0f);
    mMaterials[3]->shininess = 128;

    // skin (washed out and faint specular highlight)
    mMaterials[4]->specular = glm::vec3(0.3f, 0.3f, 0.3f);
    mMaterials[4]->shininess = 8;

    // white
    mMaterials[5]->specular = glm::vec3(0.75f, 0.75f, 0.75f);
    mMaterials[5]->shininess = 64;

    // yo
    mMaterials[6]->specular = glm::vec3(1.0f, 0.0f, 1.0f);  // magenta highlights
    mMaterials[6]->shininess = 16;

    // black
    mMaterials[7]->specular = glm::vec3(1.0f, 0.5f, 0.0f);  // orange hightlights
    mMaterials[7]->shininess = 16;

    //
    // Create entities
    //

    unsigned numRows = mMaterials.size();
    float spacing = 3;
    float z = 0.5f * spacing * numRows;
	//TODO: Car and Obstacles created here

	// left wall
	mEntities.push_back(new Entity(mMeshes[4], mMaterials[5], Transform(-0.5f * roomWidth, -11, -300, glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)))));

	// right wall
	mEntities.push_back(new Entity(mMeshes[4], mMaterials[5], Transform(0.5f * roomWidth, -11, -300, glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)))));

	//finish line
	mEntities.push_back(new Entity(flMesh, mMaterials[6], Transform(0, -5.5, -620, glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)))));

	// floor
	mEntities.push_back(new Entity(cfMesh, mMaterials[1], Transform(0, -0.5f * roomHeight, -300, glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)))));
	//car
	mEntities.push_back(new Entity(mMeshes[0], mMaterials[4], Transform(0.0f, -11.5f, 30)));
	//test obstacle
	mEntities.push_back(new Entity(mMeshes[0], mMaterials[3], Transform(2.0f, -11.5f, 20)));
	z -= spacing;

    // create the camera

    mCamera = new Camera(this);
    mCamera->setPosition(0, -8, 40);
    mCamera->lookAt(0, -8, 0);
    mCamera->setSpeed(6);

    // create shader program for debug geometry
    mDbgProgram = new ShaderProgram("shaders/vpc-vs.glsl",
                                    "shaders/vcolor-fs.glsl");

    // create geometry for axes
    mAxes = CreateAxes(2);

	setVSync(1);  // enable VSync

    CHECK_GL_ERRORS("initialization");
}

void BasicSceneRenderer::shutdown()
{
    for (unsigned i = 0; i < mPrograms.size(); i++)
        delete mPrograms[i];
    mPrograms.clear();

    delete mDbgProgram;
    mDbgProgram = NULL;

    delete mCamera;
    mCamera = NULL;

    for (unsigned i = 0; i < mEntities.size(); i++)
        delete mEntities[i];
    mEntities.clear();

    for (unsigned i = 0; i < mMeshes.size(); i++)
        delete mMeshes[i];
    mMeshes.clear();

    for (unsigned i = 0; i < mMaterials.size(); i++)
        delete mMaterials[i];
    mMaterials.clear();
    
    for (unsigned i = 0; i < mTextures.size(); i++)
        delete mTextures[i];
    mTextures.clear();

    delete mDbgProgram;
    mDbgProgram = NULL;
    
    delete mAxes;
    mAxes = NULL;
}

void BasicSceneRenderer::resize(int width, int height)
{
    glViewport(0, 0, width, height);

    // compute new projection matrix
    mProjMatrix = glm::perspective(glm::radians(50.f), width / (float)height, 0.1f, 1000.0f);
}

void BasicSceneRenderer::draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // activate current program
    ShaderProgram* prog = mPrograms[mLightingModel];
    prog->activate();

    // send projection matrix
    prog->sendUniform("u_ProjectionMatrix", mProjMatrix);

    // send the texture sampler id to shader
    prog->sendUniformInt("u_TexSampler", 0);

    // get the view matrix from the camera
    glm::mat4 viewMatrix = mCamera->getViewMatrix();

    // light setup depends on lighting model

    if (mLightingModel == PER_VERTEX_DIR_LIGHT) {

        //----------------------------------------------------------------------------------//
        //                                                                                  //
        // Basic directional light (no ambient, specular, or emissive contributions)        //
        //                                                                                  //
        //----------------------------------------------------------------------------------//

        // direction to light
        glm::vec4 lightDir = glm::normalize(glm::vec4(1, 3, 2, 0));

        // send light direction in eye space
        prog->sendUniform("u_LightDir", glm::vec3(viewMatrix * lightDir));

        // send light color/intensity
        prog->sendUniform("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f));

    } else if (mLightingModel == BLINN_PHONG_PER_FRAGMENT_DIR_LIGHT) {

        //----------------------------------------------------------------------------------//
        //                                                                                  //
        // Directional light with ambient, specular, and emissive contributions             //
        //                                                                                  //
        //----------------------------------------------------------------------------------//

        prog->sendUniform("u_AmbientLightColor", glm::vec3(0.2f, 0.2f, 0.2f));

        // direction to light
        glm::vec4 lightDir = glm::normalize(glm::vec4(1, 3, 2, 0));

        // send light direction in eye space
        prog->sendUniform("u_LightDir", glm::vec3(viewMatrix * lightDir));

        // send light color/intensity
        prog->sendUniform("u_LightColor", glm::vec3(0.8f, 0.8f, 0.8f));

    } else if (mLightingModel == BLINN_PHONG_PER_FRAGMENT_POINT_LIGHT) {

        //----------------------------------------------------------------------------------//
        //                                                                                  //
        // Point light with ambient, specular, and emissive contributions, and attenuation  //
        //                                                                                  //
        //----------------------------------------------------------------------------------//

        prog->sendUniform("u_AmbientLightColor", glm::vec3(0.1f, 0.1f, 0.1f));

        // point light position
        glm::vec3 lightPos = glm::vec3(0, 7, 0);
        glm::vec3 lightColor = glm::vec3(1.0f, 0.9f, 0.8f);

        // send light position in eye space
        prog->sendUniform("u_LightPos", glm::vec3(viewMatrix * glm::vec4(lightPos, 1)));

        // send light color/intensity
        prog->sendUniform("u_LightColor", lightColor);

        prog->sendUniform("u_AttQuat", 0.005f);
        prog->sendUniform("u_AttLin", 0.05f);
        prog->sendUniform("u_AttConst", 1.0f);

        // render the light as an emissive cube, if desired
        if (mVisualizePointLights) {
            const Mesh* lightMesh = mMeshes[0];
            lightMesh->activate();
            glBindTexture(GL_TEXTURE_2D, mTextures[7]->id());  // use black texture
            prog->sendUniform("u_MatEmissiveColor", lightColor);
            prog->sendUniform("u_ModelviewMatrix", glm::translate(viewMatrix, glm::vec3(lightPos)));
            prog->sendUniform("u_NormalMatrix", glm::mat3(1.0f));
            lightMesh->draw();
        }

    }

    // render all entities
    for (unsigned i = 0; i < mEntities.size(); i++) {

        Entity* ent = mEntities[i];

        // use the entity's material
        const Material* mat = ent->getMaterial();
        glBindTexture(GL_TEXTURE_2D, mat->tex->id());   // bind texture
        prog->sendUniform("u_Tint", mat->tint);     // send tint color

        // send the Blinn-Phong parameters, if required
        if (mLightingModel > PER_VERTEX_DIR_LIGHT) {
            prog->sendUniform("u_MatEmissiveColor", mat->emissive);
            prog->sendUniform("u_MatSpecularColor", mat->specular);
            prog->sendUniform("u_MatShininess", mat->shininess);
        }

        // compute modelview matrix
        glm::mat4 modelview = viewMatrix * ent->getWorldMatrix();

        // send the entity's modelview and normal matrix
        prog->sendUniform("u_ModelviewMatrix", modelview);
        prog->sendUniform("u_NormalMatrix", glm::transpose(glm::inverse(glm::mat3(modelview))));

        // use the entity's mesh
        const Mesh* mesh = ent->getMesh();
        mesh->activate();
        mesh->draw();
    }

    //
    // draw local axes for current entity
    //

    mDbgProgram->activate();
    mDbgProgram->sendUniform("u_ProjectionMatrix", mProjMatrix);

    Entity* activeEntity = mEntities[mActiveEntityIndex];
    mDbgProgram->sendUniform("u_ModelviewMatrix", viewMatrix * activeEntity->getWorldMatrix());
    mAxes->activate();
    //comment out the line below to get rid of axes
	//mAxes->draw();

    CHECK_GL_ERRORS("drawing");
}

bool BasicSceneRenderer::update(float dt)
{
	//TODO: Update starts here
	const Keyboard* kb = getKeyboard();

	if (kb->keyPressed(KC_ESCAPE))
		return false;

	mActiveEntityIndex = 4;
	int rbuffer = rand() % 5 + 1;
	int randomX = rand() % 5 - rbuffer;
	int randomZ = rand() % 32 + 16;

	// get the entity to manipulate
	Entity* activeEntity = mEntities[mActiveEntityIndex];
	//player is the first thing we render so it's mEntities 0
	Entity* playerVehicle = mEntities[4];
	//test obstacle
	Entity* obstacle = mEntities[5];
	//left wall
	Entity* leftWall = mEntities[0];
	//right wall
	Entity* rightWall = mEntities[1];
	//finish line
	Entity* finishLine = mEntities[2];
	//position of the car, can use carPos.x, etc. for coordinates
	glm::vec3 carPos = playerVehicle->getPosition();
	//position of test obstacle
	glm::vec3 obsPos = obstacle->getPosition();
	//camera position vector
	glm::vec3 cameraPosition = mCamera->getPosition();
	//left wall position
	glm::vec3 leftWallPos = leftWall->getPosition();
	//right wall position
	glm::vec3 rightWallPos = rightWall->getPosition();
	//finish line position
	glm::vec3 finishLinePos = finishLine->getPosition();

	// set up for pickup rotation 
	float rotSpeed = 90;
	float rotAmount = rotSpeed * dt;

	//capture of left wall
	p3LWx = leftWallPos.x;
	p3LWy = leftWallPos.y;
	p3LWz = leftWallPos.z;

	//capture of right wall
	p4RWx = rightWallPos.x;
	p4RWy = rightWallPos.y;
	p4RWz = rightWallPos.z;

	//capture of finish line
	p5FLx = finishLinePos.x;
	p5FLy = finishLinePos.y;
	p5FLz = finishLinePos.z;

	//capture of car position
	p1x = carPos.x;
	p1y = carPos.y;
	p1z = carPos.z;

	//capture of object position
	p2x = obsPos.x;
	p2y = obsPos.y;
	p2z = obsPos.z;
	
	//collision check with pickups
	calculateDistance();

	//collision check with left wall
	leftWallCollisionCheck();
	
	//collision check with right wall
	rightWallCollisionCheck();

	//collision check with finish line
	finishLineCollisionCheck();


	if (!finishLineHit && !wallHit) {

		// rotate the entity
		mEntities[5]->rotate(2.5, 0, 1.0, 0);

		//movement controls for the car, forward, backward, left, right
		//if key's pressed
		if (kb->isKeyDown(KC_LEFT)) {
			if (d < 1) {
				d += 0.1;
				//std::cout << d << std::endl;
				//playerVehicle->translateLocal(0.15, 0, 0);
				mEntities.pop_back();
				mEntities.push_back(new Entity(mMeshes[0], mMaterials[3], Transform(randomX, -11.5, carPos.z - randomZ)));
				score += 1;
				std::cout << score << std::endl;
			}
			else if (distLeftWall > 1.1) {
				playerVehicle->translateLocal(-0.1, 0, 0);
			}
			else {
				// wall hit collision, cause player death
				wallHit = true;

				// pop pickup to replace with loss condition mesh
				mEntities.pop_back();
				if (kb->isKeyDown(KC_UP) || kb->isKeyDown(KC_Z)) { mEntities.push_back(new Entity(mMeshes[7], mMaterials[8], Transform(0, -8, cameraPosition.z - 3, glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f))))); }
				else { mEntities.push_back(new Entity(mMeshes[7], mMaterials[8], Transform(0, -8, cameraPosition.z - 2.5, glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f))))); }

			}
		}
		if (kb->isKeyDown(KC_RIGHT)) {
			//activeEntity->rotate(-rotAmount, 0, 1, 0);
			if (d < 1) {
				d += 0.1;
				//std::cout << d << std::endl;
				//playerVehicle->translateLocal(-0.15, 0, 0);
				mEntities.pop_back();
				mEntities.push_back(new Entity(mMeshes[0], mMaterials[3], Transform(randomX, -11.5, carPos.z - randomZ)));
				score += 1;
				std::cout << score << std::endl;
			}
			else if (distRightWall > 1.1) {
				playerVehicle->translateLocal(0.1, 0, 0);
			}
			else {
				// wall hit collision, cause player death
				wallHit = true;

				// pop pickup to replace with loss condition mesh
				mEntities.pop_back();
				if (kb->isKeyDown(KC_UP)|| kb->isKeyDown(KC_Z)) { mEntities.push_back(new Entity(mMeshes[7], mMaterials[8], Transform(0, -8, cameraPosition.z - 3, glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f))))); }
				else { mEntities.push_back(new Entity(mMeshes[7], mMaterials[8], Transform(0, -8, cameraPosition.z - 2.5, glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f))))); }
				

			}

		}
		if (kb->isKeyDown(KC_Z) || kb->isKeyDown(KC_UP)) {
			if (d < 1) {
				d += 0.1;
				//std::cout << d << std::endl;
				//playerVehicle->translateLocal(0, 0, 0.1);
				mCamera->setPosition(cameraPosition.x, cameraPosition.y, cameraPosition.z);
				mEntities.pop_back();
				mEntities.push_back(new Entity(mMeshes[0], mMaterials[3], Transform(randomX, -11.5, carPos.z - randomZ)));
				score += 1;
				std::cout << score << std::endl;

			}
			//if finish line reached, pop car and pickup
			else if (distFinishLine < 0.525) {
				finishLineHit = true;

				// pop pickup to replace with win condition mesh
				mEntities.pop_back();
				mEntities.push_back(new Entity(mMeshes[7], mMaterials[9], Transform(0, -8, cameraPosition.z - 2.5, glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)))));
			}
			else {
				playerVehicle->translateLocal(0, 0, -0.5);
				mCamera->setPosition(cameraPosition.x, cameraPosition.y, cameraPosition.z - 0.5);
			}

		}
		else if (kb->isKeyDown(KC_X) || kb->isKeyDown(KC_DOWN)) {
			if (d < 1) {
				d += 0.1;
				//std::cout << d << std::endl; 
				//playerVehicle->translateLocal(0, 0, -0.1);
				mCamera->setPosition(cameraPosition.x, cameraPosition.y, cameraPosition.z);
				mEntities.pop_back();
				mEntities.push_back(new Entity(mMeshes[0], mMaterials[3], Transform(randomX, -11.5, carPos.z - randomZ)));
				score += 1;
				std::cout << score << std::endl;
			}
			//if finish line reached, pop car and pickup
			else if (distFinishLine < 0.525) {
				finishLineHit = true;

				// pop pickup to replace with win condition mesh
				mEntities.pop_back();
				//winner mesh
				mEntities.push_back(new Entity(mMeshes[7], mMaterials[9], Transform(0, -8, cameraPosition.z - 2.5, glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)))));
			}
			else {
				playerVehicle->translateLocal(0, 0, 0.1);
				mCamera->setPosition(cameraPosition.x, cameraPosition.y, cameraPosition.z + 0.1);
			}

		}
	}

    // update the camera
    mCamera->update(dt);

    return true;
}
