#ifndef VISUALIZATION_HLSL
#define VISUALIZATION_HLSL


#include "Constants.hlsl"

/** Providing some methods to visualize the numeric value into a tile graph.
*
*/ 

// [ morgan3d, 2D Vector Field Flow. https://www.shadertoy.com/view/4s23DG]
/**
* @param p: texCoord for arrow, range in [0, 1].
* @param v: vector of the arrow.
*/ 
float Arrow(float2 p, float2 v) {
	static const float arrowHeadAngle = pi / 8.0;
	static const float allowHeadLength = 0.1;
	static const float allowShaftThickness = 0.01;

	// Make everything relative to the center, which may be fractional
	p -= 0.5;
		
    float lenV = length(v);
	float lenP = length(p);
	
	if (lenV > 0.0) {
		// Non-zero velocity case
		float2 dirP = p / lenP;
		float2 dirV = v / lenV;
		
		// We can't draw arrows larger than the tile radius, so clamp magnitude.
		// Enforce a minimum length to help see direction
		lenV = max(lenV * 0.5, allowHeadLength * 0.5);

		// Arrow tip location
		v = dirV * lenV;
		
		// Define a 2D implicit surface so that the arrow is antialiased.
		// In each line, the left expression defines a shape and the right controls
		// how quickly it fades in or out.

		float dist = max(
			// Shaft
			allowShaftThickness - max(
				abs(dot(p, float2(dirV.y, -dirV.x))), // Width
				abs(dot(p, dirV)) - lenV + allowHeadLength // Length
			), 
			
			// Arrow head
			min(0.0, dot(v - p, dirV) - cos(arrowHeadAngle) * length(v - p)) * 2.0 // Front sides
			+ min(0.0, dot(p, dirV) + allowHeadLength - lenV) // Back
		) * 128.0;
		
		return clamp(1.0 + dist, 0.0, 1.0);
	} 
	else {
		// Center of the pixel is always on the arrow
		return max(0.0, 1.2 - lenP);
	}
}


#endif