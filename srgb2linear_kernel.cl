constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

void kernel srgb2linear(read_only image2d_t image, global uint* outbuffer) {
    const int2 pos = {get_global_id(0), get_global_id(1)};

    uint final_color=0;
    uint4 color = (uint4){0xFF, 0xFF, 0xFF, 0xFF} - read_imageui(image, sampler, pos);
    final_color |= color.z;
    final_color |= color.y << 8;
    final_color |= color.x << 16;

    outbuffer[pos.x+pos.y*get_global_size(0)] = final_color;
}
