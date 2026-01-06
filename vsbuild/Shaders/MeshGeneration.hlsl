#ifndef MESH_GENERATION_HLSL
#define MESH_GENERATION_HLSL

/** Basis Mesh generation for Compute Shader 
* 
*/


// @return: Position xy of grid in specified index.
float2 GridVertex(uint vertexID, uint sideVertexCount, float interval) {
	uint2 index = uint2(vertexID / sideVertexCount, vertexID % sideVertexCount);
	// The grid center is generated in (0, 0).
	return interval * (index - (sideVertexCount * 0.5));
}

uint GridIndexCount(uint sideVertexCount) {
	uint sideCellCount = sideVertexCount - 1;
	return (sideCellCount * sideCellCount) * 6;
}

// TODO: Provide precomputed params input version for efficiency.
// @return: IndexPack which contains 4 indices.
uint4 GridIndexPack(uint indexPackID, uint sideVertexCount) {
	uint4 indices = 0;
	indices.x = indexPackID * 4;
	indices.y = indices.x + 1;
	indices.z = indices.x + 2;
	indices.w = indices.x + 3;
	indices = min(indices, GridIndexCount(sideVertexCount));

	// Calculate grid vertex index for the whole pack.
	uint4 triangleIndices = indices / 3;
	uint4 triangleLineCount = triangleIndices / (2 * sideVertexCount - 2);
	uint4 faceIndices = indices % 3;
    uint4 parity = triangleIndices & 1;
	uint4 offset = 
		(faceIndices == 0) * (sideVertexCount + parity) + 
		(faceIndices == 1) + 
        (faceIndices == 2) * (parity * sideVertexCount);
    return triangleIndices / 2 + triangleLineCount + offset;
}

#endif