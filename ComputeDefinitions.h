#pragma once
#include <stdint.h>

namespace compute {
	enum class SSBOAccessMode {
		Undefined, 
		ReadOnly,
		ReadWrite
	};

	enum class SSBOInitMode {
		Zero, 
		StructuredData,       // TODO: Custom structured data by definition.
		Model,                // TODO
		Texture,              // TODO
		InternalFunction,     // Temporary solution.
		ComputeShader         // Init SSBO from another compute shader.
	};

	enum class SSBOUsage {
		Buffer,
		IndexInput,
		VertexInput,
		Instancing
	};

	struct DispatchGroup {
		uint32_t x = 1;
		uint32_t y = 1;
		uint32_t z = 1;
	};

}