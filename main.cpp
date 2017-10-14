#include <iostream>
#include <fstream>
#include <vector>

// These are for the random generation
#include <cstdlib>
#include <ctime>

#include <CL/cl.hpp>

#include "bitmap.hpp"

#define TERM_BOLD "\033[1m"
#define TERM_GREEN "\x1b[32m"
#define TERM_RED "\x1b[31m"
#define TERM_CYAN "\033[36m"
#define TERM_RESET "\x1b[0m"

#define M_VECTOR_SIZE 4096
#define MAX_FLOAT 2048

// Switch to init the vectors on host
#define HOST_VECINIT 0

#define DEBUG 1

std::string read_kernel(std::string kernel_path) {
    // Read the kernel file and return it as string;
    std::ifstream sourceFile(kernel_path);
    std::string sourceCode( (std::istreambuf_iterator<char>(sourceFile)), (std::istreambuf_iterator<char>()));
    sourceFile.close();
    return sourceCode;
}

std::string get_dir(const std::string filepath) {
    std::string toret = "";

    for (int i=filepath.length(); i>=0; i--) {
        if (filepath[i] == '/') {
            toret = filepath.substr(0,i);
            break;
        }
    }

    if (toret == "") {
        std::cerr << TERM_RED << "Error: provided filepath is not a valid path." << TERM_RESET;
        return NULL;
    }

    return toret;
}

cl::Device ocl_get_default_device() {
    // get all platforms
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    int howmany_platforms = all_platforms.size();
    if (howmany_platforms == 0) {
        std::cerr << "No platforms found. Make sure OpenCL is installed correctly.\n";
        exit(1);
    }

    // Platform slection
    std::cout << "Platforms found:\n";
    for (int i=0; i<howmany_platforms; i++) {
        std::cout << "#" << i << ": " << all_platforms[i].getInfo<CL_PLATFORM_NAME>() << std::endl;
    }

    cl::Platform default_platform = all_platforms[0];
    std::cout << "Using default platform " <<
                 TERM_BOLD <<
                 default_platform.getInfo<CL_PLATFORM_NAME>() <<
                 TERM_RESET <<
                 std::endl;

    // Device selection
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    int howmany_devices = all_devices.size();
    if (howmany_devices == 0){
        std::cerr << "No devices found. Make sure OpenCL is installed correctly.\n";
        exit(1);
    }
    cl::Device default_device = all_devices[0];
    std::cout << "Using default device " <<
                TERM_BOLD <<
                default_device.getInfo<CL_DEVICE_NAME>() <<
                TERM_RESET <<
                std::endl;
    return default_device;
}

void write_bitmap(BMPVEC& buffer, std::string outpath) {
    std::ofstream outfs(outpath, std::ios::out | std::ios::binary);
    outfs.write(&buffer[0], buffer.size());
    outfs.close();
}

void get_bitmap_data(BMPVEC& buffer, BMPVEC& nbuffer) {
    PBITMAPFILEHEADER b_fh = (PBITMAPFILEHEADER)(&buffer[0]);
    DWORD offset = b_fh->bfOffBits;

    BMPVEC::const_iterator first = buffer.begin() + offset;
    BMPVEC::const_iterator last = buffer.end();

    nbuffer = BMPVEC(first, last);

#if DEBUG
    std::cout << TERM_GREEN << "Test newbuffer\n" << TERM_RESET;
    printf("First blue: %02X \nLast red: %02X \n", (unsigned char)nbuffer[0], (unsigned char)nbuffer[nbuffer.size()-1]);
#endif
}

DWORD get_bitmap_offset(BMPVEC& buffer) {
    PBITMAPFILEHEADER b_fh = (PBITMAPFILEHEADER)(&buffer[0]);
    return b_fh->bfOffBits;
}

void read_bitmap(std::string path, BMPVEC& buffer) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        std::cerr << TERM_RED << "Error: Could not find image for path " << path << std::endl << TERM_RESET;
        exit(1);
    }

    PBITMAPFILEHEADER b_fh;
    PBITMAPINFOHEADER b_ih;

    file.seekg(0,std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0,std::ios::beg);

    buffer.resize(length);
    file.read(&buffer[0], length);

    b_fh = (PBITMAPFILEHEADER)(&buffer[0]);
    b_ih = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));

#if DEBUG
    std::cout << TERM_CYAN << "Picture size: " << b_ih->biWidth << " x " << b_ih->biHeight << std::endl << TERM_RESET;
    std::cout << TERM_CYAN << "Bitmap data offset: " << b_fh->bfOffBits << std::endl << TERM_RESET;
    printf("Fist blue: %02X \n", (unsigned char)buffer[b_fh->bfOffBits]);
