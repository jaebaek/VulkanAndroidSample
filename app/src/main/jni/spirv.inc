/*

// OpenCL
const sampler_t sampler =
CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;

kernel void foo(global float* out,
                global float* in,
                __read_only image2d_t inImage) {
  uint i = get_global_id(0);
  float sum = 0.0f;
  for (int j = 0; j < 4;++j)
    sum += read_imagef(inImage, sampler, (float2)((float)i, (float)i))[j];
  out[i] = in[i] + 100.0f * sum;
}

; SPIR-V
; Version: 1.0
; Generator: Codeplay; 0
; Bound: 69
; Schema: 0
               OpCapability Shader
               OpCapability VariablePointers
               OpExtension "SPV_KHR_storage_buffer_storage_class"
               OpExtension "SPV_KHR_variable_pointers"
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %36 "foo" %gl_GlobalInvocationID
               OpSource OpenCL_C 120
               OpDecorate %28 SpecId 0
               OpDecorate %29 SpecId 1
               OpDecorate %30 SpecId 2
               OpDecorate %_runtimearr_float ArrayStride 4
               OpMemberDecorate %_struct_5 0 Offset 0
               OpDecorate %_struct_5 Block
               OpDecorate %26 DescriptorSet 0
               OpDecorate %26 Binding 0
               OpDecorate %gl_GlobalInvocationID BuiltIn GlobalInvocationId
               OpDecorate %gl_WorkGroupSize BuiltIn WorkgroupSize
               OpDecorate %33 DescriptorSet 1
               OpDecorate %33 Binding 0
               OpDecorate %34 DescriptorSet 1
               OpDecorate %34 Binding 1
               OpDecorate %35 DescriptorSet 1
               OpDecorate %35 Binding 2
               OpDecorate %35 NonWritable
      %float = OpTypeFloat 32
%_ptr_StorageBuffer_float = OpTypePointer StorageBuffer %float
%_runtimearr_float = OpTypeRuntimeArray %float
  %_struct_5 = OpTypeStruct %_runtimearr_float
%_ptr_StorageBuffer__struct_5 = OpTypePointer StorageBuffer %_struct_5
          %7 = OpTypeImage %float 2D 0 0 0 1 Unknown
%_ptr_UniformConstant_7 = OpTypePointer UniformConstant %7
       %uint = OpTypeInt 32 0
       %void = OpTypeVoid
         %11 = OpTypeFunction %void
     %v3uint = OpTypeVector %uint 3
%_ptr_Input_v3uint = OpTypePointer Input %v3uint
%_ptr_Input_uint = OpTypePointer Input %uint
    %v2float = OpTypeVector %float 2
    %v4float = OpTypeVector %float 4
         %17 = OpTypeSampler
%_ptr_UniformConstant_17 = OpTypePointer UniformConstant %17
%_ptr_Private_v3uint = OpTypePointer Private %v3uint
         %20 = OpTypeSampledImage %7
    %float_0 = OpConstant %float 0
     %uint_0 = OpConstant %uint 0
    %float_3 = OpConstant %float 3
         %24 = OpUndef %v2float
  %float_100 = OpConstant %float 100
         %26 = OpVariable %_ptr_UniformConstant_17 UniformConstant
%gl_GlobalInvocationID = OpVariable %_ptr_Input_v3uint Input
         %28 = OpSpecConstant %uint 1
         %29 = OpSpecConstant %uint 1
         %30 = OpSpecConstant %uint 1
%gl_WorkGroupSize = OpSpecConstantComposite %v3uint %28 %29 %30
         %32 = OpVariable %_ptr_Private_v3uint Private %gl_WorkGroupSize
         %33 = OpVariable %_ptr_StorageBuffer__struct_5 StorageBuffer
         %34 = OpVariable %_ptr_StorageBuffer__struct_5 StorageBuffer
         %35 = OpVariable %_ptr_UniformConstant_7 UniformConstant
         %36 = OpFunction %void None %11
         %37 = OpLabel
         %38 = OpLoad %7 %35
         %39 = OpAccessChain %_ptr_Input_uint %gl_GlobalInvocationID %uint_0
         %40 = OpLoad %uint %39
         %41 = OpConvertUToF %float %40
         %42 = OpFMul %float %41 %float_3
         %43 = OpCompositeInsert %v2float %42 %24 0
         %44 = OpVectorShuffle %v2float %43 %24 0 0
         %45 = OpLoad %17 %26
         %46 = OpSampledImage %20 %38 %45
         %47 = OpImageSampleExplicitLod %v4float %46 %44 Lod %float_0
         %48 = OpCompositeExtract %float %47 0
         %49 = OpFAdd %float %48 %float_0
         %50 = OpLoad %17 %26
         %51 = OpSampledImage %20 %38 %50
         %52 = OpImageSampleExplicitLod %v4float %51 %44 Lod %float_0
         %53 = OpCompositeExtract %float %52 1
         %54 = OpFAdd %float %49 %53
         %55 = OpLoad %17 %26
         %56 = OpSampledImage %20 %38 %55
         %57 = OpImageSampleExplicitLod %v4float %56 %44 Lod %float_0
         %58 = OpCompositeExtract %float %57 2
         %59 = OpFAdd %float %54 %58
         %60 = OpLoad %17 %26
         %61 = OpSampledImage %20 %38 %60
         %62 = OpImageSampleExplicitLod %v4float %61 %44 Lod %float_0
         %63 = OpCompositeExtract %float %62 3
         %64 = OpFAdd %float %59 %63
         %65 = OpAccessChain %_ptr_StorageBuffer_float %34 %uint_0 %40
         %66 = OpLoad %float %65
         %67 = OpExtInst %float %1 Fma %64 %float_100 %66
         %68 = OpAccessChain %_ptr_StorageBuffer_float %33 %uint_0 %40
               OpStore %68 %67
               OpReturn
               OpFunctionEnd

*/

