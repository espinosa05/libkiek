#include <kiek/kiek_vulkan_backend.h>
#include <core/wm_vulkan.h>
#include <core/memory.h>
#include <core/types.h>
#include <core/utils.h>
#include <core/log.h>
#include <core/strings.h>

#define KIEK_ENGINE_ID_STRING "KIEK!"

#define KIEK_TRACE(...) F_LOG_T(OS_STDERR, "KIEK_TRACE", ANSI_COLOR_YELLOW, __VA_ARGS__)

#define VULKAN_SETUP_CHECK(call)                                                        \
    MACRO_START                                                                         \
        VkResult vk_rs = call;                                                          \
        if (vk_rs != VK_SUCCESS) {                                                      \
            F_LOG_T(OS_STDERR, "FATAL", ANSI_COLOR_RED, "failed to setup renderer:\n"   \
                                                        #call " failed with error %s",  \
                                                        string_VkResult(vk_rs));        \
            ABORT();                                                                    \
        }                                                                               \
    MACRO_END

/* static function declaration start */
static void set_application_version_header(struct kiek_app_version_header *version_header, struct kiek_app_version_header *user_arg);
static void get_present_instance_extension_properties(struct m_array *present_extension_array);
static b32 required_instance_extensions_present(const struct wm_vulkan_extensions *required_extension_names);
/* static function declaration end */

void kiek_vulkan_startup(struct kiek_vulkan_context *kvk, const struct kiek_vulkan_context_info kvk_info)
{
    CHECK_NULL(kvk_info.extensions);

    /* set application info */
    struct kiek_app_version_header version_header = {0};
    set_application_version_header(&version_header, kvk_info.version);
    VkApplicationInfo app_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = kvk_info.app_name,
        .applicationVersion = VK_MAKE_VERSION(version_header.major, version_header.minor, version_header.patch),
        .pEngineName        = KIEK_ENGINE_ID_STRING,
        .engineVersion      = VK_MAKE_VERSION(KIEK_ENGINE_VERSION_MAJOR, KIEK_ENGINE_VERSION_MINOR, KIEK_ENGINE_VERSION_PATCH),
    };

    /* initialize the Vulkan API */
    ASSERT_RT(required_instance_extensions_present(kvk_info.extensions), "required vulkan extensions not available");
    VkInstanceCreateInfo instance_info = {
        .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo           = &app_info,
        .enabledExtensionCount      = kvk_info.extensions->count,
        .ppEnabledExtensionNames    = kvk_info.extensions->names,
    };
    VULKAN_SETUP_CHECK(vkCreateInstance(&instance_info, NULL, &kvk->instance));
}

VkInstance kiek_vulkan_get_instance(const struct kiek_vulkan_context kvk)
{
    return kvk.instance;
}

void kiek_vulkan_shutdown(struct kiek_vulkan_context *kvk)
{
    KIEK_TRACE("shutting down...");
    vkDestroyInstance(kvk->instance, NULL);
}

#ifndef KIEK_APP_VERISON_MAJOR
#   define KIEK_APP_VERSION_MAJOR  0
#endif /* KIEK_APP_VERSION_MAJOR */
#ifndef KIEK_APP_VERSION_MINOR
#   define KIEK_APP_VERSION_MINOR  0
#endif /* KIEK_APP_VERSION_MINOR */
#ifndef KIEK_APP_VERSION_PATCH
#   define KIEK_APP_VERSION_PATCH  1
#endif /* KIEK_APP_VERSION_PATCH */

#define KIEK_APPLICATION_VERSION_HEADER_DEFAULT (struct kiek_app_version_header) { KIEK_APP_VERSION_MAJOR, KIEK_APP_VERSION_MINOR, KIEK_APP_VERSION_PATCH }

static void set_application_version_header(struct kiek_app_version_header *version_header, struct kiek_app_version_header *user_arg)
{
    *version_header = KIEK_APPLICATION_VERSION_HEADER_DEFAULT;
    if (!user_arg) {
        KIEK_TRACE("No"STR_QUOT("KIEK!")"application version passed! falling back to default Testing version...");
        *version_header = *user_arg;
    }

    KIEK_TRACE(STR_NL"KIEK!-Vulkan-App Information"STR_NL
               STR_TAB"Kiek-Version-Major"STR_TAB": "USZ_FMT STR_NL
               STR_TAB"Kiek-Version-Minor"STR_TAB": "USZ_FMT STR_NL
               STR_TAB"Kiek-Version-Patch"STR_TAB": "USZ_FMT STR_NL,
               version_header->major,
               version_header->minor,
               version_header->patch);
}

#define GET_MAJOR_VERSION(ver)  (((u32)(ver)<<22U)&0xFF)
#define GET_MINOR_VERSION(ver)  (((u32)(ver)<<12U)&0xFF)
#define GET_PATCH(ver)          (ver&0xFF)

static void get_present_instance_extension_properties(struct m_array *present_extension_array)
{
    u32 count = 0;
    VULKAN_SETUP_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &count, NULL));
    m_array_init(present_extension_array, sizeof(VkExtensionProperties), count);
    VULKAN_SETUP_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &count, present_extension_array->data));
    /* hacky */
    present_extension_array->count = count;
}

static void print_instance_extension_property_names(struct m_array present_extension_array)
{
    VkExtensionProperties *present_extensions = present_extension_array.data;
    KIEK_TRACE("present extensions:");
    for (usz i = 0; i < present_extension_array.count; ++i) {
        LOG("\t["USZ_N_FMT(2)"] "STR_FMT"\n", i, present_extensions[i].extensionName);
    }
}

static b32 required_instance_extensions_present(const struct wm_vulkan_extensions *required_extensions)
{
    struct m_array present_extension_array = {0};
    get_present_instance_extension_properties(&present_extension_array);
    print_instance_extension_property_names(present_extension_array);

    VkExtensionProperties *present_extensions = present_extension_array.data;
    b32 required_extensions_present = FALSE;

    for (u32 i = 0; i < required_extensions->count; ++i) {
        required_extensions_present = FALSE;
        for (u32 j = 0; j < present_extension_array.count; ++j) {
            if (cstr_compare(present_extensions[j].extensionName, required_extensions->names[i])) {
                required_extensions_present = TRUE;
                break;
            }
        }
        if (!required_extensions_present) {
            break;
        }
    }

    /* free vulkan allocated data */
    m_free(present_extensions);

    return required_extensions_present;
}


