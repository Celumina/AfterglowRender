#include "ShaderDefinitions.h"

#include <stdexcept>

#include "AfterglowUtilities.h"

uint32_t shader::AttachmentTextureBindingIndex(uint32_t inputAttachmentLocalIndex) {
	return util::EnumValue(shader::GlobalSetBindingIndex::EnumCount) + inputAttachmentLocalIndex;
}
