#ifndef __KIEK_VULKAN_BACKEND_H__
#define __KIEK_VULKAN_BACKEND_H__

#include <core/vulkan.h>
#include <core/utils.h>
#include <core/log.h>


struct kiek_vulkan_device_data {
    VkPhysicalDeviceProperties          properties;
    VkPhysicalDeviceFeatures            features;
    VkPhysicalDeviceMemoryProperties    memory_properties;
    VkDevice                            handle;
};

struct kiek_vulkan_context {
    /* Vulkan API instance */
    VkInstance                      instance;
    /* Vulkan Device information */
    struct kiek_vulkan_device_data  device;
};

struct kiek_app_version_header {
    u32 major;
    u32 minor;
    u32 patch;
};
#define KIEK_APP_VERSION_HEADER(...)        (struct kiek_app_version_header) {__VA_ARGS__}
#define KIEK_APP_VERSION_HEADER_REF(...)    &KIEK_APP_VERSION_HEADER(__VA_ARGS__)

struct kiek_vulkan_context_info {
    struct kiek_app_version_header  *version;
    /* optional argument! */
    const char                      *app_name;
    struct wm_vulkan_extensions     *extensions;
};

#ifndef KIEK_ENGINE_VERSION_MAJOR
#   define KIEK_ENGINE_VERSION_MAJOR 0
#endif /* KIEK_ENGINE_VERSION_MAJOR */

#ifndef KIEK_ENGINE_VERSION_MINOR
#   define KIEK_ENGINE_VERSION_MINOR 0
#endif /* KIEK_ENGINE_VERSION_MINOR */

#ifndef KIEK_ENGINE_VERSION_PATCH
#   define KIEK_ENGINE_VERSION_PATCH 0
#endif /* KIEK_ENGINE_VERSION_PATCH */

void kiek_vulkan_startup(struct kiek_vulkan_context *kvk, const struct kiek_vulkan_context_info info);
void kiek_vulkan_shutdown(struct kiek_vulkan_context *kvk);
VkInstance kiek_vulkan_get_instance(const struct kiek_vulkan_context kvk);

#endif /* __KIEK_VULKAN_BACKEND_H__ */
