#include "AfterglowUserInterfacePass.h"
#include "SwapchainConfigurations.h"

AfterglowUserInterfacePass::AfterglowUserInterfacePass(AfterglowDevice& device, AfterglowPassInterface* prevPass) :
	AfterglowPassInterface(device, AfterglowPassInterface::ExtentMode::Swapchain, swapchain::presentFormat) {

	AfterglowSubpassContext::PassUsage colorUsage = AfterglowSubpassContext::PassUsage::Export;
	if (prevPass && isValidAttachment(prevPass->exportColorAttachmentIndex(ColorAttachment::Color))) {
		colorUsage = AfterglowSubpassContext::PassUsage::ImportExport;
	}

	// Create attachments (Reuse image if import attachments were exist. )
	uint32_t colorAttachmentIndex = subpassContext().appendAttachment(
		AfterglowSubpassContext::presentAttachment(colorFormat(), colorUsage)
	);

	if (prevPass && isValidAttachment(prevPass->presentAttachmentIndex())) {
		subpassContext().attachment(colorAttachmentIndex).initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	// Pass IO
	recordImportAttachments(prevPass, ColorAttachment::Color, colorAttachmentIndex);
	setExportColorAttachment(ColorAttachment::Color, colorAttachmentIndex, render::sceneColorTextureName, true);

	////////////////////////////////////////
	// UserInterface
	////////////////////////////////////////
	std::string userInterfaceName(inreflect::EnumName(render::Domain::UserInterface));
	subpassContext().appendSubpass(userInterfaceName);

	subpassContext().bindColorAttachment(userInterfaceName, colorAttachmentIndex);

	auto& userInterfaceDependency = subpassContext().makeDependency(
		AfterglowSubpassContext::undefinedSubpassName(), userInterfaceName
	);
}
