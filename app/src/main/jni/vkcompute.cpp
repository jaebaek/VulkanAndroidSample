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
  // Jaebaek: Copy this code from Android NDK document
  //
  // Make sure the desired instance validation layers are available
  // NOTE:  These are not listed in an arbitrary order.  Threading must be
  //        first, and unique_objects must be last.  This is the order they
  //        will be inserted by the loader.
  const char *instance_layers[] = {
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_LUNARG_device_limits",
    "VK_LAYER_LUNARG_image",
    "VK_LAYER_LUNARG_swapchain",
    "VK_LAYER_GOOGLE_unique_objects"
  };

  std::vector<const char *> requiredProperties;
  for (int k = 0; k < sizeof(instance_layers) / sizeof(char *); ++k) {
    const char *layer = nullptr;
    for (int i = 0; i < layerProperties.size(); i++) {
      if (!strcmp(instance_layers[k], layerProperties[i].layerName)) {
        layer = instance_layers[k];
        break;
      }
    }
    if (layer != nullptr) {
      LOGI("%s", layer);
      requiredProperties.push_back(layer);
    }
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
  VkResult result;

  VkInstanceCreateInfo instInfo = {};
  instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instInfo.pApplicationInfo = &appInfo;
  instInfo.enabledLayerCount = requiredProperties.size();
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

static VkDescriptorSetLayout descSetLayout[2];

void createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding binding[4] = {};
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

  binding[2].binding = 2;
  binding[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  binding[2].descriptorCount = 1;
  binding[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  binding[2].pImmutableSamplers = NULL;

  binding[3].binding = 0;
  binding[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  binding[3].descriptorCount = 1;
  binding[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  binding[3].pImmutableSamplers = NULL;

  VkDescriptorSetLayoutCreateInfo info[2] = {};
  info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info[0].bindingCount = 1;
  info[0].pBindings = &binding[3];
  CALL_VK(vkCreateDescriptorSetLayout(device, &info[0], NULL, &descSetLayout[0]));

  info[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info[1].bindingCount = 3;
  info[1].pBindings = binding;
  CALL_VK(vkCreateDescriptorSetLayout(device, &info[1], NULL, &descSetLayout[1]));
}

static VkPipelineLayout layout;

void createPipelineLayout() {
  VkPipelineLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.setLayoutCount = 2;
  info.pSetLayouts = descSetLayout;

  CALL_VK(vkCreatePipelineLayout(device, &info, NULL, &layout));
}

static VkDescriptorPool descPool[2];

void createDescriptorPool() {
  VkDescriptorPoolSize size[3] = {};
  size[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
  size[0].descriptorCount = 1;

  size[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  size[1].descriptorCount = 2;

  size[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  size[2].descriptorCount = 1;

  VkDescriptorPoolCreateInfo info[2] = {};
  info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info[0].flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  info[0].maxSets = 1;
  info[0].poolSizeCount = 1;
  info[0].pPoolSizes = size;

  info[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info[1].flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  info[1].maxSets = 1;
  info[1].poolSizeCount = 2;
  info[1].pPoolSizes = &size[1];

  CALL_VK(vkCreateDescriptorPool(device, &info[0], NULL, &descPool[0]));
  CALL_VK(vkCreateDescriptorPool(device, &info[1], NULL, &descPool[1]));
}

static VkDescriptorSet descSet[2];

void createDescriptorSet() {
  VkDescriptorSetAllocateInfo info[2] = {};
  info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info[0].descriptorPool = descPool[0];
  info[0].descriptorSetCount = 1;
  info[0].pSetLayouts = &descSetLayout[0];

  info[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info[1].descriptorPool = descPool[1];
  info[1].descriptorSetCount = 1;
  info[1].pSetLayouts = &descSetLayout[1];

  CALL_VK(vkAllocateDescriptorSets(device, &info[0], &descSet[0]));
  CALL_VK(vkAllocateDescriptorSets(device, &info[1], &descSet[1]));
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

VkSampler sampler;
VkImageView imageView;

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
  vkGetBufferMemoryRequirements(device, buffer[1], &requirement);

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

  VkWriteDescriptorSet write[4] = {};
  write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write[0].dstSet = descSet[1];
  write[0].dstBinding = 0;
  write[0].dstArrayElement = 0;
  write[0].descriptorCount = 1;
  write[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write[0].pBufferInfo = &info[0];

  info[1].buffer = buffer[1];
  info[1].offset = 0;
  info[1].range = VK_WHOLE_SIZE;

  write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write[1].dstSet = descSet[1];
  write[1].dstBinding = 1;
  write[1].dstArrayElement = 0;
  write[1].descriptorCount = 1;
  write[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write[1].pBufferInfo = &info[1];

  VkDescriptorImageInfo descImgInfo[2] = {};
  descImgInfo[0].imageView = imageView;
  descImgInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  descImgInfo[1].sampler = sampler;

  write[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write[2].dstSet = descSet[1];
  write[2].dstBinding = 2;
  write[2].dstArrayElement = 0;
  write[2].descriptorCount = 1;
  write[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  write[2].pImageInfo = &descImgInfo[0];

  write[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write[3].dstSet = descSet[0];
  write[3].dstBinding = 0;
  write[3].dstArrayElement = 0;
  write[3].descriptorCount = 1;
  write[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  write[3].pImageInfo = &descImgInfo[1];

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

static VkDeviceMemory imageMemory;
static void *ptrImageMemory;

#define IMAGE_WIDTH     4
#define IMAGE_HEIGHT    4

static VkImage image;

void setImageLayout(VkCommandBuffer cmdBuffer,
                    VkImageLayout oldImageLayout,
                    VkImageLayout newImageLayout,
                    VkPipelineStageFlags srcStages,
                    VkPipelineStageFlags destStages) {
  VkImageMemoryBarrier imageMemoryBarrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = NULL,
      .srcAccessMask = 0,
      .dstAccessMask = 0,
      .oldLayout = oldImageLayout,
      .newLayout = newImageLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

  switch (oldImageLayout) {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      break;

    default:
      break;
  }

  switch (newImageLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      imageMemoryBarrier.dstAccessMask =
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      break;

    default:
      break;
  }

  vkCmdPipelineBarrier(cmdBuffer, srcStages, destStages, 0, 0, NULL, 0, NULL, 1,
                       &imageMemoryBarrier);
}

static const VkFormat imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
#define FORMAT_TYPE float

void createImage() {
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.imageType = VK_IMAGE_TYPE_2D;
  info.format = imageFormat;
  info.extent.width = IMAGE_WIDTH;
  info.extent.height = IMAGE_HEIGHT;
  info.extent.depth = 1;
  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_LINEAR;
  info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  info.queueFamilyIndexCount = requiredQueue.size();

  std::vector<uint32_t> indices;
  std::for_each(requiredQueue.begin(),
                requiredQueue.end(),
                [&indices](const QueueFamily& qf) {
                  indices.push_back(qf.index);
                });
  info.pQueueFamilyIndices = indices.data();
  info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

  vkCreateImage(device, &info, NULL, &image);

  // Memory requirement
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  // Allocate
  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;

  VkMemoryPropertyFlags flag = memRequirements.memoryTypeBits
      | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  uint32_t index = memoryProperty.memoryTypeCount;
  for (uint32_t i = 0;i < memoryProperty.memoryTypeCount;++i) {
    if ((memoryProperty.memoryTypes[i].propertyFlags & flag) == flag) {
      index = i;
      break;
    }
  }
  if (index == memoryProperty.memoryTypeCount)
    LOGW("No required memory type exists.");

  allocInfo.memoryTypeIndex = index;

  CALL_VK(vkAllocateMemory(device, &allocInfo, NULL, &imageMemory));

  // Bind
  CALL_VK(vkBindImageMemory(device, image, imageMemory, 0));

  // Map
  CALL_VK(vkMapMemory(device,
                      imageMemory,
                      0,
                      sizeof(FORMAT_TYPE) * bufferSize,
                      0,
                      &ptrImageMemory));

  // Sampler
  const VkSamplerCreateInfo samplerInfo = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = nullptr,
    .magFilter = VK_FILTER_NEAREST,
    .minFilter = VK_FILTER_NEAREST,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .mipLodBias = 0.0f,
    .maxAnisotropy = 1,
    .compareOp = VK_COMPARE_OP_NEVER,
    .minLod = 0.0f,
    .maxLod = 0.0f,
    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    .unnormalizedCoordinates = VK_TRUE,
  };

  CALL_VK(vkCreateSampler(device, &samplerInfo, nullptr,
                          &sampler));

  // ImageView
  VkImageViewCreateInfo viewInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = nullptr,
    .image = image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = imageFormat,
    .components =
    {
      VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
      VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A,
    },
    .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    .flags = 0,
  };
  CALL_VK(vkCreateImageView(device, &viewInfo, nullptr, &imageView));
}

void initImage() {
  // Init image
  for (int x = 0;x < IMAGE_WIDTH; ++x)
    for (int y = 0;y < IMAGE_HEIGHT; ++y) {
      ((FORMAT_TYPE *)ptrImageMemory)[y * IMAGE_WIDTH * 4 + x * 4] = 3.0f;
      ((FORMAT_TYPE *)ptrImageMemory)[y * IMAGE_WIDTH * 4 + x * 4 + 1] = 0;
      ((FORMAT_TYPE *)ptrImageMemory)[y * IMAGE_WIDTH * 4 + x * 4 + 2] = 0;
      ((FORMAT_TYPE *)ptrImageMemory)[y * IMAGE_WIDTH * 4 + x * 4 + 3] = 0;
    }
  vkUnmapMemory(device, imageMemory);

  // Command recording
  std::vector<uint32_t> indices;
  std::for_each(requiredQueue.begin(),
                requiredQueue.end(),
                [&indices](const QueueFamily& qf) {
                  indices.push_back(qf.index);
                });
  VkCommandPoolCreateInfo cmdPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = indices[0],
  };

  VkCommandPool pool;
  CALL_VK(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &pool));

  VkCommandBuffer gfxCmd;
  const VkCommandBufferAllocateInfo cmd = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  CALL_VK(vkAllocateCommandBuffers(device, &cmd, &gfxCmd));
  VkCommandBufferBeginInfo cmd_buf_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr
  };
  CALL_VK(vkBeginCommandBuffer(gfxCmd, &cmd_buf_info));

  // Set image layout
  setImageLayout(gfxCmd,
                 VK_IMAGE_LAYOUT_PREINITIALIZED,
                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                 VK_PIPELINE_STAGE_HOST_BIT,
                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  CALL_VK(vkEndCommandBuffer(gfxCmd));

  VkFenceCreateInfo fenceInfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
  };
  VkFence fence;
  CALL_VK(vkCreateFence(device, &fenceInfo, nullptr, &fence));

  VkSubmitInfo submitInfo = {
      .pNext = nullptr,
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &gfxCmd,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr,
  };
  CALL_VK(vkQueueSubmit(queue[0], 1, &submitInfo, fence));
  CALL_VK(vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000));
  vkDestroyFence(device, fence, nullptr);

  vkFreeCommandBuffers(device, pool, 1, &gfxCmd);
  vkDestroyCommandPool(device, pool, nullptr);
}

//------------------------------------------

void bindDescSet() {
  for (int i = 0; i < commandBuffer.size(); ++i) {
    vkCmdBindDescriptorSets(commandBuffer[i],
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            layout,
                            0,
                            2,
                            descSet,
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
  CALL_VK(vkFreeDescriptorSets(device, descPool[0], 1, &descSet[0]));
  CALL_VK(vkFreeDescriptorSets(device, descPool[1], 1, &descSet[1]));
  vkDestroyDescriptorPool(device, descPool[0], NULL);
  vkDestroyDescriptorPool(device, descPool[1], NULL);
  vkDestroyDescriptorSetLayout(device, descSetLayout[0], NULL);
  vkDestroyDescriptorSetLayout(device, descSetLayout[1], NULL);
  vkDestroyImage(device, image, NULL);
  vkDestroyBuffer(device, buffer[0], NULL);
  vkDestroyBuffer(device, buffer[1], NULL);
  vkFreeMemory(device, deviceMemory[0], NULL);
  vkFreeMemory(device, deviceMemory[1], NULL);

  vkFreeMemory(device, imageMemory, NULL);
  vkDestroyImageView(device, imageView, NULL);
  vkDestroySampler(device, sampler, NULL);

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

  // Memory allocation
  createBuffer();
  exploreMemoryProperty();
  allocateAndMapMemory();
  createImage();

  // Descriptor set
  createDescriptorPool();
  createDescriptorSetLayout();
  createPipelineLayout();

  // Pipeline build
  createShaderModule();
  createComputePipeline();

  createDescriptorSet();

  updateDescriptorSet(); // NOTE: updating descriptor set must be done before
                         //       updating resources
  memoryInit();
  initImage();

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
