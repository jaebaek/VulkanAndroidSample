#include <algorithm>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <unistd.h>
#include <vector>

#include "log.h"
#include "vkcompute.h"
#include "vulkan_wrapper.h"

//------------------------------------------

static std::vector<VkLayerProperties> layerProperties;

void checkLayer() {
  uint32_t count;

  CALL_VK(vkEnumerateInstanceLayerProperties(&count, NULL));
  layerProperties.resize(count);
  CALL_VK(vkEnumerateInstanceLayerProperties(&count, layerProperties.data()));

  LOGI("%u layers exists", count);

#if 0
  for (int i = 0; i < count; i++) {
    LOGI("%d:\n%s", i, layerProperties[i].layerName);
    LOGI("%u", layerProperties[i].specVersion);
    LOGI("%u", layerProperties[i].implementationVersion);
    LOGI("%s", layerProperties[i].description);
    LOGI("");

    uint32_t countExt;
    std::vector<VkExtensionProperties> extProperties;

    CALL_VK(vkEnumerateInstanceExtensionProperties(layerProperties[i].layerName,
                                                   &countExt,
                                                   NULL));
    extProperties.resize(count);
    CALL_VK(vkEnumerateInstanceExtensionProperties(layerProperties[i].layerName,
                                                   &countExt,
                                                   extProperties.data()));
    for (int j = 0; j < countExt; j++) {
      LOGI("\t%d:\n%s", j, extProperties[j].extensionName);
      LOGI("\t%u", extProperties[j].specVersion);
      LOGI("");
    }
  }
#endif
}

//------------------------------------------

static VkInstance instance;

const static char *enabledExtension[] = {
  "VK_EXT_debug_report"
};

VkDebugReportCallbackEXT callback;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
                                                    VkDebugReportObjectTypeEXT
                                                    objType,
                                                    uint64_t obj,
                                                    size_t location,
                                                    int32_t code,
                                                    const char* layerPrefix,
                                                    const char* msg,
                                                    void* userData) {
  LOGE("validation layer (%s): %s", layerPrefix, msg);

  return VK_FALSE;
}

void createInstance() {
  std::vector<const char *> requiredProperties;
  for (int i = 0; i < layerProperties.size(); i++) {
    LOGI("%s", layerProperties[i].layerName);
    requiredProperties.push_back(layerProperties[i].layerName);
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
  VkResult result;

  VkInstanceCreateInfo instInfo = {};
  instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instInfo.pApplicationInfo = &appInfo;
  instInfo.enabledLayerCount = layerProperties.size();
  instInfo.ppEnabledLayerNames = requiredProperties.data();
  instInfo.enabledExtensionCount = 1;
  instInfo.ppEnabledExtensionNames = enabledExtension;

  CALL_VK(vkCreateInstance(&instInfo, NULL, &instance));

  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = debugCallback;

  auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

  CALL_VK(func(instance, &createInfo, nullptr, &callback));
}

//------------------------------------------

static VkPhysicalDevice physicalDevice;

struct QueueFamily {
  VkQueueFlags queue;
  uint32_t index;
};
static std::vector<QueueFamily> requiredQueue;

bool getQueueFamilyIndex(const VkPhysicalDevice& targetPhysicalDevice) {
  uint32_t count;
  std::vector<VkQueueFamilyProperties> properties;

  vkGetPhysicalDeviceQueueFamilyProperties(targetPhysicalDevice, &count, NULL);

  properties.resize(count);
  vkGetPhysicalDeviceQueueFamilyProperties(targetPhysicalDevice,
                                           &count,
                                           properties.data());

  requiredQueue.clear();
  for (uint32_t i = 0;i < count; ++i) {
    if (properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      QueueFamily q = {properties[i].queueFlags, i};
      requiredQueue.push_back(q);
      return true;
    }
  }

  return false;
}

void createPhysicalDevice() {
  uint32_t count;
  std::vector<VkPhysicalDevice> physicalDevices;
  VkResult result;

  physicalDevice = 0;

  CALL_VK(vkEnumeratePhysicalDevices(instance, &count, NULL));

  physicalDevices.resize(count);
  CALL_VK(vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data()));

  for (uint32_t i = 0;i < count; ++i) {
    VkPhysicalDeviceProperties property;
    vkGetPhysicalDeviceProperties(physicalDevices[i], &property);

    if (getQueueFamilyIndex(physicalDevices[i])) {
      physicalDevice = physicalDevices[i];
      return;
    }
  }

  LOGW("No device supports both VK_QUEUE_COMPUTE_BIT");
}

