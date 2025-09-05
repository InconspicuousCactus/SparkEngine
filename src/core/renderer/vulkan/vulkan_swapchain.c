#include "Spark/renderer/vulkan/vulkan_swapchain.h"
#include "Spark/renderer/texture.h"
#include "Spark/renderer/vulkan/vulkan_context.h"
#include "Spark/renderer/vulkan/vulkan_texture.h"
#include "Spark/renderer/vulkan/vulkan_utils.h"
#include "Spark/core/smemory.h"
#include <vulkan/vulkan_core.h>

// ============================
// Private functions
// ============================
void select_surface_format(vulkan_context_t* context, VkSurfaceFormatKHR* out_format);
VkPresentModeKHR select_present_mode(vulkan_context_t* context);
void create_depth_buffers(vulkan_context_t* context, vulkan_swapchain_t* swapchain);

void vulkan_swapchain_create(struct vulkan_context* context, vulkan_swapchain_t* out_swapchain) {
    VkSurfaceFormatKHR surface_format; 
    select_surface_format(context, &surface_format);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device.handle, context->surface, &surface_capabilities));

    u32 image_count = surface_capabilities.minImageCount + 1;
    if (image_count > surface_capabilities.maxImageCount && surface_capabilities.maxImageCount != 0) {
        image_count = surface_capabilities.maxImageCount;
    }

    out_swapchain->image_count = image_count;
    out_swapchain->image_format = surface_format.format;
    context->screen_width = surface_capabilities.currentExtent.width;
    context->screen_height = surface_capabilities.currentExtent.height;

    SASSERT(image_count <= SWAPCHAIN_MAX_IMAGE_COUNT, "Trying to create more images in swapchain than max image count.");

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .flags = 0,
        .surface = context->surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = surface_capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &context->graphics_queue.family_index,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = select_present_mode(context),
        .clipped = VK_TRUE,
    };

    VK_CHECK(vkCreateSwapchainKHR(context->logical_device, &create_info, context->allocator, &out_swapchain->handle));

    u32 swapchain_image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context->logical_device, out_swapchain->handle, &swapchain_image_count, NULL));
    SASSERT(swapchain_image_count == image_count, "Failed to correctly set the number of swapchain images. Expected %d, got %d", image_count, swapchain_image_count);

    VK_CHECK(vkGetSwapchainImagesKHR(context->logical_device, out_swapchain->handle, &swapchain_image_count, out_swapchain->images));

    for (u32 i = 0; i < image_count; i++) {
        vulkan_image_view_create(context, 
                out_swapchain->images[i], 
                VK_IMAGE_VIEW_TYPE_2D, 
                surface_format.format,
                VK_IMAGE_ASPECT_COLOR_BIT, 
                1, 
                1, 
                &out_swapchain->image_views[i]);
    }

    create_depth_buffers(context, out_swapchain);
}

void vulkan_swapchain_destroy(struct vulkan_context* context, vulkan_swapchain_t* swapchain) {
    for (u32 i = 0; i < swapchain->image_count; i++) {
        vkDestroyImageView(context->logical_device, swapchain->image_views[i], context->allocator);
        vulkan_image_destroy(context, &swapchain->depth_buffers[i]);
    }

    vkDestroySwapchainKHR(context->logical_device, swapchain->handle, context->allocator);
}

void vulkan_swapchain_recreate(vulkan_context_t* context, vulkan_swapchain_t* swapchain) {
    vulkan_swapchain_destroy(context, swapchain);
    vulkan_swapchain_create(context, swapchain);
}

void select_surface_format(vulkan_context_t* context, VkSurfaceFormatKHR* out_format) {
    u32 format_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device.handle, context->surface, &format_count, NULL));

#define MAX_SURFACE_FORMAT_COUNT 64
    VkSurfaceFormatKHR formats[MAX_SURFACE_FORMAT_COUNT];
    if (format_count > MAX_SURFACE_FORMAT_COUNT) {
        SWARN("Device supports more surface formats than able to store. Got %d, Max: %d", format_count, MAX_SURFACE_FORMAT_COUNT);
        format_count = MAX_SURFACE_FORMAT_COUNT;
    }
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device.handle, context->surface, &format_count, formats));

    *out_format = formats[0];
    for (u32 i = 0; i < format_count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            *out_format = formats[i];
        }
    }
}

VkPresentModeKHR select_present_mode(vulkan_context_t* context) {
    u32 present_mode_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device.handle, context->surface, &present_mode_count, NULL));

    VkPresentModeKHR* present_modes = sallocate(sizeof(VkSurfaceFormatKHR) * present_mode_count, MEMORY_TAG_ARRAY);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device.handle, context->surface, &present_mode_count, present_modes));

    for (u32 i = 0; i < present_mode_count; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            sfree(present_modes, sizeof(VkSurfaceFormatKHR) * present_mode_count, MEMORY_TAG_ARRAY);
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    sfree(present_modes, sizeof(VkSurfaceFormatKHR) * present_mode_count, MEMORY_TAG_ARRAY);

    return VK_PRESENT_MODE_FIFO_KHR;
}

void create_depth_buffers(vulkan_context_t* context, vulkan_swapchain_t* swapchain) {
    for (u32 i = 0; i < swapchain->image_count; i++) {
        vulkan_image_create(context, 
                TEXTURE_FILTER_NEAREST,
                context->physical_device.depth_format, 
                context->screen_width,
                context->screen_height,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                &swapchain->depth_buffers[i]);

        vulkan_image_transition_layout(context, 
                &swapchain->depth_buffers[i], 
                VK_IMAGE_LAYOUT_UNDEFINED, 
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
                VK_QUEUE_FAMILY_IGNORED, 
                VK_QUEUE_FAMILY_IGNORED, 
                &context->copy_to_graphics_command_buffer,
                &context->graphics_queue);
    }
}
