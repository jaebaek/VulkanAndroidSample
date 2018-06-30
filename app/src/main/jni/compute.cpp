// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>

#include <algorithm>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#include "log.h"
#include "vkcompute.h"
#include "vulkan_wrapper.h"

#include "spirv.inc"

VkResult vkGetBestComputeQueueNPH(VkPhysicalDevice physicalDevice, uint32_t* queueFamilyIndex) {
  uint32_t queueFamilyPropertiesCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, 0);

  VkQueueFamilyProperties* const queueFamilyProperties = (VkQueueFamilyProperties*)malloc(
      sizeof(VkQueueFamilyProperties) * queueFamilyPropertiesCount);

  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties);

  // first try and find a queue that has just the compute bit set
  for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
    // mask out the sparse binding bit that we aren't caring about (yet!) and the transfer bit
    const VkQueueFlags maskedFlags = (~(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT) &
        queueFamilyProperties[i].queueFlags);

    if (!(VK_QUEUE_GRAPHICS_BIT & maskedFlags) && (VK_QUEUE_COMPUTE_BIT & maskedFlags)) {
      *queueFamilyIndex = i;
      return VK_SUCCESS;
    }
  }

  // lastly get any queue that'll work for us
  for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
    // mask out the sparse binding bit that we aren't caring about (yet!) and the transfer bit
    const VkQueueFlags maskedFlags = (~(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT) &
        queueFamilyProperties[i].queueFlags);

    if (VK_QUEUE_COMPUTE_BIT & maskedFlags) {
      *queueFamilyIndex = i;
      return VK_SUCCESS;
    }
  }

  return VK_ERROR_INITIALIZATION_FAILED;
}

