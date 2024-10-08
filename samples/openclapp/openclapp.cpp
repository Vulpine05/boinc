// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2013 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.
//
// This program serves as both
// - An example BOINC-OpenCL application, illustrating the use of the BOINC API
//   and OpenCL API.
// - A program for testing various features of BOINC.
//
// The program reads the input nxn matrix from the "input" file, inverts the
// matrix NUM_ITERATIONS times and write to "output" file.
//
// To run, place the executable in the same directory as an init_data.xml
// file specifying the gpu_type (vendor) and gpu_device_num, then invoke
// form the command line as follows:
// $ cd to/the/directory/containing/executable/and/init_data.mxl/file
// $ ./openclapp [options]
//
// command line options
// -run_slow: sleep 1 second after each character
// -cpu_time N: use about N CPU seconds after copying files
// -early_exit: exit(10) after 30 iterations
// -early_crash: crash after 30 iterations
//
// See https://github.com/BOINC/boinc/wiki/GPUApp
// Original contributor: Tuan Le (tuanle86@berkeley.edu)

#include "openclapp.hpp"
#include "boinc_opencl.h"

using std::string;

int main(int argc, char * argv[]) {
    int i, retval, lastInversion=0, checkpointExists=0, matrixSize=0;
    double fd;
    char input_path[512], output_path[512], chkpt_path[512], buf[256];
    MFILE out;
    FILE* state, *infile;

    generate_random_input_file(MATRIX_SIZE); //call this if you don't want to
                                             //construct the input file manually
	for (i=0; i<argc; i++) {
        if (!strcmp(argv[i], "-early_exit")) early_exit = true;
        if (!strcmp(argv[i], "-early_crash")) early_crash = true;
        if (!strcmp(argv[i], "-run_slow")) run_slow = true;
        if (!strcmp(argv[i], "-cpu_time")) {
            cpu_time = atof(argv[++i]);
        }
    }

    retval = boinc_init();
    if (retval) {
        fprintf(stderr,
            "ERROR: %s boinc_init returned %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), retval
        );
        exit(retval);
    }

    // open the input file (resolve logical name first)
    //
    boinc_resolve_filename(INPUT_FILENAME, input_path, sizeof(input_path));
    infile = boinc_fopen(input_path, "r");
    if (!infile) {
        fprintf(stderr,
            "ERROR: %s Couldn't find input file, resolved name %s.\n",
            boinc_msg_prefix(buf, sizeof(buf)), input_path
        );
        getchar();
        exit(-1);
    }

    boinc_resolve_filename(OUTPUT_FILENAME, output_path, sizeof(output_path));

    // See if there's a valid checkpoint file.
    // If so retrieve the current matrix and inversion number
    //
    boinc_resolve_filename(CHECKPOINT_FILE, chkpt_path, sizeof(chkpt_path));
    state = boinc_fopen(chkpt_path, "r");
    if (state) {
        printf("Checkpoint file is detected. Read from checkpoint file ... \n");
        checkpointExists=fscanf(state, "%d", &lastInversion);
        if (checkpointExists == 1) {
            isStateFileInUse=true;
            printf("Last inversion # is : %d\n",lastInversion);
            fscanf(state,"%d",&matrixSize);
            width=height=matrixSize;
            printf("Initialize host ....\n");
            initialize_host(state);
        }
        fclose(state);
    } else {
        printf("There's no valid checkpoint file!\n");
    }

    retval = out.open(output_path, "wb");

    if (retval) {
        fprintf(stderr,
            "ERROR: %s APP: matrix_inversion output open failed:\n",
            boinc_msg_prefix(buf, sizeof(buf))
        );
        fprintf(stderr,
            "ERROR: %s resolved name %s, retval %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), output_path, retval
        );
        perror("open");
        exit(1);
    }

#ifdef APP_GRAPHICS
    // create shared mem segment for graphics, and arrange to update it
    //
    shmem = (UC_SHMEM*)boinc_graphics_make_shmem("matrix_inversion", sizeof(UC_SHMEM));
    if (!shmem) {
        fprintf(stderr,
            "ERROR: %s failed to create shared mem segment\n",
            boinc_msg_prefix(buf, sizeof(buf))
        );
    }
    update_shmem();
    boinc_register_timer_callback(update_shmem);
#endif

    if (checkpointExists != 1) { //checkpoint file is not found.
        matrixSize=get_matrix_size(infile);
        printf("Matrix Size: width = height = %d\n",matrixSize);
        width=height=matrixSize;
        // Initialize Host application
        printf("Initialize host ....\n");
        if (initialize_host(infile)==1) {
            return 1;
        }
        out.printf("\n----------------- Before being inversed ----------------\n\n");
        printf("Computation is running ... Inverse the matrix %d times. Start at inversion #1\n",
               NUM_ITERATIONS);
    } else {
        out.printf("\n----------------- Last checkpointed inversion #%d ----------------\n\n",
                   lastInversion);
        printf("Computation is resumed ... Inverse the matrix %d more times. Start at inversion #%d\n",
               NUM_ITERATIONS-lastInversion,lastInversion+1);
    }

    // Initialize OpenCL resources
    if (initialize_cl(argc, argv) != 0) {
        return 1;
    }

    print_to_file(&out,input,matrixSize);

    for (i=lastInversion+1;i<=NUM_ITERATIONS;++i) {
        //the invert function will trigger kernel calls.
        invert(input,output,matrixSize);
        printf("Finish inversion #%d\n",i);
        for (int j=0;j<matrixSize*matrixSize;++j) {
            input[j]=output[j]; //change the input for the next iteration
        }
        if (run_slow) {
            boinc_sleep(1.);
        }

        if (early_exit && i>30) {
            exit(-10);
        }

        if (early_crash && i>30) {
            boinc_crash();
        }

        if (boinc_time_to_checkpoint()) {
            printf("Perform checkpointing at inversion # %d\n",i);
            //we'll need to write the current matrix to the state file.
            retval = do_checkpoint(out, i, input, matrixSize);
            if (retval) {
                fprintf(stderr,
                    "ERROR: %s APP: matrix_inversion checkpoint failed %d\n",
                    boinc_msg_prefix(buf, sizeof(buf)), retval
                );
                exit(retval);
            }
            boinc_checkpoint_completed();
        }
        fd = i/NUM_ITERATIONS;
        if (cpu_time) fd /= 2;
        boinc_fraction_done(fd);
    }

    out.printf("\n\n----------------- Final inversion #%d ----------------\n\n",
               NUM_ITERATIONS);
    print_to_file(&out,output,matrixSize);

    retval = out.flush(); //force the output file to be closed.
    if (retval) {
        fprintf(stderr,
            "ERROR: %s APP: matrix_inversion flush failed %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), retval
        );
        exit(1);
    }

    // Releases OpenCL resources
    if (cleanup_cl()==1) {
        fprintf(stderr, "Error from cleanup_cl() !");
        return 1;
    }

    // Release host resources
    cleanup_host();

    // burn up some CPU time if needed
    //
    if (cpu_time) {
        printf("\nBurning up some CPU time ... \n");
        double start = dtime();
        for (i=0; ; i++) {
            double e = dtime()-start;
            if (e > cpu_time) break;
            fd = .5 + .5*(e/cpu_time);
            boinc_fraction_done(fd);

            if (boinc_time_to_checkpoint()) {
                retval = do_checkpoint(out, NUM_ITERATIONS, input, matrixSize);
                if (retval) {
                    fprintf(stderr,
                        "ERROR: %s APP: maxtrix_inversion checkpoint failed %d\n",
                        boinc_msg_prefix(buf, sizeof(buf)), retval
                    );
                    exit(1);
                }
                boinc_checkpoint_completed();
            }
            comp_result = do_a_giga_flop(i);
        }
    }
    boinc_fraction_done(1);

#ifdef APP_GRAPHICS
    update_shmem();
#endif

    if (boinc_is_standalone()) {
        printf("\nDone! Please press ENTER to exit. ");
        getchar();
    }
    boinc_finish(0);
}


