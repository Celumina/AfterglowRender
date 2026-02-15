#include "RenderDefinitions.h"
#include "ExceptionUtilities.h"

std::string render::HLSLTexturePixelTypeName(AttachmentType inputAttachment) {
    switch (inputAttachment) {
        case AttachmentType::Color:
            return "float4";
        case AttachmentType::Depth:
            return "float";
        case AttachmentType::DepthStencil:
            return "float2"; // TODO: Not supported yet.
    }
    EXCEPT_RUNTIME("inputAttachment type name is not defined.");
}
