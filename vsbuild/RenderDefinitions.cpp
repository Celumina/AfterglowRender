#include "RenderDefinitions.h"
#include <stdexcept>

std::string render::HLSLTexturePixelTypeName(InputAttachmentType inputAttachment) {
    switch (inputAttachment) {
        case InputAttachmentType::Color:
            return "float4";
        case InputAttachmentType::Depth:
            return "float";
        case InputAttachmentType::DepthStencil:
            return "float2"; // TODO: Not supported yet.
    }
    throw std::runtime_error("inputAttachment type name is not defined.");
}