/*** BOINC FUNCTION DEFINITIONS ***/

/* Do a billion floating-point ops */
static double do_a_giga_flop(int foo) {
    double x = 3.14159*foo;
    int i;
    for (i=0; i<500000000; i++) {
        x += 5.12313123;
        x *= 0.5398394834;
    }
    return x;
}

/* Save the computation state into checkpoint file */
int do_checkpoint(MFILE& mf, int n, cl_float *in, int matrixSize) {
    int retval;
    string resolved_name;

    FILE* f = fopen("temp", "w");
    if (!f) return 1;
    fprintf(f, "%d", n); //write inversion number
    fprintf(f, " ");
    fprintf(f, "%d", matrixSize); //write matrixSize
    fprintf(f, " ");
    for (int i=0;i<matrixSize*matrixSize;++i) {
        fprintf(f, " ");
        fprintf(f, "%f", in[i]);
    }
    fclose(f);
    retval = mf.flush();
    if (retval) return retval;
    boinc_resolve_filename_s(CHECKPOINT_FILE, resolved_name);
    retval = boinc_rename("temp", resolved_name.c_str());
    if (retval) return retval;
    return 0; //return 0 to indicate success.
}

/*** FUNCTION DEFINITIONS ***/

/* Create an input file filled with random data of type cl_float. */
void generate_random_input_file(int n) {
    FILE *infile;

    infile=fopen(INPUT_FILENAME,"w");
    cl_float *in = (cl_float *)malloc(sizeof(cl_float)*(n*n));
    srand(n);
    for( int i = 0; i < n; i++ ) {
        for (int j = 0; j < n; j++) {
            in[i*n+j] = 2.0*(rand()%32768)/32768.0 - 1.0;
        }
        in[i*n+i] += sqrt((float)n);
    }
    int j=0;
    for (int i=0;i<n*n;++i) {
        fprintf(infile,"%15f",in[i]);
        if (j+1==n) {
            fprintf(infile,"\n");
            j=0;
        } else {
            ++j;
        }
    }
    fclose(infile);
    free(in);
}

