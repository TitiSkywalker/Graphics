#pragma once

//image size
static constexpr int WIDTH = 400;
static constexpr int HEIGHT = 400;

//anti-aliasing
static constexpr bool SUPERSAMPLING = false;
static constexpr bool JITTER = false;
static constexpr bool GAUSSIANBLUR = false;	

// ray tracing 
static constexpr int SAMPLERATE = 10;			
static constexpr float EPSILON = 0.01;				//prevent self intersection
static constexpr float FALLOFF = 0.25;				//falloff for secondary rays
static constexpr int MAXDEPTH = 100;			
static constexpr float STOPPROBABILITY = 0.5;

// accelerating
static constexpr bool USEMPI = true;		

// choose input/output file
static constexpr int CHOICE = 0;

// edit this when you want to add new files or change filename
// all input/output files must be in the "input/output" directory
static const char* inputFiles[] =
{
	"scene0_glass.scene",
	"scene1_ball.scene",
	"scene2_bunny.scene",
	"scene3_diamond.scene",
	// note: the earth texture needs to be converted into .bmp file
	// you can find "converter.py" inside "texture" directory
	"scene4_earth.scene",
	"scene5_goat.scene",
	"scene6_eagle.scene",
};

static const char* outputFiles[] =
{
	"scene0_glass.bmp",
	"scene1_ballj.bmp",
	"scene2_bunny.bmp",
	"scene3_diamond.bmp",
	"scene4_earth.bmp",
	"scene5_goat.bmp",
	"scene6_eagle.bmp",
};