void Test() {
  VkApplicationInfo applicationInfo = { };
  applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  applicationInfo.pApplicationName = "vkcompute";
  applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 2);

  VkInstanceCreateInfo instanceCreateInfo = { };
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = &applicationInfo;

  VkInstance instance;
  CALL_VK2(vkCreateInstance, &instanceCreateInfo, 0, &instance);

  uint32_t physicalDeviceCount = 0;
  CALL_VK2(vkEnumeratePhysicalDevices, instance, &physicalDeviceCount, 0);

  VkPhysicalDevice* const physicalDevices = (VkPhysicalDevice*)malloc(
      sizeof(VkPhysicalDevice) * physicalDeviceCount);

  CALL_VK2(vkEnumeratePhysicalDevices, instance, &physicalDeviceCount, physicalDevices);

  for (uint32_t i = 0; i < physicalDeviceCount; i++) {
    uint32_t queueFamilyIndex = 0;
    CALL_VK2(vkGetBestComputeQueueNPH, physicalDevices[i], &queueFamilyIndex);

    LOGI("QFI: %u", i);
    const float queuePrioritory = 1.0f;
    const VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      0,
      0,
      queueFamilyIndex,
      1,
      &queuePrioritory
    };

    const VkDeviceCreateInfo deviceCreateInfo = {
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      0,
      0,
      1,
      &deviceQueueCreateInfo,
      0,
      0,
      0,
      0,
      0
    };

    VkDevice device;
    CALL_VK2(vkCreateDevice, physicalDevices[i], &deviceCreateInfo, 0, &device);

    VkQueue queue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    VkCommandPoolCreateInfo commandPoolCreateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      0,
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      queueFamilyIndex
    };

    VkCommandPool commandPool;
    CALL_VK2(vkCreateCommandPool, device, &commandPoolCreateInfo, 0, &commandPool);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      0,
      commandPool,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      1
    };

    VkCommandBuffer commandBuffer;
    CALL_VK2(vkAllocateCommandBuffers, device, &commandBufferAllocateInfo, &commandBuffer);

    VkDescriptorPoolSize descriptorPoolSize[] = {
      {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        4
      },
      {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        4
      },
      {
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        4
      }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      0,
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      4,
      NELEMS(descriptorPoolSize),
      descriptorPoolSize
    };

    VkDescriptorPool descriptorPool;
    CALL_VK2(vkCreateDescriptorPool, device, &descriptorPoolCreateInfo, 0, &descriptorPool);

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {
      {
        0,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        1,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0
      },
      {
        1,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        1,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0
      }
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      0,
      0,
      2,
      descriptorSetLayoutBindings
    };

    VkDescriptorSetLayout descriptorSetLayout;
    CALL_VK2(vkCreateDescriptorSetLayout, device, &descriptorSetLayoutCreateInfo, 0, &descriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      0,
      0,
      1,
      &descriptorSetLayout,
      0,
      0
    };

    VkPipelineLayout pipelineLayout;
    CALL_VK2(vkCreatePipelineLayout, device, &pipelineLayoutCreateInfo, 0, &pipelineLayout);

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      0,
      0,
      sizeof(spirv),
      (const uint32_t*)spirv
    };

    VkShaderModule shader_module;

    CALL_VK2(vkCreateShaderModule, device, &shaderModuleCreateInfo, 0, &shader_module);

    VkComputePipelineCreateInfo computePipelineCreateInfo = {
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      0,
      0,
      {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        0,
        0,
        VK_SHADER_STAGE_COMPUTE_BIT,
        shader_module,
        "foo",
        0
      },
      pipelineLayout,
      0,
      0
    };

    VkPipeline pipeline;
    CALL_VK2(vkCreateComputePipelines, device, 0, 1, &computePipelineCreateInfo, 0, &pipeline);

    const int32_t bufferLength = 16;

    const uint32_t bufferSize = sizeof(int32_t) * bufferLength;

    // we are going to need two buffers from this one memory
    const VkDeviceSize memorySize = bufferSize;

    const VkBufferCreateInfo bufferCreateInfo = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      0,
      0,
      bufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer in_buffer;
    CALL_VK2(vkCreateBuffer, device, &bufferCreateInfo, 0, &in_buffer);

    VkPhysicalDeviceMemoryProperties properties;

    vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &properties);

    // set memoryTypeIndex to an invalid entry in the properties.memoryTypes array
    uint32_t memoryTypeIndex = VK_MAX_MEMORY_TYPES;

    for (uint32_t k = 0; k < properties.memoryTypeCount; k++) {
      if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & properties.memoryTypes[k].propertyFlags) &&
          (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & properties.memoryTypes[k].propertyFlags) &&
          (memorySize < properties.memoryHeaps[properties.memoryTypes[k].heapIndex].size)) {
    LOGI("mem_type[%u] = %x", k, (uint32_t)properties.memoryTypes[k].propertyFlags);
        memoryTypeIndex = k;
        break;
      }
    }

    const VkMemoryAllocateInfo memoryAllocateInfo = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      memorySize,
      memoryTypeIndex
    };

    VkDeviceMemory memory0;
    CALL_VK2(vkAllocateMemory, device, &memoryAllocateInfo, 0, &memory0);

    CALL_VK2(vkBindBufferMemory, device, in_buffer, memory0, 0);

    float *payload0;
    CALL_VK2(vkMapMemory,device, memory0, 0, memorySize, 0, (void **)&payload0);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      0,
      descriptorPool,
      1,
      &descriptorSetLayout
    };

    VkDescriptorSet descriptorSet;
    CALL_VK2(vkAllocateDescriptorSets, device, &descriptorSetAllocateInfo, &descriptorSet);

    VkBuffer out_buffer;
    CALL_VK2(vkCreateBuffer, device, &bufferCreateInfo, 0, &out_buffer);

    VkDeviceMemory memory1;
    CALL_VK2(vkAllocateMemory, device, &memoryAllocateInfo, 0, &memory1);

    CALL_VK2(vkBindBufferMemory, device, out_buffer, memory1, 0);

    float *payload1;
    CALL_VK2(vkMapMemory, device, memory1, 0, memorySize, 0, (void **)&payload1);

    VkDescriptorBufferInfo in_descriptorBufferInfo = {
      in_buffer,
      0,
      VK_WHOLE_SIZE
    };

    VkDescriptorBufferInfo out_descriptorBufferInfo = {
      out_buffer,
      0,
      VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet writeDescriptorSet[2] = {
      {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        0,
        descriptorSet,
        0,
        0,
        1,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        0,
        &in_descriptorBufferInfo,
        0
      },
      {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        0,
        descriptorSet,
        1,
        0,
        1,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        0,
        &out_descriptorBufferInfo,
        0
      }
    };

    vkUpdateDescriptorSets(device, 2, writeDescriptorSet, 0, 0);
  // --------------------
    // Set up memory 0
    for (uint32_t k = 1; k < memorySize / sizeof(float); k++) {
      payload0[k] = k;
    }

    // Set up memory 1
    for (uint32_t k = 1; k < memorySize / sizeof(float); k++) {
      payload1[k] = 2 * k;
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      0,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      0
    };

    CALL_VK2(vkBeginCommandBuffer, commandBuffer, &commandBufferBeginInfo);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineLayout, 0, 1, &descriptorSet, 0, 0);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    vkCmdDispatch(commandBuffer, bufferSize / sizeof(int32_t), 1, 1);

    CALL_VK2(vkEndCommandBuffer, commandBuffer);

    const VkPipelineStageFlags vkPipelineStageFlags =
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.pWaitDstStageMask = &vkPipelineStageFlags;

    CALL_VK2(vkQueueSubmit, queue, 1, &submitInfo, 0);

    CALL_VK2(vkQueueWaitIdle, queue);

    // Print memory 0
    for (uint32_t k = 0, e = bufferSize / sizeof(float); k < e; k++) {
        LOGI("payload0[%u] = %f", k, payload0[k]);
    }
    vkUnmapMemory(device, memory0);

    // Print memory 1
    for (uint32_t k = 0, e = bufferSize / sizeof(float); k < e; k++) {
        LOGI("payload1[%u] = %f", k, payload1[k]);
    }
    vkUnmapMemory(device, memory1);
  }
}