const uint32_t spirv[] = {
0x07230203, 0x00010000, 0x00030000, 0x00000045, 0x00000000, 0x00020011, 0x00000001, 0x00020011,
0x0000115a, 0x000b000a, 0x5f565053, 0x5f52484b, 0x726f7473, 0x5f656761, 0x66667562, 0x735f7265,
0x61726f74, 0x635f6567, 0x7373616c, 0x00000000, 0x0008000a, 0x5f565053, 0x5f52484b, 0x69726176,
0x656c6261, 0x696f705f, 0x7265746e, 0x00000073, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e,
0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0005000f, 0x00000005, 0x00000024,
0x006f6f66, 0x0000001b, 0x00030003, 0x00000003, 0x00000078, 0x00040047, 0x0000001c, 0x00000001,
0x00000000, 0x00040047, 0x0000001d, 0x00000001, 0x00000001, 0x00040047, 0x0000001e, 0x00000001,
0x00000002, 0x00040047, 0x00000004, 0x00000006, 0x00000004, 0x00050048, 0x00000005, 0x00000000,
0x00000023, 0x00000000, 0x00030047, 0x00000005, 0x00000002, 0x00040047, 0x0000001a, 0x00000022,
0x00000000, 0x00040047, 0x0000001a, 0x00000021, 0x00000000, 0x00040047, 0x0000001b, 0x0000000b,
0x0000001c, 0x00040047, 0x0000001f, 0x0000000b, 0x00000019, 0x00040047, 0x00000021, 0x00000022,
0x00000001, 0x00040047, 0x00000021, 0x00000021, 0x00000000, 0x00040047, 0x00000022, 0x00000022,
0x00000001, 0x00040047, 0x00000022, 0x00000021, 0x00000001, 0x00040047, 0x00000023, 0x00000022,
0x00000001, 0x00040047, 0x00000023, 0x00000021, 0x00000002, 0x00030047, 0x00000023, 0x00000018,
0x00030016, 0x00000002, 0x00000020, 0x00040020, 0x00000003, 0x0000000c, 0x00000002, 0x0003001d,
0x00000004, 0x00000002, 0x0003001e, 0x00000005, 0x00000004, 0x00040020, 0x00000006, 0x0000000c,
0x00000005, 0x00090019, 0x00000007, 0x00000002, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
0x00000001, 0x00000000, 0x00040020, 0x00000008, 0x00000000, 0x00000007, 0x00040015, 0x00000009,
0x00000020, 0x00000000, 0x00020013, 0x0000000a, 0x00030021, 0x0000000b, 0x0000000a, 0x00040017,
0x0000000c, 0x00000009, 0x00000003, 0x00040020, 0x0000000d, 0x00000001, 0x0000000c, 0x00040020,
0x0000000e, 0x00000001, 0x00000009, 0x00040017, 0x0000000f, 0x00000002, 0x00000002, 0x00040017,
0x00000010, 0x00000002, 0x00000004, 0x0002001a, 0x00000011, 0x00040020, 0x00000012, 0x00000000,
0x00000011, 0x00040020, 0x00000013, 0x00000006, 0x0000000c, 0x0003001b, 0x00000014, 0x00000007,
0x0004002b, 0x00000002, 0x00000015, 0x00000000, 0x0004002b, 0x00000009, 0x00000016, 0x00000000,
0x0004002b, 0x00000002, 0x00000017, 0x40400000, 0x00030001, 0x0000000f, 0x00000018, 0x0004002b,
0x00000002, 0x00000019, 0x42c80000, 0x0004003b, 0x00000012, 0x0000001a, 0x00000000, 0x0004003b,
0x0000000d, 0x0000001b, 0x00000001, 0x00040032, 0x00000009, 0x0000001c, 0x00000001, 0x00040032,
0x00000009, 0x0000001d, 0x00000001, 0x00040032, 0x00000009, 0x0000001e, 0x00000001, 0x00060033,
0x0000000c, 0x0000001f, 0x0000001c, 0x0000001d, 0x0000001e, 0x0005003b, 0x00000013, 0x00000020,
0x00000006, 0x0000001f, 0x0004003b, 0x00000006, 0x00000021, 0x0000000c, 0x0004003b, 0x00000006,
0x00000022, 0x0000000c, 0x0004003b, 0x00000008, 0x00000023, 0x00000000, 0x00050036, 0x0000000a,
0x00000024, 0x00000000, 0x0000000b, 0x000200f8, 0x00000025, 0x0004003d, 0x00000007, 0x00000026,
0x00000023, 0x00050041, 0x0000000e, 0x00000027, 0x0000001b, 0x00000016, 0x0004003d, 0x00000009,
0x00000028, 0x00000027, 0x00040070, 0x00000002, 0x00000029, 0x00000028, 0x00050085, 0x00000002,
0x0000002a, 0x00000029, 0x00000017, 0x00060052, 0x0000000f, 0x0000002b, 0x0000002a, 0x00000018,
0x00000000, 0x0007004f, 0x0000000f, 0x0000002c, 0x0000002b, 0x00000018, 0x00000000, 0x00000000,
0x0004003d, 0x00000011, 0x0000002d, 0x0000001a, 0x00050056, 0x00000014, 0x0000002e, 0x00000026,
0x0000002d, 0x00070058, 0x00000010, 0x0000002f, 0x0000002e, 0x0000002c, 0x00000002, 0x00000015,
0x00050051, 0x00000002, 0x00000030, 0x0000002f, 0x00000000, 0x00050081, 0x00000002, 0x00000031,
0x00000030, 0x00000015, 0x0004003d, 0x00000011, 0x00000032, 0x0000001a, 0x00050056, 0x00000014,
0x00000033, 0x00000026, 0x00000032, 0x00070058, 0x00000010, 0x00000034, 0x00000033, 0x0000002c,
0x00000002, 0x00000015, 0x00050051, 0x00000002, 0x00000035, 0x00000034, 0x00000001, 0x00050081,
0x00000002, 0x00000036, 0x00000031, 0x00000035, 0x0004003d, 0x00000011, 0x00000037, 0x0000001a,
0x00050056, 0x00000014, 0x00000038, 0x00000026, 0x00000037, 0x00070058, 0x00000010, 0x00000039,
0x00000038, 0x0000002c, 0x00000002, 0x00000015, 0x00050051, 0x00000002, 0x0000003a, 0x00000039,
0x00000002, 0x00050081, 0x00000002, 0x0000003b, 0x00000036, 0x0000003a, 0x0004003d, 0x00000011,
0x0000003c, 0x0000001a, 0x00050056, 0x00000014, 0x0000003d, 0x00000026, 0x0000003c, 0x00070058,
0x00000010, 0x0000003e, 0x0000003d, 0x0000002c, 0x00000002, 0x00000015, 0x00050051, 0x00000002,
0x0000003f, 0x0000003e, 0x00000003, 0x00050081, 0x00000002, 0x00000040, 0x0000003b, 0x0000003f,
0x00060041, 0x00000003, 0x00000041, 0x00000022, 0x00000016, 0x00000028, 0x0004003d, 0x00000002,
0x00000042, 0x00000041, 0x0008000c, 0x00000002, 0x00000043, 0x00000001, 0x00000032, 0x00000040,
0x00000019, 0x00000042, 0x00060041, 0x00000003, 0x00000044, 0x00000021, 0x00000016, 0x00000028,
0x0003003e, 0x00000044, 0x00000043, 0x000100fd, 0x00010038
};