#endif

    file.close();
}

void bgr2bgra(BMPVEC& rawbmp, BMPVEC& bgravec) {
    bgravec.reserve(rawbmp.size() + rawbmp.size()/3);
    int k = 0;
    for (int i = 0; i < rawbmp.size(); i+=3) {
        bgravec[k]=rawbmp[i];
        bgravec[k+1] = rawbmp[i+1];
        bgravec[k+2] = rawbmp[i+2];
        bgravec[k+3] = 0xFF;
        k+=4;
    }
}

void prepend_bitmap_headers(BMPVEC& original, float* new_pixeldata) {
    PBITMAPFILEHEADER b_fh = (PBITMAPFILEHEADER)(&original[0]);
    int k = 0;
    uint8_t* p = reinterpret_cast<uint8_t*>(new_pixeldata);
    for (int i = b_fh->bfOffBits; i < original.size(); i+=3) {
        original[i] = p[k];
        original[i+1] = p[k+1];
        original[i+2] = p[k+2];
        k+=4;
    }
}

int main(int argc, char** argv) {

    std::string pwd = get_dir(argv[0]);

    BMPVEC bmp;
    read_bitmap("/home/gabmus/Desktop/george1024.bmp", bmp);
    DWORD bmp_offset = get_bitmap_offset(bmp);

    int bmp_width = 0, bmp_height = 0;
    PBITMAPINFOHEADER b_ih = (PBITMAPINFOHEADER)(&bmp[0] + sizeof(BITMAPFILEHEADER));
    bmp_width = b_ih->biWidth;
    bmp_height = b_ih->biHeight;

    std::cout << TERM_GREEN << bmp_width << "x" << bmp_height << std::endl << TERM_RESET;


    //BMPVEC bmp_data get_bitmap_data(bmp);
    // Example of what can be done on the array
    /*for (int i = bmp_offset; i < bmp.size(); i++) {
        bmp[i] += 0x22;
    }
    write_bitmap(bmp, "/home/gabmus/outfile.bmp");*/


    cl::Device default_device = ocl_get_default_device();

    // Context definition
    cl::Context context({default_device});

    // Define the sources for device code
    cl::Program::Sources sources;

    BMPVEC bmp_bgra_data;
    BMPVEC bmp_bgr_data;
    get_bitmap_data(bmp, bmp_bgr_data);
    bgr2bgra(bmp_bgr_data, bmp_bgra_data);

    cl::Image2D cl_image = cl::Image2D(
                context,
                CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                cl::ImageFormat(CL_BGRA, CL_UNORM_INT8),
                bmp_width, bmp_height,
                0,
                (void*)(&bmp_bgra_data[0]));

    cl::Image2D cl_outimage = cl::Image2D(
                context,
                CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                cl::ImageFormat(CL_BGRA, CL_UNORM_INT8),
                bmp_width, bmp_height,
                0,
                (void*)(&bmp_bgra_data[0]));

    std::string kernel_code_srgb2linear = read_kernel(pwd + "/srgb2linear_kernel.cl");
    sources.push_back({kernel_code_srgb2linear.c_str(), kernel_code_srgb2linear.length()});


    // Build device code
    cl::Program program(context, sources);
    if (program.build({default_device}) != CL_SUCCESS) {
        std::cerr << TERM_RED <<
                    "Error Building: " <<
                    program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) <<
                    TERM_RESET <<
                    std::endl;
        exit(1);
    }

    cl::CommandQueue queue(context, default_device);

    // Prepare to run the kernel
    cl::Kernel kernel_srgb2linear = cl::Kernel(program, "srgb2linear");
    kernel_srgb2linear.setArg(0, cl_image);
    kernel_srgb2linear.setArg(1, cl_outimage);
    queue.enqueueNDRangeKernel(kernel_srgb2linear, cl::NullRange, cl::NDRange(bmp_width, bmp_height), cl::NullRange);
    queue.finish();

    // Get the results out of the device, back to the host
    // Init host out vector
    /*float* host_outvec = new float[bmp_width*bmp_height];
    queue.enqueueReadBuffer(
                cl_result,
                CL_TRUE,
                0,
                sizeof(float)*bmp_width*bmp_height,
                host_outvec);

    bool isAllZero = true;
    for (int i = 0; i < bmp_width * bmp_height; i++)
        if (host_outvec[i]) isAllZero = false;
    printf( isAllZero ? "Image is black\n" : "Image is not black\n" );
    */
    float* host_outvec = new float[bmp_width*bmp_height];
    cl::size_t<3> ri_origin;
    ri_origin[0] = 0;
    ri_origin[1] = 0;
    ri_origin[2] = 0;
    cl::size_t<3> ri_region;
    ri_region[0] = bmp_width;
    ri_region[1] = bmp_height;
    ri_region[2] = 1;
    queue.enqueueReadImage(
                cl_outimage,
                CL_TRUE,
                ri_origin,
                ri_region,
                0,
                0,
                host_outvec);
    prepend_bitmap_headers(bmp, host_outvec);

    write_bitmap(bmp, "/home/gabmus/test_ocl.bmp");
    return 0;