/*
 * Parse the input file and determine the size of the matrix.
 * This is an nxn matrix. Note: if width<> height, the matrix is
 * non-invertible.
 */
int get_matrix_size(FILE *infile) {
    int w=0;
    char c;

    fseek(infile,0,SEEK_SET);
    while (true) {
        do {
            c=fgetc(infile);
            if (c == EOF || c == '\n') {
                goto exitLoop;
            }
        } while (isspace(c));

        if (isdigit(c) || c=='.' || c=='-') {
            ++w;
        }

        do {
            c=fgetc(infile);
            if (c == EOF || c == '\n') {
                goto exitLoop;
            }
        } while (isdigit(c) || c=='.' || c=='-');

        if (c==EOF || c == '\n') {
            break;
        }
    }
    exitLoop:
    return w;
}

/*
 * \brief Host Initialization
 *        Allocate and initialize memory
 *        on the host. Print input array.
 */
int initialize_host(FILE *infile) {
    input  = NULL;
    output = NULL;

    if (width!=height) {
        fprintf(stderr, "Error: non nxn matrix cannot be invertiable.\n");
        return 1;
    }

    /////////////////////////////////////////////////////////////////
    // Allocate and initialize memory used by host
    /////////////////////////////////////////////////////////////////
    cl_uint sizeInBytes = width * height * sizeof(cl_float);
    input = (cl_float *) malloc(sizeInBytes);
    if (input == NULL) {
        fprintf(stderr, "Error: Failed to allocate input memory on host\n");
        return 1;
    }

    output = (cl_float *) malloc(sizeInBytes);
    if(output == NULL) {
        fprintf(stderr, "Error: Failed to allocate output memory on host\n");
        return 1;
    }

    //fillRandom(input,width,height);
    fetch_elements_into_host_memory(infile,input);
    return 0;
}

/*
 * Read the float values from input file into "input" array.
 */
