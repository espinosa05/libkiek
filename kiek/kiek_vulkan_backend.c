#include <kiek/kiek_vulkan_backend.h>
#include <core/os_vulkan.h>
#include <core/memory.h>
#include <core/types.h>
#include <core/utils.h>
#include <core/log.h>

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
static void SetApplicationVersionHeader(Kiek_ApplicationVersionHeader *versionHeader, Kiek_ApplicationVersionHeader *userArg);
static void GetRequiredInstanceExtensionNames(M_Array *requiredExtensionNames);
static void GetPresentInstanceExtensionProperties(M_Array *presentExtensionArray);
static b32  RequiredInstanceExtensionsPresent(const M_Array requiredExtensionNameArray);
/* static function declaration end */

void kiek_vulkan_startup(struct kiek_vulkan_context *kvk, const struct kiek_vulkan_context_info kvk_info)
{
    /* set application info */
    struct kiek_application_version_header version_header = {0};
    set_application_version_header(&version_header, kvk_info.app_version_header);
    VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = kvk_info.app_name,
        .applicationVersion = VK_MAKE_VERSION(version_header.major, version_header.minor, version_header.patch),
        .pEngineName        = KIEK_ENGINE_ID_STRING,
        .engineVersion      = VK_MAKE_VERSION(KIEK_ENGINE_VERSION_MAJOR, KIEK_ENGINE_VERSION_MINOR, KIEK_ENGINE_VERSION_PATCH),
    };

    /* initialize the Vulkan API */
    struct m_array instance_extensions = {0};
    get_required_instance_extension_names(&instance_extensions);
    ASSERT_RT(required_instance_extensions_present(instance_extensions), "required vulkan extensions not available");
    VkInstanceCreateInfo instanceInfo = {
        .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo           = &app_info,
        .enabledExtensionCount      = instance_extensions.count,
        .ppEnabledExtensionNames    = instance_extensions.data,
    };
    VULKAN_SETUP_CHECK(vkCreateInstance(&instance_info, NULL, &kvk->instance));
    m_array_delete(instance_extensions);
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

#define KIEK_APPLICATION_VERSION_HEADER_DEFAULT (Kiek_ApplicationVersionHeader) { KIEK_APP_VERSION_MAJOR, KIEK_APP_VERSION_MINOR, KIEK_APP_VERSION_PATCH }

static void set_application_version_header(struct kiek_application_version_header *version_header, struct kiek_application_version_header *user_arg)
{
    *versionHeader = KIEK_APPLICATION_VERSION_HEADER_DEFAULT;
    if (!user_arg) {
        KIEK_TRACE("No \"KIEK!\" application version passed! falling back to default Testing version...");
        *version_header = *user_arg;
    }

    KIEK_TRACE("\nKIEK!-Vulkan-App Information\n"
               "\tKiek-Version-Major\t:%d\n"
               "\tKiek-Version-Minor\t:%d\n"
               "\tKiek-Version-Patch\t:%d\n",
               version_header->major,
               version_header->minor,
               version_header->patch);
}

static void get_required_instance_extension_names(struct m_array *required_extension_names)
{
    struct os_wm_extensions wm_extensions = {0};
    os_wm_get_required_extensions(&wm_extensions);

    struct m_array_info required_extension_names_info = {
        .width  = sizeof(*wm_extensions.names),
        .base   = wm_extensions.names,
        .cap    = wm_extensions.count,
        .count  = wm_extensions.count,
    };
    m_array_init_ext(required_extension_names, required_extension_names_info);
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

static void PrintInstanceExtensionPropertyNames(M_Array presentExtensionArray)
{
    VkExtensionProperties *presentExtensions = presentExtensionArray.data;
    KIEK_TRACE("present extensions:");
    for (usz i = 0; i < presentExtensionArray.count; ++i) {
        LOG("\t[%02d] %s\n", i, presentExtensions[i].extensionName);
    }
}

static b32 RequiredInstanceExtensionsPresent(const M_Array requiredExtensionNameArray)
{
    ASSERT(requiredExtensionNameArray.count,    "array of size 0 passed!!");
    ASSERT(requiredExtensionNameArray.data,     "array pointing to NULL passed!!");

    M_Array presentExtensionArray = {0};
    GetPresentInstanceExtensionProperties(&presentExtensionArray);
    PrintInstanceExtensionPropertyNames(presentExtensionArray);

    char **requiredExtensionNames = requiredExtensionNameArray.data;
    VkExtensionProperties *presentExtensions = presentExtensionArray.data;

    b32 requiredExtensionsPresent = FALSE;
    for (u32 i = 0; i < requiredExtensionNameArray.count; ++i) {
        requiredExtensionsPresent = FALSE;
        for (u32 j = 0; j < presentExtensionArray.count; ++j) {
            if (CStr_Compare(presentExtensions[j].extensionName, requiredExtensionNames[i])) {
                requiredExtensionsPresent = TRUE;
                break;
            }
        }
        if (!requiredExtensionsPresent) {
            break;
        }
    }

    M_Free(presentExtensions);
    return requiredExtensionsPresent;
}