/*
#if !HOST_VECINIT
    std::string kernel_code_vecinit = read_kernel(pwd + "/vecinit_kernel.cl");
    sources.push_back({kernel_code_vecinit.c_str(), kernel_code_vecinit.length()});
#endif

    std::string kernel_code_vaddSimple = read_kernel(pwd + "/vaddSimple_kernel.cl");
    sources.push_back({kernel_code_vaddSimple.c_str(), kernel_code_vaddSimple.length()});

    // Build device code
    cl::Program program(context, sources);
    if (program.build({default_device}) != CL_SUCCESS) {
        std::cerr << TERM_RED <<
                    "Error Building: " <<
                    program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) <<
                    TERM_RESET <<
                    std::endl;
        exit(1);
    }

    // Initialize space for vectors on device as buffers

    cl::Buffer b_A(context, CL_MEM_READ_WRITE, sizeof(float) * M_VECTOR_SIZE);
    cl::Buffer b_B(context, CL_MEM_READ_WRITE, sizeof(float) * M_VECTOR_SIZE);
    cl::Buffer b_C(context, CL_MEM_READ_WRITE, sizeof(float) * M_VECTOR_SIZE);

    // Initialize vectors on host (don't need to have a C[] here)

    float A[M_VECTOR_SIZE];
    float B[M_VECTOR_SIZE];

#if HOST_VECINIT
    for (int i=0; i<M_VECTOR_SIZE; i++) {
        A[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX/MAX_FLOAT);
        B[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX/MAX_FLOAT);
    }
#endif
    // Create queue, and write arrays to device buffer in this queue
    cl::CommandQueue queue(context, default_device);
#if HOST_VECINIT
    queue.enqueueWriteBuffer(b_A, CL_TRUE, 0, sizeof(float)*M_VECTOR_SIZE, A);
    queue.enqueueWriteBuffer(b_B, CL_TRUE, 0, sizeof(float)*M_VECTOR_SIZE, B);
#endif


    // ---DEPRECATED---
    // Prepare to run the kernel                              vvv "vaddSimple" is the name of the kernel function
    //                                                                                                        vvv number of threads we want
    //cl::KernelFunctorGlobal vaddSimple_functor(cl::Kernel(program, "vaddSimple"), queue, cl::NullRange, cl::NDRange(M_VECTOR_SIZE), cl::NullRange);
    // Run the kernel calling the functor
    //vaddSimple_functor(b_A, b_B, b_C);

#if !HOST_VECINIT
    cl::Kernel kernel_vecinit = cl::Kernel(program, "vecinit");
    kernel_vecinit.setArg(0, b_A);
    kernel_vecinit.setArg(1, b_B);
    queue.enqueueNDRangeKernel(kernel_vecinit, cl::NullRange, cl::NDRange(M_VECTOR_SIZE), cl::NullRange);
    queue.finish();
#endif

    // Prepare to run the kernel vaddSimple
    cl::Kernel kernel_vaddSimple = cl::Kernel(program, "vaddSimple");
    kernel_vaddSimple.setArg(0, b_A);
    kernel_vaddSimple.setArg(1, b_B);
    kernel_vaddSimple.setArg(2, b_C);
    queue.enqueueNDRangeKernel(kernel_vaddSimple, cl::NullRange, cl::NDRange(M_VECTOR_SIZE), cl::NullRange);
    queue.finish();

    // Get the results out of the device, back to the host
    float C[M_VECTOR_SIZE];
    queue.enqueueReadBuffer(b_C, CL_TRUE, 0, sizeof(float)*M_VECTOR_SIZE, C);

#if !HOST_VECINIT
    queue.enqueueReadBuffer(b_A, CL_TRUE, 0, sizeof(float)*M_VECTOR_SIZE, A);
    queue.enqueueReadBuffer(b_B, CL_TRUE, 0, sizeof(float)*M_VECTOR_SIZE, B);
#endif

    // Print the results
    std::cout << "\n\nResults:\n=================================================\n";
    for (int i=0; i<M_VECTOR_SIZE; i++) {
        std::cout << A[i] << "\t\t" << B[i] << "\t\t" << C[i] << std::endl;
    }
    std::cout << "=================================================\n";

    return 0;*/
}