void fetch_elements_into_host_memory(FILE *infile, cl_float *in) {
    float num=0;
    int i=0;
    if (!isStateFileInUse) {
        fseek(infile,0,SEEK_SET);
    }
    while (fscanf(infile,"%f",&num)==1) {
        in[i]=num;
        ++i;
    }
}

/*
 * Converts the contents of a file into a string
 */
char * convert_to_string(const char *fileName) {
    int count=0;
    char *s;
    char c;
    int i=0;

    // look for "openclapp_kernels.cl" in "boinc/samples/openclapp/debug" or
    // in "boinc/samples/openclapp/release". Note that "openclapp_kernels.cl"
    // is automatically copied to these directories along the building process.
    FILE *infile=fopen(fileName,"r");
    if (!infile) { //not found. This typically happens on Linux or Mac.
        //look for "openclapp_kernels.cl" in "boinc/sample/openclapp/" instead.
        infile = fopen(KERNELS_FILEPATH,"r");
        if (!infile) {
            fprintf(stderr, "ERROR: Failed to open file %s!", fileName);
            exit(0);
        }
    }
    fseek(infile,0,SEEK_SET);
    while (fgetc(infile)!=EOF) count++;
    s=(char *) malloc(sizeof(char)*(count+1)); //add 1 for string terminator.
    fseek(infile,0,SEEK_SET);
    while ((c=fgetc(infile))!=EOF) {
        s[i++]=c;
    }
    s[i]='\0';
    fclose(infile);
    return s;
}

/*
 * \brief OpenCL related initialization
 *        Create Context, Device list, Command Queue
 *        Load CL file, compile, link CL source
 *		  Build program and kernel objects
 */

 // Note: OpenCL memory buffer objects will be created in invert
 //       function before kernel calls are made.
int initialize_cl(int argc, char * argv[]) {
    cl_int status = 0;
    int retval = 0;

    localThreads[0]  = LOCAL_WORK_SIZE;
    globalThreads[0] = GLOBAL_WORK_SIZE;
    cl_platform_id platform = NULL;
    cl_device_id device;

    // IMPORTANT NOTE: production applications should always specify
    // the GPU type (vendor) in the call to boinc_get_opencl_ids as
    // the third argument: it must be either PROC_TYPE_NVIDIA_GPU,
    // PROC_TYPE_AMD_GPU or PROC_TYPE_INTEL_GPU.  This is to support
    // older versions of the BOINC client which do not include the
    // <gpu-type> field in the init_data.xml file.
    //
    // This sample passes -1 for the type argument to allow using
    // just one sample for any GPU vendor (AMD, NVIDIA or Intel.)
    // As a result, the init_data.xml file for this sample must
    // specify the GPU type (vendor) and either gpu_device_num (the
    // GPU's index from that vendor) or gpu_opencl_dev_index (the
    // GPU's index among OpenCL-capable devices from that vendor.)
    //
    // See the ReadMe file for more details, including an explanation
    // of the difference between the gpu_device_num and the
    // gpu_opencl_dev_index.
    retval = boinc_get_opencl_ids(argc, argv, -1, &device, &platform);
    if (retval) {
        fprintf(stderr,
            "Error: boinc_get_opencl_ids() failed with error %d\n",
            retval
        );
        return 1;
    }

   cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM,
                                     (cl_context_properties)platform,
                                     0
                                   };
    context = clCreateContext(cps, 1, &device, NULL, NULL, &status);
    if (status != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateContext() returned %d\n", status);
        return 1;
    }

    /////////////////////////////////////////////////////////////////
    // Create an OpenCL command queue
    /////////////////////////////////////////////////////////////////
    commandQueue = clCreateCommandQueue(context, device, 0, &status);
    if(status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Creating Command Queue. (clCreateCommandQueue) returned %d\n",
            status
        );
        return 1;
    }

    /////////////////////////////////////////////////////////////////
    // Load CL file, build CL program object, create CL kernel object
    /////////////////////////////////////////////////////////////////
    source = convert_to_string(KERNELS_FILENAME);
    size_t sourceSize[]    = { strlen(source) };
    program = clCreateProgramWithSource(context, 1, &source, sourceSize, &status);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Loading Binary into cl_program (clCreateProgramWithBinary) returned %d\n",
            status
        );

        return 1;
    }

    /* create a cl program executable for all the devices specified */
    status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (status != CL_SUCCESS)  {
        fprintf(stderr,
            "Error: Building Program (clBuildProgram) returned %d\n",
            status
        );

        return 1;
    }

    /* get a kernel object handle for a kernel with the given name */
    GEStep1A_kernel = clCreateKernel(program, "GEStep1A", &status);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: clCreateKernel (GEStep1A) returned %d\n",
            status
        );

        return 1;
    }

    GEStep2_kernel = clCreateKernel(program, "GEStep2", &status);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: clCreateKernel (GEStep2) returned %d\n",
            status
        );

        return 1;
    }

	GEStep3_kernel = clCreateKernel(program, "GEStep3", &status);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: clCreateKernel (GEStep3) returned %d\n",
            status
        );

        return 1;
    }

    return 0;
}

