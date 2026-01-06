#ifndef ACES_TONESCALE_HLSL
#define ACES_TONESCALE_HLSL

struct TonescaleParams {
	float n;
	float n_r;
	float g;
	float t_1;
	float c_t; 
	float s_2;
	float u_2;
	float m_2;
	// float forward_limit;
	// float inverse_limit;
	// float log_peak;
};

// @deprecated: Impremented in C++.
// // Tonescale pre-calculations
// TonescaleParams InitTonescaleParams(float peakLuminance) {

// 	// Preset constants that set the desired behavior for the curve
// 	const float n = peakLuminance;
	
// 	static const float n_r = 100.0;	// normalized white in nits (what 1.0 should be)
// 	static const float g = 1.15;	   // surround / contrast
// 	static const float c = 0.18;		// anchor for 18% grey
// 	static const float c_d = 10.013;   // output luminance of 18% grey (in nits)
// 	static const float w_g = 0.14;	 // change in grey between different peak luminance
// 	static const float t_1 = 0.04;	 // shadow toe or flare/glare compensation
// 	static const float r_hit_min = 128.0;	// scene-referred value "hitting the roof" for SDR (e.g. when n = 100 nits)
// 	static const float r_hit_max = 896.0;   // scene-referred value "hitting the roof" for when n = 10000 nits

// 	// Calculate output constants
// 	const float r_hit = r_hit_min + (r_hit_max - r_hit_min) * (log(n / n_r) / log(10000.0 / 100.0));
// 	const float m_0 = (n / n_r);
// 	const float m_1 = 0.5 * (m_0 + sqrt(m_0 * (m_0 + 4.0 * t_1)));
// 	const float u = pow((r_hit / m_1) / ((r_hit / m_1) + 1), g);
// 	const float m = m_1 / u;
// 	const float w_i = log(n / 100.0) / log(2.0);
// 	const float c_t = c_d / n_r * (1.0 + w_i * w_g);
// 	const float g_ip = 0.5 * (c_t + sqrt(c_t * (c_t + 4.0 * t_1)));
// 	const float g_ipp2 = -(m_1 * pow((g_ip / m),(1.0 / g))) / (pow( g_ip / m , 1.0 / g) - 1.0);
// 	const float w_2 = c / g_ipp2;
// 	const float s_2 = w_2 * m_1;
// 	const float u_2 = pow( (r_hit / m_1) / ((r_hit / m_1) + w_2), g);
// 	const float m_2 = m_1 / u_2;
	
// 	TonescaleParams tsParams = { 
// 		n, 
// 		n_r, 
// 		g, 
// 		t_1, 
// 		c_t, 
// 		s_2, 
// 		u_2, 
// 		m_2 
// 	};

// 	return tsParams;
// }

/**
* @param x: scene-referred input (i.e. linear ACES2065-1)
*/
float TonescaleForward(float x) {
	// forward MM tone scale
	float f = ACESOutParams[0].tonescale_m_2 * pow(max(0.0, x) / (x + ACESOutParams[0].tonescale_s_2), ACESOutParams[0].tonescale_g);  	
	float h = max(0.0, f * f / (f + ACESOutParams[0].tonescale_t_1));		 // forward flare 
	return h * ACESOutParams[0].tonescale_n_r;	// output is luminance in cd/m^2
}

#endif