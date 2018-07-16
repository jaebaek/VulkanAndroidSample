const sampler_t sampler =
CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

kernel void foo(global float* out,
                global float* in,
                __read_only image2d_t inImage) {
  uint i = get_global_id(0);
  float sum = 0.0f;
  for (int j = 0; j < 4;++j)
    sum += read_imagef(inImage, sampler, (float2)(0.5, .05))[j];
  out[i] = in[i] + 100.0f * sum;
}