/*
 * \brief Release OpenCL resources (Context, Memory etc.)
 */
int cleanup_cl(void) {
    cl_int status;

    status = clReleaseKernel(GEStep1A_kernel);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: In clReleaseKernel (GEStep1A_kernel) returned %d\n",
            status
        );
        return 1;
    }

    status = clReleaseKernel(GEStep2_kernel);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: In clReleaseKernel (GEStep2_kernel) returned %d\n",
            status
        );
        return 1;
	}

    status = clReleaseKernel(GEStep3_kernel);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: In clReleaseKernel (GEStep3_kernel) returned %d\n",
            status
        );
        return 1;
    }

    status = clReleaseProgram(program);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: clReleaseProgram returned %d\n",
            status
        );
        return 1;
    }

    status = clReleaseCommandQueue(commandQueue);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: In clReleaseCommandQueue returned %d\n",
            status
        );
        return 1;
    }

    status = clReleaseContext(context);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: In clReleaseContext returned %d\n",
            status
        );
        return 1;
    }

    return 0;
}

/*
 * \brief Releases program's resources
 */
void cleanup_host(void) {
    if (input != NULL) {
        free(input);
        input = NULL;
    }

    if (output != NULL) {
        free(output);
        output = NULL;
    }

    if (source != NULL) {
        free((char *)source);
        source = NULL;
    }
}

/*
 * Write the result to output file
 */
void print_to_file(MFILE *out, float *h_odata, int n) {
    int count=0;
    int move=0;
    int num_elements=n*n;
    while (num_elements>0) {
        out->printf("%15f    ",h_odata[move]);
        ++count;
        ++move;
        if (count==n) {
            out->printf("\n");
            count=0;
        }
        --num_elements;
    }
}

/*
 * \brief Run OpenCL program
 *
 *        Bind host variables to kernel arguments
 *		  Run the CL kernel
 */
int run_GEStep1A_kernel(cl_float * AI, int i, int n2, int lda2) {
    cl_int status;

    /*
     * the input array to the kernel. This array will eventually be modified
     * to the inverted array.
     */
    status = clSetKernelArg(GEStep1A_kernel, 0, sizeof(cl_mem), (void *)&inputBuffer);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (input) returned %d\n",
            status
        );
        return 1;
    }

    /*i*/
    status = clSetKernelArg(GEStep1A_kernel, 1, sizeof(int), (void *)&i);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (i) returned %d\n",
            status
        );
        return 1;
    }

    /*n2*/
    status = clSetKernelArg(GEStep1A_kernel, 2, sizeof(int), (void *)&n2);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (n2) returned %d\n",
            status
        );
        return 1;
    }

    /*lda2*/
    status = clSetKernelArg(GEStep1A_kernel, 3, sizeof(int), (void *)&lda2);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (lda2) returned %d\n",
            status
        );
        return 1;
    }

    /*
     * Enqueue a kernel run call.
     */
    status = clEnqueueNDRangeKernel(commandQueue,
                                    GEStep1A_kernel,
                                    1,
                                    NULL,
                                    globalThreads,
                                    localThreads,
                                    0,
                                    NULL,
                                    NULL);

    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Enqueueing kernel onto command queue. (clEnqueueNDRangeKernel) returned %d\n",
            status
        );
        return 1;
    }

    clFinish(commandQueue);

    /* Enqueue readBuffer*/  //Note: we are reading back from inputBuffer since AI is modified directly in kernel
    status = clEnqueueReadBuffer(commandQueue,
                                 inputBuffer,
                                 CL_FALSE,
                                 0,
                                 globalThreads[0] * sizeof(cl_float),
                                 AI,
                                 0,
                                 NULL,
                                 NULL);

    if(status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: clEnqueueReadBuffer failed. (clEnqueueReadBuffer) returned %d\n",
            status
        );
        return 1;
    }

    return 0;
}

