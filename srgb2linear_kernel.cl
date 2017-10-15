constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

void kernel srgb2linear(read_only image2d_t image, write_only image2d_t outimage) { // a same image cannot be rw
    const int2 pos = {get_global_id(0), get_global_id(1)};


    //float final_color=0.0f;

    //uint4 rel_lum = (uint4){

    float4 current_pixel = read_imagef(image, sampler, pos);

    //                       R     G     B     A
    //float4 color = (float4){1.0f, 1.0f, 1.0f, 1.0f} * ;

    //float4 color = pow(current_pixel, exp_linear);
    //color = pow(current_pixel, exp_srgb);

    //final_color |= color.z;
    //final_color |= color.y << 8;
    //final_color |= color.x << 16;

    write_imagef(outimage, pos, current_pixel);
    //outbuffer[pos.x+pos.y*get_global_size(0)] = (float)final_color;
    //outbuffer[pos.x+pos.y*get_global_size(0)] = read_imagef(image, sampler, pos);
}
