#include "GreedySnakeCommon.hlsl"
#include "../Random.hlsl"

inline uint2 SnakeBodyFrontTileBias(uint snakeBody) {
	switch(snakeBody) {			
	case greedySnakeID_UpBody:
		return uint2(0, 1);
	case greedySnakeID_DownBody:
		return uint2(0, -1);
	case greedySnakeID_RightBody:
		return uint2(1, 0);
	case greedySnakeID_LeftBody: 
		return uint2(-1, 0);
	}
	return 0;
}

inline void Update(uint2 tileID, in GlobalStatesStruct states, inout uint4 tileInfo) {
	// Update GlobalState
	if (all(tileID == 0)) {
		GlobalStatesStruct statesOut = states;
		statesOut.reset = false;
		statesOut.food = 0;
		GlobalStatesOut[0] = statesOut;		
	}
	
	// GroupMemoryBarrierWithGroupSync();

	uint2 backTileID = tileID;

	// Handle input
	int inputDirection = -1;
	switch(int(input)) {
	case greedySnakeInput_Reset:
		GlobalStatesOut[0].reset = true;
		break;
	case greedySnakeInput_Up:
		backTileID += uint2(0, -1);
		inputDirection = greedySnakeID_UpBody;
		break;
	case greedySnakeInput_Down:
		backTileID += uint2(0, 1);
		inputDirection = greedySnakeID_DownBody;
		break;
	case greedySnakeInput_Right:
		backTileID += uint2(-1, 0);
		inputDirection = greedySnakeID_RightBody;
		break;
	case greedySnakeInput_Left:
		backTileID += uint2(1, 0);
		inputDirection = greedySnakeID_LeftBody;
		break;
	}

	backTileID = clamp(backTileID, 0, greedySnakeTileSideElements - 1);
	uint4 backTileInfo = TileInfoIn[backTileID];

	if (tileInfo.g == states.scores && !IsGreedySnakeBody(TileInfoIn[tileID + SnakeBodyFrontTileBias(tileInfo.r)].r)) {
		tileInfo.r = IsReversedGreedySnakeBody(tileInfo.r, inputDirection) ? tileInfo.r : inputDirection;
	}

	// Head Condition
	bool isHead = false;
	if (backTileInfo.g == states.scores && !IsGreedySnakeBody(TileInfoIn[backTileID + SnakeBodyFrontTileBias(backTileInfo.r)].r)) {
		bool reversed = IsReversedGreedySnakeBody(backTileInfo.r, inputDirection);
		int headDirection = reversed ? backTileInfo.r : inputDirection;
		GlobalStatesOut[0].scores = max(states.scores + (reversed ? -1 : 0), 1);
		isHead = true;
		switch(tileInfo.r) {
		case greedySnakeID_Empty:
			tileInfo.r = headDirection;
			tileInfo.g = backTileInfo.g;
			break;
		case greedySnakeID_Block:
			GlobalStatesOut[0].reset = true;
			break;
		case greedySnakeID_Food:
			GlobalStatesOut[0].food = states.food + 1;
			GlobalStatesOut[0].scores = states.scores + 1;
			// InterlockedAdd(GlobalStatesOut[0].food, 1);
			// InterlockedAdd(GlobalStatesOut[0].scores, 1);
			tileInfo.r = headDirection;
			tileInfo.g = backTileInfo.g;
			break;
		}
	}

	// GroupMemoryBarrierWithGroupSync();

	if (tileInfo.g == states.scores) {
		uint2 frontTileID = tileID + SnakeBodyFrontTileBias(tileInfo.r);
		if (!IsGreedySnakeBody(TileInfoIn[frontTileID].r)) {
			tileInfo.g += 1;
		}
		// tileInfo.g += GlobalStatesOut[0].food;
	} 
	// Update Body
	if (IsGreedySnakeBody(tileInfo.r)) { 
		// Snake Eating
		if (GlobalStatesOut[0].food) {
			tileInfo.g += GlobalStatesOut[0].food;
		}
		// Snake Hungry
		else {
			tileInfo.g = max(int(tileInfo.g) - 1, 0);
		}
		
		tileInfo.r *= tileInfo.g != 0;
	}
	// Update Scene (Generate new foods)
	else if (tileInfo.r == greedySnakeID_Empty && !isHead){ 
		float randomValue = Hash(tileID * greedySnakeInvTileSideElements, randomSeed0 * frac(time));
		if (randomValue > greedySnakeFoodRegenerationLimit) {
			tileInfo.r = greedySnakeID_Food;
		}
	}
}

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	GlobalStatesStruct states = GlobalStatesIn[0];
	uint4 tileInfo = TileInfoIn[threadID.xy];

	if (states.reset) {
		tileInfo = 0;
		ResetGreedySnakeTile(threadID.xy, tileInfo);
		if (all(threadID == 0)) {
			ResetGreedySnakeGlobalStates();
		}
	}

	// @note: This code solved the following issuses:
	// 		Snake has not grow in low frame rate
	// 	    And tilt steps in height frame rate
	if (int(frameIndex) == 0) {
		GlobalStatesOut[0] = states;
		TileInfoOut[threadID.xy] = tileInfo;
		return;
	}

	static const uint maxExecutions = 8;
	uint executionCount = 0;
	float remainingTime = states.accumulatedTime;

	while(remainingTime > greedySnakeUpdateInterval) {
		if (executionCount >= maxExecutions) {
			remainingTime = 0.0;
			break;
		}
		Update(threadID.xy, states, tileInfo);
		remainingTime -= greedySnakeUpdateInterval;
		// GroupMemoryBarrierWithGroupSync();
	}

	// Update accumulatedTime
	if (all(threadID == 0)) {
		GlobalStatesOut[0].accumulatedTime = remainingTime + deltaTime;
	}

	TileInfoOut[threadID.xy] = tileInfo;
}