#ifndef COLOR_CONVERSION_HLSL
#define COLOR_CONVERSION_HLSL

float Grayscale(in float3 color) {
	return color.r * 0.299 + color.g * 0.587 + color.b * 0.114;
}

float3 Desaturation(in float3 color, in float value) {
	return lerp(color, Grayscale(color), value);
}

float3 LinearRGBToSRGB(in float3 linearColor) {
	float3 srgbLower = linearColor * 12.92;
	float3 srgbHigher = 1.055 * pow(linearColor, 1.0 / 2.4) - 0.055;
	return lerp(srgbHigher, srgbLower, step(linearColor, 0.0031308));
}

float3 SRGBToLinearRGB(in float3 srgbColor) {
	float3 linearLower = srgbColor / 12.92;
	float3 linearHigher = pow((srgbColor + 0.055) / 1.055, 2.4);
	return lerp(linearHigher, linearLower, step(srgbColor, 0.04045));
}


// HSV <-> RGB, From Unreal Engine.
float3 HueToLinearRGB(in float hue) {
	// mul 6 maps hue from [0, 1] to [0, 6], To aquire a series of tend-shape weight.
	float r = abs(hue * 6 - 3) - 1;
	float g = 2 - abs(hue * 6 - 2);
	float b = 2 - abs(hue * 6 - 4);
	return saturate(float3(r, g, b));
}

float3 HSVToLinearRGB(in float3 hsv) {
	// Get pure hue rgb value, then apply saturation and illuminance.
	float3 rgb = HueToLinearRGB(hsv.x);
	// (rbg - 1) * hsv.y + 1: lerp(1, rgb, saturation)
	return ((rgb - 1) * hsv.y + 1) * hsv.z;
}

// Hue-Chroma-Value space
// chroma = max(r, g, b) - min(r, g, b);
// saturation = chroma / max(r, g, b); 
// Notice that chroma is non-normalized saturation.
float3 RGBToHCV(in float3 rgb) {
	float4 p = (rgb.g < rgb.b) ? float4(rgb.bg, -1.0f, 2.0f / 3.0f) : float4(rgb.gb, 0.0f, -1.0f / 3.0f);
	float4 q = (rgb.r < p.x) ? float4(p.xyw, rgb.r) : float4(rgb.r, p.yzx);
	float chroma = q.x - min(q.w, q.y);
	float hue = abs((q.w - q.y) / (6.0f * chroma + 1e-10f) + q.z);
	return float3(hue, chroma, q.x);
}

float3 LinearRGBToHSV(in float3 rgb) {
	float3 hcv = RGBToHCV(rgb);
	float s = hcv.y / (hcv.z + 1e-10f);
	return float3(hcv.x, s, hcv.z);
}

#endif