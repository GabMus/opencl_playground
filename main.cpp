#include <iostream>
#include <fstream>

// These are for the random generation
#include <cstdlib>
#include <ctime>

#include <CL/cl.hpp>

#define TERM_BOLD "\033[1m"
#define TERM_GREEN "\x1b[32m"
#define TERM_RED "\x1b[31m"
#define TERM_CYAN "\033[36m"
#define TERM_RESET "\x1b[0m"

#define M_VECTOR_SIZE 4096
#define MAX_FLOAT 2048

// Switch to init the vectors on host
#define HOST_VECINIT 0

std::string read_kernel(std::string kernel_path) {
    // Read the kernel file and return it as string;
    std::ifstream sourceFile(kernel_path);
    std::string sourceCode( (std::istreambuf_iterator<char>(sourceFile)), (std::istreambuf_iterator<char>()));
    return sourceCode;
}

int main(int argc, char** argv) {
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

    // Context definition
    cl::Context context({default_device});

    // Define the sources for device code
    cl::Program::Sources sources;
    // TODO: fix this path, absolute is not needed
#if !HOST_VECINIT
    std::string kernel_code_vecinit = read_kernel("/home/gabmus/Development/OpenCL/QtCreator/vadd_test_ocl/vecinit_kernel.cl");
    sources.push_back({kernel_code_vecinit.c_str(), kernel_code_vecinit.length()});
#endif
    std::string kernel_code_vaddSimple = read_kernel("/home/gabmus/Development/OpenCL/QtCreator/vadd_test_ocl/vaddSimple_kernel.cl");
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
    // TODO: initialize the vector in the device instead of the host
    for (int i=0; i<M_VECTOR_SIZE; i++) {
        A[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX/MAX_FLOAT);
        B[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX/MAX_FLOAT);
    }
#endif
    // Create queue, and write arrays to device buffer in this queue
    cl::CommandQueue queue(context,default_device);
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

    return 0;
}
