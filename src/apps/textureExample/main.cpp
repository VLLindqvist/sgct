#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();

unsigned int myTextureIndex;
sgct_utils::SGCTBox * myBox = NULL;

//variables to share across cluster
double curr_time = 0.0;

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );

	if( !gEngine->init() )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::Instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::Instance()->setDecodeFunction(myDecodeFun);

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;
	if(myBox != NULL) delete myBox;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	glPushAttrib( GL_ENABLE_BIT );
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	
	double speed = 25.0;
	
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -3.0f);
	glRotated(curr_time * speed, 0.0, -1.0, 0.0);
	glRotated(curr_time * (speed/2.0), 1.0, 0.0, 0.0);
	glColor3f(1.0f,1.0f,1.0f);
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(myTextureIndex) );
	//draw the box
	myBox->draw();
	glPopMatrix();

	glPopAttrib();
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		curr_time = sgct::Engine::getTime();
	}
}

void myInitOGLFun()
{
	sgct::TextureManager::Instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::Instance()->setCompression(sgct::TextureManager::S3TC_DXT);
	sgct::TextureManager::Instance()->loadTexure(myTextureIndex, "box", "box.png", true);

	myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::Regular);
	//myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::CubeMap);
	//myBox = new sgct_utils::SGCTBox(1.0f, sgct_utils::SGCTBox::SkyBox);

	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);

	//Set up backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); //our polygon winding is counter clockwise
}

void myEncodeFun()
{
	sgct::SharedData::Instance()->writeDouble(curr_time);
}

void myDecodeFun()
{
	curr_time = sgct::SharedData::Instance()->readDouble();
}