int run_GEStep2_kernel(cl_float * AI, cl_float diag, int i, int n2, int lda2) {
    cl_int status;

    /*
     * the input array to the kernel. This array will eventually be modified
     * to the inverted array.
     */
    status = clSetKernelArg(GEStep2_kernel, 0, sizeof(cl_mem), (void *)&inputBuffer);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (AI) returned %d\n",
            status
        );
        return 1;
    }

    /*diag*/
    status = clSetKernelArg(GEStep2_kernel, 1, sizeof(cl_float), (void *)&diag);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (diag) returned %d\n",
            status
        );
        return 1;
    }

    /*i*/
    status = clSetKernelArg(GEStep2_kernel, 2, sizeof(int), (void *)&i);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (i) returned %d\n",
            status
        );
        return 1;
    }

    /*n2*/
    status = clSetKernelArg(GEStep2_kernel, 3, sizeof(int), (void *)&n2);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (n2) returned %d\n",
            status
        );
        return 1;
    }

    /*lda2*/
    status = clSetKernelArg(GEStep2_kernel, 4, sizeof(int), (void *)&lda2);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (lda2) returned %d\n",
            status
        );
        return 1;
    }

    /*
     * Enqueue a kernel run call.
     */
    status = clEnqueueNDRangeKernel(commandQueue,
                                    GEStep2_kernel,
                                    1,
									NULL,
                                    globalThreads,
                                    localThreads,
                                    0,
                                    NULL,
                                    NULL);

    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Enqueueing kernel onto command queue. (clEnqueueNDRangeKernel) returned %d\n",
            status
        );
        return 1;
    }

    clFinish(commandQueue);

    /* Enqueue readBuffer*/
	//Note: we are reading back from inputBuffer since AI is modified directly in kernel
    status = clEnqueueReadBuffer(commandQueue,
                                 inputBuffer,
                                 CL_FALSE,
                                 0,
                                 globalThreads[0] * sizeof(cl_float),
                                 AI,
                                 0,
                                 NULL,
                                 NULL);
    if (status != CL_SUCCESS) {
        fprintf(stderr, "Error: clEnqueueReadBuffer failed. (clEnqueueReadBuffer) returned %d\n", status);
        return 1;
    }

    return 0;
}

int run_GEStep3_kernel(cl_float * AI, int i, int n2, int lda2) {
    cl_int status;

    /*
     * The input array to the kernel. This array will eventually be modified
     * to the inverted array.
     */
    status = clSetKernelArg(GEStep3_kernel, 0, sizeof(cl_mem), (void *)&inputBuffer);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (input) returned %d\n",
            status
        );
        return 1;
    }

    /*i*/
    status = clSetKernelArg(GEStep3_kernel, 1, sizeof(int), (void *)&i);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (i) returned %d\n",
            status
        );
        return 1;
    }

    /*n2*/
    status = clSetKernelArg(GEStep3_kernel, 2, sizeof(int), (void *)&n2);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (n2) returned %d\n",
            status
        );
        return 1;
    }

	/*lda2*/
    status = clSetKernelArg(GEStep3_kernel, 3, sizeof(int), (void *)&lda2);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Setting kernel argument. (lda2) returned %d\n",
            status
        );
        return 1;
    }

    /*
     * Enqueue a kernel run call.
     */
    status = clEnqueueNDRangeKernel(commandQueue,
                                    GEStep3_kernel,
                                    1,
                                    NULL,
                                    globalThreads,
                                    localThreads,
                                    0,
                                    NULL,
                                    NULL);

    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: Enqueueing kernel onto command queue. (clEnqueueNDRangeKernel) returned %d\n",
            status
        );
        return 1;
    }

    clFinish(commandQueue);

    /* Enqueue readBuffer*/
	//Note: we are reading back from inputBuffer since AI is modified directly in kernel
    status = clEnqueueReadBuffer(commandQueue,
                                 inputBuffer,
                                 CL_TRUE,
                                 0,
                                 globalThreads[0] * sizeof(cl_float),
                                 AI,
                                 0,
                                 NULL,
                                 NULL);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: clEnqueueReadBuffer failed. (clEnqueueReadBuffer) returned %d\n",
            status
        );
        return 1;
    }

	return 0;
}