//------------------------------------------

static VkDevice device;

void createDevice() {
  std::vector<VkDeviceQueueCreateInfo> queueInfo;
  queueInfo.resize(requiredQueue.size());

  const float priorities[] = { 1.0f };

  for (int i = 0; i < requiredQueue.size(); ++i) {
    queueInfo[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo[i].queueFamilyIndex = requiredQueue[i].index;
    queueInfo[i].queueCount = 1;
    queueInfo[i].pQueuePriorities = priorities;
  }

  VkDeviceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  info.pQueueCreateInfos = queueInfo.data();
  info.queueCreateInfoCount = queueInfo.size();

  CALL_VK(vkCreateDevice(physicalDevice, &info, NULL, &device));
}

//------------------------------------------

static std::vector<VkQueue> queue;

void createQueue() {
  for (int i = 0; i < requiredQueue.size(); ++i) {
    VkQueue q;
    vkGetDeviceQueue(device, requiredQueue[i].index, 0, &q);
    queue.push_back(q);
  }
}

//------------------------------------------

static std::vector<VkCommandPool> cmdPool;

void createCommandPool() {
  for (int i = 0; i < requiredQueue.size(); ++i) {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // Check this flags.
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = requiredQueue[i].index;

    VkCommandPool pool;
    CALL_VK(vkCreateCommandPool(device, &info, NULL, &pool));

    cmdPool.push_back(pool);
  }
}

//------------------------------------------

static std::vector<VkCommandBuffer> commandBuffer;

void allocateCommandBuffer() {
  for (int i = 0; i < cmdPool.size(); ++i) {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = cmdPool[i];
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    VkCommandBuffer cmdBuf;
    CALL_VK(vkAllocateCommandBuffers(device, &info, &cmdBuf));

    commandBuffer.push_back(cmdBuf);
  }
}

//------------------------------------------

void recordCommandBuffer() {
  for (int i = 0; i < commandBuffer.size(); ++i) {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CALL_VK(vkBeginCommandBuffer(commandBuffer[i], &info));
  }
}

// NOTE: End recording of command buffer. If the application wishes to further
// use the command buffer, the command buffer must be reset.
void endCommandBufferRecording() {
  for (int i = 0; i < commandBuffer.size(); ++i) {
    CALL_VK(vkEndCommandBuffer(commandBuffer[i]));
  }
}

//------------------------------------------

#include "spirv.inc"
static VkShaderModule module;

void createShaderModule() {
  VkShaderModuleCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = sizeof(spirv);
  info.pCode = (const uint32_t*) spirv;

  CALL_VK(vkCreateShaderModule(device, &info, NULL, &module));
}

//------------------------------------------

static VkDescriptorSetLayout descSetLayout;

void createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding binding[2];
  binding[0].binding = 0;
  binding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  binding[0].descriptorCount = 1;
  binding[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  // XXX: binding[0].pImmutableSamplers = ...
  binding[0].pImmutableSamplers = NULL;

  binding[1].binding = 1;
  binding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  binding[1].descriptorCount = 1;
  binding[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  binding[1].pImmutableSamplers = NULL;

  VkDescriptorSetLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = NELEMS(binding);
  info.pBindings = binding;

  CALL_VK(vkCreateDescriptorSetLayout(device, &info, NULL, &descSetLayout));
}

static VkPipelineLayout layout;

void createPipelineLayout() {
  VkPipelineLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.setLayoutCount = 1;
  info.pSetLayouts = &descSetLayout;

  CALL_VK(vkCreatePipelineLayout(device, &info, NULL, &layout));
}

static VkDescriptorPool descPool;

void createDescriptorPool() {
  VkDescriptorPoolSize size = {};
  size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  size.descriptorCount = 2;

  VkDescriptorPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  info.maxSets = 2;
  info.poolSizeCount = 1;
  info.pPoolSizes = &size;

  CALL_VK(vkCreateDescriptorPool(device, &info, NULL, &descPool));
}

static VkDescriptorSet descSet;

void createDescriptorSet() {
  VkDescriptorSetAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.descriptorPool = descPool;
  info.descriptorSetCount = 1;
  info.pSetLayouts = &descSetLayout;

  CALL_VK(vkAllocateDescriptorSets(device, &info, &descSet));
}

//------------------------------------------

static VkPipeline pipeline;

void createComputePipeline() {
  VkPipelineShaderStageCreateInfo stage = {};
  stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stage.module = module;
  stage.pName = "foo";
  // XXX: stage.pSpecializationInfo = ...

  VkComputePipelineCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  info.stage = stage;
  info.layout = layout;

  CALL_VK(vkCreateComputePipelines(device,
                                   VK_NULL_HANDLE,
                                   1,
                                   &info,
                                   NULL,
                                   &pipeline));
}

void bindPipeline() {
  for (int i = 0;i < commandBuffer.size(); ++i) {
    vkCmdBindPipeline(commandBuffer[i],
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipeline);
  }
}

//------------------------------------------

static const uint32_t bufferSize = 64;
static VkBuffer buffer[2];
static uint32_t requiredMemoryType;
static uint32_t requiredMemorySize;
static uint32_t requiredMemoryAlignment;

void createBuffer() {
  VkBufferCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  // info.flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
  info.size = bufferSize;
  info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.queueFamilyIndexCount = requiredQueue.size();

  std::vector<uint32_t> indices;
  std::for_each(requiredQueue.begin(),
                requiredQueue.end(),
                [&indices](const QueueFamily& qf) {
                  indices.push_back(qf.index);
                });
  info.pQueueFamilyIndices = indices.data();

  CALL_VK(vkCreateBuffer(device, &info, NULL, &buffer[0]));
  CALL_VK(vkCreateBuffer(device, &info, NULL, &buffer[1]));

  VkMemoryRequirements requirement;
  vkGetBufferMemoryRequirements(device, buffer[0], &requirement);

  requiredMemoryType = requirement.memoryTypeBits |
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  requiredMemorySize = requirement.size;
  requiredMemoryAlignment = requirement.alignment;
}

static VkPhysicalDeviceMemoryProperties memoryProperty;
static uint32_t memoryIndex;

void exploreMemoryProperty() {
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperty);

  memoryIndex = memoryProperty.memoryTypeCount;
  for (uint32_t i = 0;i < memoryProperty.memoryTypeCount;++i)
    if ((memoryProperty.memoryTypes[i].propertyFlags & requiredMemoryType) ==
        requiredMemoryType) {
      memoryIndex = i;
      break;
    }
  if (memoryIndex == memoryProperty.memoryTypeCount)
    LOGW("No required memory type exists.");
}

static VkDeviceMemory deviceMemory[2];
static void *ptrMemory[2];

void allocateAndMapMemory() {
  // Allocate
  VkMemoryAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  info.allocationSize = std::max(bufferSize, requiredMemorySize);
  info.memoryTypeIndex = memoryIndex;

  CALL_VK(vkAllocateMemory(device, &info, NULL, &deviceMemory[0]));
  CALL_VK(vkAllocateMemory(device, &info, NULL, &deviceMemory[1]));

  // Bind
  CALL_VK(vkBindBufferMemory(device, buffer[0], deviceMemory[0], 0));
  CALL_VK(vkBindBufferMemory(device, buffer[1], deviceMemory[1], 0));

  // Map
  CALL_VK(vkMapMemory(device, deviceMemory[0], 0, bufferSize, 0, &ptrMemory[0]));
  CALL_VK(vkMapMemory(device, deviceMemory[1], 0, bufferSize, 0, &ptrMemory[1]));
}

// NOTE:
// Updating descriptor sets must be done before updating resources (i.e., buffer
// , image) and after binding the resources. Therefore, the valid order of
// memory setting must be
//
//    1. Allocate resources
//    2. Bind resources
//    3. Update corresponding descriptor sets
//    4. Update resources via the mapped host memory
//
// The order of mapping resources to the host memory and descriptor set
// creations does not matter.
void updateDescriptorSet() {
  VkDescriptorBufferInfo info[2] = {};
  info[0].buffer = buffer[0];
  info[0].offset = 0;
  info[0].range = VK_WHOLE_SIZE;

  VkWriteDescriptorSet write[2] = {};
  write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write[0].dstSet = descSet;
  write[0].dstBinding = 0;
  write[0].dstArrayElement = 0;
  write[0].descriptorCount = 1;
  write[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write[0].pBufferInfo = &info[0];

  info[1].buffer = buffer[1];
  info[1].offset = 0;
  info[1].range = VK_WHOLE_SIZE;

  write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write[1].dstSet = descSet;
  write[1].dstBinding = 1;
  write[1].dstArrayElement = 0;
  write[1].descriptorCount = 1;
  write[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write[1].pBufferInfo = &info[1];

  vkUpdateDescriptorSets(device, NELEMS(write), write, 0, NULL);
}

void memoryInit() {
  // Fill something to ptrMemory
  for (int i = 0;i < bufferSize/sizeof(float); ++i)
    ((float *)ptrMemory[0])[i] = (float) i;

  // Fill something to ptrMemory
  for (int i = 0;i < bufferSize/sizeof(float); ++i)
    ((float *)ptrMemory[1])[i] = (float) i + 100.0;
}

//------------------------------------------

void bindDescSet() {
  for (int i = 0; i < commandBuffer.size(); ++i) {
    vkCmdBindDescriptorSets(commandBuffer[i],
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            layout,
                            0,
                            1,
                            &descSet,
                            0,
                            NULL);
  }
}

void dispatch() {
  for (int i = 0;i < commandBuffer.size(); ++i)
    vkCmdDispatch(commandBuffer[i],
                  bufferSize / (4 * sizeof(float)),
                  4,
                  1);
}

void submit() {
  for (int i = 0; i < queue.size(); ++i) {
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &commandBuffer[i];

    CALL_VK(vkQueueSubmit(queue[i], 1, &info, VK_NULL_HANDLE));
  }
}

void wait() {
  for (int i = 0; i < queue.size(); ++i)
    CALL_VK(vkQueueWaitIdle(queue[i]));
}

void printMemory() {
  for (int i = 0;i < bufferSize/sizeof(float); ++i)
    LOGI("memory[0][%d]: %f", i, ((float *)ptrMemory[0])[i]);
  for (int i = 0;i < bufferSize/sizeof(float); ++i)
    LOGI("memory[1][%d]: %f", i, ((float *)ptrMemory[1])[i]);
}

void unmapMemory() {
  vkUnmapMemory(device, deviceMemory[0]);
  vkUnmapMemory(device, deviceMemory[1]);
}

//------------------------------------------

void Destroy() {
  CALL_VK(vkFreeDescriptorSets(device, descPool, 1, &descSet));
  vkDestroyDescriptorPool(device, descPool, NULL);
  vkDestroyDescriptorSetLayout(device, descSetLayout, NULL);
  vkDestroyBuffer(device, buffer[0], NULL);
  vkDestroyBuffer(device, buffer[1], NULL);
  vkFreeMemory(device, deviceMemory[0], NULL);
  vkFreeMemory(device, deviceMemory[1], NULL);

  vkDestroyPipelineLayout(device, layout, NULL);
  vkDestroyPipeline(device, pipeline, NULL);
  vkDestroyShaderModule(device, module, NULL);
  for (int i = 0; i < commandBuffer.size(); ++i) {
    vkFreeCommandBuffers(device, cmdPool[i], 1, &commandBuffer[i]);
  }
  for (int i = 0; i < cmdPool.size(); ++i) {
    vkDestroyCommandPool(device, cmdPool[i], nullptr);
  }
  vkDestroyDevice(device, NULL);

  auto func = (PFN_vkDestroyDebugReportCallbackEXT)
      vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
  func(instance, callback, NULL);

  vkDestroyInstance(instance, NULL);
}

//------------------------------------------

void Run() {
  checkLayer();
  createInstance();
  createPhysicalDevice();
  createDevice();
  createQueue();
  createCommandPool();

  // Command buffer life cycle
  allocateCommandBuffer();

  // Descriptor set
  createDescriptorPool();
  createDescriptorSetLayout();
  createPipelineLayout();

  // Pipeline build
  createShaderModule();
  createComputePipeline();

  createDescriptorSet();

  // Memory allocation
  createBuffer();
  exploreMemoryProperty();
  allocateAndMapMemory();

  updateDescriptorSet(); // NOTE: updating descriptor set must be done before
                         //       updating resources
  memoryInit();

  recordCommandBuffer();
  bindDescSet();
  bindPipeline();
  dispatch();

  endCommandBufferRecording();
  submit();
  wait();

  printMemory();
  unmapMemory();

  LOGI("VkCompute Success");
}
