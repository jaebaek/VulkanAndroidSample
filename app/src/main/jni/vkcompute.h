// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, "VkCompute ",              \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

#define CALL_VK2(func, ...)                                           \
  if (!(void *)func) {                                                \
    __android_log_print(ANDROID_LOG_ERROR, "VkCompute ",              \
                        "Vulkan error. File[%s], line[%d], func=%s",  \
                        __FILE__, __LINE__, #func);                   \
    assert(false);                                                    \
  } else if (VK_SUCCESS != (func(__VA_ARGS__))) {                     \
    __android_log_print(ANDROID_LOG_ERROR, "VkCompute ",              \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

void Run();
void Destroy();
void Test();
