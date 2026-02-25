#ifndef GREEDY_SNAKE_COMMON_HLSL
#define GREEDY_SNAKE_COMMON_HLSL

#include "../Random.hlsl"

// Enums
static const int greedySnakeID_Empty = 0;
static const int greedySnakeID_Block = 1;
static const int greedySnakeID_Food = 2;
static const int greedySnakeID_UpBody = 3;
static const int greedySnakeID_DownBody = 4;
static const int greedySnakeID_RightBody = 5;
static const int greedySnakeID_LeftBody = 6;

static const int greedySnakeInput_None = 0;
static const int greedySnakeInput_Reset = 1;
static const int greedySnakeInput_Up = 2;
static const int greedySnakeInput_Down = 3;
static const int greedySnakeInput_Right = 4;
static const int greedySnakeInput_Left = 5;

// Gameplays
static const float greedySnakeUpdateInterval = 0.03334;
static const float greedySnakeBlockLimit = 0.98;
static const float greedySnakeFoodLimit = 0.99;
static const float greedySnakeFoodRegenerationLimit = 0.999;

// SceneInfos
static const uint greedySnakeTileSideElements = 32;
static const float greedySnakeInvTileSideElements = 1.0 / greedySnakeTileSideElements;

bool IsGreedySnakeBody(int id) {
	return id >= greedySnakeID_UpBody && id <= greedySnakeID_LeftBody;
}

// @warning: You must ensure all input IDs are GreedySnakesBody.
bool IsReversedGreedySnakeBody(int idA, int idB) {
	// return (idA == greedySnakeInput_Up && idB == greedySnakeInput_Down) 
	// 	|| (idA == greedySnakeInput_Down && idB == greedySnakeInput_Up) 
	// 	|| (idA == greedySnakeInput_Right && idB == greedySnakeInput_Left) 
	// 	|| (idA == greedySnakeInput_Left && idB == greedySnakeInput_Right);
	return (abs(idA - idB) == 1) && (idA + idB != (greedySnakeID_DownBody + greedySnakeID_RightBody));
}

int ReverseGreedySnakeBody(int id) {
	return (id == greedySnakeID_UpBody || id == greedySnakeID_RightBody) ? id + 1 : 
		(id == greedySnakeID_DownBody || id == greedySnakeID_LeftBody) ? id - 1 : -1;
}

// @warning: This funciton includes temporal random, pay attention on synchronization from double buffers scene.
void ResetGreedySnakeTile(uint2 tileID, inout uint4 tileInfo) {
	/* TileInfo:
		.r : tile ID
		.g : snake's remaining life
	*/

	/*	tileID:
		0 : empty
		1 : block
		2 : food
		3 : snake's body top
		4 : snake's body bottom
		5 : snake's body right
		6 : snake's body left
	*/

	float timeSeed = frac(deltaTime) * 10.0;

	// Food
	if (Hash(tileID * greedySnakeInvTileSideElements, randomSeed1 * timeSeed) > greedySnakeFoodLimit) {
		tileInfo.r = greedySnakeID_Food;
	}
	// Block
	if (Hash(tileID * greedySnakeInvTileSideElements, randomSeed0 * timeSeed) > greedySnakeBlockLimit) {
		tileInfo.r = greedySnakeID_Block;
	}

	// Snake's body
	uint2 headTileID = greedySnakeTileSideElements / 2;
	if (all(tileID == headTileID)) {
		tileInfo.r = greedySnakeID_UpBody;	
		tileInfo.g = 2; // .g Scores
	}
	else if (all(tileID == headTileID + uint2(0, -1))) {
		tileInfo.r = greedySnakeID_UpBody;	
		tileInfo.g = 1;
	}

	// Edge Block
	if (any(tileID <= 0) || any(tileID >= greedySnakeTileSideElements - 1)) {
		tileInfo.r = greedySnakeID_Block;
	}
}

void ResetGreedySnakeGlobalStates() {
	GlobalStatesStruct states = (GlobalStatesStruct)0;

	states.reset = false;
	states.scores = 1;
	states.food = 0;
	states.accumulatedTime = 0.0;

	GlobalStatesOut[0] = states;
}

#endif