#ifndef MORTON_ORDER_HLSL
#define MORTON_ORDER_HLSL

uint SpreadBits(uint x) {
    x = (x | (x << 16)) & 0x0000FF0000FF;
    x = (x | (x <<  8)) & 0x00F00F00F00F;
    x = (x | (x <<  4)) & 0x0C30C30C30C3;
    x = (x | (x <<  2)) & 0x249249249249;
    return x;
}
// Compute Morton index for 2D coordinates (x, y)
uint MortonIndex2D(uint x, uint y) {
    return SpreadBits(x) | (SpreadBits(y) << 1);
}
// Compute 3D Morton index for (x, y, z)
uint MortonIndex3D(uint x, uint y, uint z) {
    return SpreadBits(x) | (SpreadBits(y) << 1) | (SpreadBits(z) << 2);
}


#endif