void invertge(cl_float * AI_d, int lda, int n) {
    int lda2 = lda * 2;
    // perform elementary row operations till A in AI becomes identity matrix
    for (int i = 0; i < n; i++) {
        // execute kernel
        run_GEStep1A_kernel(AI_d,i,n*2, lda2);
    }

    for (int i = n-1; i >= 0; i--) {
        cl_float diag = 1.0;
        diag=AI_d[i*lda2+i];
        // execute kernels
        run_GEStep2_kernel(AI_d,diag,i,n*2, lda2);
        run_GEStep3_kernel(AI_d,i,n*2, lda2);
    }
}

/* inverts nxn matrix input and stores the result in output */
void invert(cl_float * in, cl_float *out, int n) {
    printf("starting inversion n = %d ", n);
    volatile clock_t gputime;
    gputime=clock();

    int lda = (((n+15)&(~15))|16);
    cl_float * AI_d = (cl_float *)malloc(sizeof(cl_float)*n*lda*2);
    memset(AI_d,0,sizeof(cl_float)*n*lda*2);
    for (int i = 0; i < n; i++) {
        memcpy(&AI_d[lda*i*2], &in[n*i], sizeof(cl_float)*n);
        AI_d[lda*i*2+n+i] = 1;
    }

    cl_int status;

    /////////////////////////////////////////////////////////////////
    // Create OpenCL memory buffer
    /////////////////////////////////////////////////////////////////
    inputBuffer = clCreateBuffer(context,
                                 CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                                 sizeof(cl_float) * globalThreads[0],
                                 AI_d,
                                 &status);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: clCreateBuffer (inputBuffer) returned %d\n",
            status
        );
        exit(0);
    }

    // Note: there's no output buffer. In kernel, AI_d is modified directly.
	// Thus, we should read the result back to host from inputBuffer as well.
    invertge(AI_d, lda, n);
    gputime=clock()-gputime;
    fprintf(stderr, " %7.1f ms ",gputime/1.e3f);
    fprintf(stderr, " %7.2f Gflops", 1e-3*(3.0)*n*n*n/3.0/gputime);

#ifdef VERIFY
    // let's verify that
    cl_float error=0.0;

    // multiply inverse*xcopy, should be Identity matrix
    for (int k = 0; k < n; k++) {
        for (int j = 0; j < n; j++) {
            cl_float sum = 0;
            for (int i = 0; i < n; i++) {
                sum += AI[j*lda*2+n+i]*A[i*n+k];
	        }
            if (j!=k) {
                error += sum * sum;
            } else {
                error += (1.0-sum) * (1.0-sum);
            }
        }
    }
    fprintf(stderr, " %6.2f SSE", error);
#endif

    //copy the result to output
    for (int i = 0; i < n; i++) {
        memcpy(&out[n*i], &AI_d[lda*i*2+n], sizeof(cl_float)*n);
    }

    status = clReleaseMemObject(inputBuffer);
    if (status != CL_SUCCESS) {
        fprintf(stderr,
            "Error: In clReleaseMemObject (inputBuffer) returned %d\n",
            status
        );
    }

    free(AI_d);
    fprintf(stderr," done!\n");
}
