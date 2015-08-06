#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ocl_platformid=-1;
int ocl_deviceid=-1;

cl_context ocl_context;
cl_device_id ocl_devices[100];
cl_command_queue ocl_command_queue;
//cl_event kernel_completion;

cl_program	ocl_kernel_program;

char* CL_ERR_STRING(int i)
{
	const char *CL_ERR_STR_LIST[64]={"CL_SUCCESS","CL_DEVICE_NOT_FOUND","CL_DEVICE_NOT_AVAILABLE","CL_COMPILER_NOT_AVAILABLE","CL_MEM_OBJECT_ALLOCATION_FAILURE","CL_OUT_OF_RESOURCES","CL_OUT_OF_HOST_MEMORY","CL_PROFILING_INFO_NOT_AVAILABLE","CL_MEM_COPY_OVERLAP","CL_IMAGE_FORMAT_MISMATCH","CL_IMAGE_FORMAT_NOT_SUPPORTED","CL_BUILD_PROGRAM_FAILURE","CL_MAP_FAILURE","CL_MISALIGNED_SUB_BUFFER_OFFSET","CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST","CL_INVALID_VALUE","CL_INVALID_DEVICE_TYPE","CL_INVALID_PLATFORM","CL_INVALID_DEVICE","CL_INVALID_CONTEXT","CL_INVALID_QUEUE_PROPERTIES","CL_INVALID_COMMAND_QUEUE","CL_INVALID_HOST_PTR","CL_INVALID_MEM_OBJECT","CL_INVALID_IMAGE_FORMAT_DESCRIPTOR","CL_INVALID_IMAGE_SIZE","CL_INVALID_SAMPLER","CL_INVALID_BINARY","CL_INVALID_BUILD_OPTIONS","CL_INVALID_PROGRAM","CL_INVALID_PROGRAM_EXECUTABLE","CL_INVALID_KERNEL_NAME","CL_INVALID_KERNEL_DEFINITION","CL_INVALID_KERNEL","CL_INVALID_ARG_INDEX","CL_INVALID_ARG_VALUE","CL_INVALID_ARG_SIZE","CL_INVALID_KERNEL_ARGS","CL_INVALID_WORK_DIMENSION","CL_INVALID_WORK_GROUP_SIZE","CL_INVALID_WORK_ITEM_SIZE","CL_INVALID_GLOBAL_OFFSET","CL_INVALID_EVENT_WAIT_LIST","CL_INVALID_EVENT","CL_INVALID_OPERATION","CL_INVALID_GL_OBJECT","CL_INVALID_BUFFER_SIZE","CL_INVALID_MIP_LEVEL","CL_INVALID_GLOBAL_WORK_SIZE"};

	i=abs(i);
	if(i>=30)i-=16;

	if(abs(i)<64-16) return (char*)CL_ERR_STR_LIST[abs(i)];
	return "Undefined";
}

#define CL_CHECK(_expr)                                                         \
   {                                                                         \
     cl_int _err = _expr;                                                       \
     if (_err != CL_SUCCESS)                                                    \
		error_stop("OpenCL Error %d: %s \n\n%s",(int)_err, CL_ERR_STRING((int)_err), #_expr);\
   }

#define CL_CHECK_ERR(_ret , _expr){\
	cl_int _err = CL_INVALID_VALUE;\
	_ret = _expr;\
	if (_err != CL_SUCCESS)\
		error_stop("OpenCL Error %d: %s \n\n%s",(int)_err, CL_ERR_STRING((int)_err), #_expr);\
}

cl_program ocl_compile(const char* program_source)
{
	cl_program program;
	CL_CHECK_ERR(program , clCreateProgramWithSource(ocl_context, 1, &program_source, NULL, &_err));
	if (clBuildProgram(program, 1, ocl_devices, "-cl-fast-relaxed-math", NULL, NULL) != CL_SUCCESS) {
		char buffer[10240];
		clGetProgramBuildInfo(program, ocl_devices[ocl_deviceid], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
		error_stop( "CL Compilation failed:\n%s", buffer);
	}
	CL_CHECK(clUnloadCompiler());
	
	return program;
}

void ocl_init()
{
	cl_platform_id platforms[100];
	cl_uint platforms_n = 0;
	CL_CHECK(clGetPlatformIDs(100, platforms, &platforms_n));

	printf(">> %d OpenCL platform(s) found: \n", platforms_n);
	if (platforms_n == 0) error_stop("No OpenCL Platform found !");

	for (int i=0; i<platforms_n; i++)
	{
		char buffer[10240];
		printf("  > %d \n", i);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, 10240, buffer, NULL));
		printf("  PROFILE = %s\n", buffer);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION, 10240, buffer, NULL));
		printf("  VERSION = %s\n", buffer);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 10240, buffer, NULL));
		printf("  NAME = %s\n", buffer);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 10240, buffer, NULL));
		printf("  VENDOR = %s\n", buffer);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, 10240, buffer, NULL));
		printf("  EXTENSIONS = %s\n", buffer);
	}

	cl_uint ocl_devices_n = 0;

	bool init_success=false;

	loopj(0,platforms_n) 
	{
		printf("> %d \n", j);
		CL_CHECK(clGetDeviceIDs(platforms[j], /*CL_DEVICE_TYPE_ALL*/CL_DEVICE_TYPE_GPU, 100, ocl_devices, &ocl_devices_n));
		printf(">> Try platform %d ( %d device(s) )\n", j,ocl_devices_n);

		if (ocl_devices_n>0) ocl_platformid=j; else continue;
		if (ocl_devices_n == 0) continue;//error_stop("No OpenCL Device found !");
		if (ocl_platformid < 0) continue;//error_stop("No OpenCL Device found !");

		for (int i=0; i<ocl_devices_n; i++)
		{
			char buffer[10240];
			cl_uint buf_uint;
			cl_ulong buf_ulong;
			printf("  > %d\n", i);
			CL_CHECK(clGetDeviceInfo(ocl_devices[i], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL));
			printf("  DEVICE_NAME = %s\n", buffer);
			CL_CHECK(clGetDeviceInfo(ocl_devices[i], CL_DEVICE_VENDOR, sizeof(buffer), buffer, NULL));
			printf("  DEVICE_VENDOR = %s\n", buffer);
			CL_CHECK(clGetDeviceInfo(ocl_devices[i], CL_DEVICE_VERSION, sizeof(buffer), buffer, NULL));
			printf("  DEVICE_VERSION = %s\n", buffer);
			CL_CHECK(clGetDeviceInfo(ocl_devices[i], CL_DRIVER_VERSION, sizeof(buffer), buffer, NULL));
			printf("  DRIVER_VERSION = %s\n", buffer);
			CL_CHECK(clGetDeviceInfo(ocl_devices[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(buf_uint), &buf_uint, NULL));
			printf("  DEVICE_MAX_COMPUTE_UNITS = %u\n", (unsigned int)buf_uint);
			CL_CHECK(clGetDeviceInfo(ocl_devices[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(buf_uint), &buf_uint, NULL));
			printf("  DEVICE_MAX_CLOCK_FREQUENCY = %u\n", (unsigned int)buf_uint);
			CL_CHECK(clGetDeviceInfo(ocl_devices[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL));
			printf("  DEVICE_GLOBAL_MEM_SIZE = %llu\n", (unsigned long long)buf_ulong);
			CL_CHECK(clGetDeviceInfo(ocl_devices[i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL));
			printf("  CL_DEVICE_LOCAL_MEM_SIZE = %llu\n", (unsigned long long)buf_ulong);
	
			cl_context_properties props[] = 
			{
				CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(), 
				CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(), 
				CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[ocl_platformid], 
				0
			};
			cl_int err;
			ocl_context=clCreateContext(props, 1, ocl_devices, NULL, NULL, &err);
			if(err==CL_SUCCESS){ ocl_deviceid=i; init_success=true;}
			//CL_CHECK_ERR(ocl_context,clCreateContext(props, 1, ocl_devices, NULL, NULL, &_err));

			if(init_success) break;
		}
		if(init_success) break;
	}
	
	printf("\nSelected : platform %d device %d\n\n",ocl_platformid,ocl_deviceid);

	if(ocl_deviceid<0) error_stop("No compatible OpenCL Device found.");

	CL_CHECK_ERR(ocl_command_queue,clCreateCommandQueue(ocl_context, ocl_devices[ocl_deviceid], 0, &_err));

	// general kernel source

	char *ocl_program_source = file_read("../kernel/kernel.cl");
	ocl_kernel_program = ocl_compile(ocl_program_source); 
}

cl_kernel ocl_get_kernel(char* name)
{
	cl_kernel k;
	CL_CHECK_ERR(k,clCreateKernel(ocl_kernel_program, name, &_err));
	return k;
}

cl_mem ocl_pbo_map(GLuint pbo)
{
	cl_mem mem;
	CL_CHECK_ERR(mem,clCreateFromGLBuffer(ocl_context, CL_MEM_READ_WRITE, pbo, &_err));
	//glFlush();
    //CL_CHECK(clEnqueueAcquireGLObjects(ocl_command_queue,1, &mem, 0, NULL, NULL));
	return mem;
}
void ocl_pbo_unmap(cl_mem& mem)
{
    //clEnqueueReleaseGLObjects(ocl_command_queue,1, &mem, 0, NULL, NULL);
}

void ocl_exit()
{
	CL_CHECK(clReleaseProgram(ocl_kernel_program));
	//CL_CHECK(clUnloadCompiler());
	clFinish(ocl_command_queue);
	CL_CHECK(clReleaseContext(ocl_context));
}

void ocl_pbo_begin(cl_mem& mem)
{
	glFlush();
    CL_CHECK(clEnqueueAcquireGLObjects(ocl_command_queue,1, &mem, 0, NULL, NULL));
}

void ocl_pbo_end(cl_mem& mem)
{
    CL_CHECK(clEnqueueReleaseGLObjects(ocl_command_queue,1, &mem, 0, NULL, NULL));
}

// Round Up Division function
size_t ocl_round_up(int group_size, int global_size) 
{
    int r = global_size % group_size;
    if(r == 0) 
    {
        return global_size;
    } else 
    {
        return global_size + group_size - r;
    }
}

cl_mem ocl_malloc(uint size,void* ptr=0, uint flags=CL_MEM_READ_WRITE)
{
	if( ptr ) flags |= CL_MEM_COPY_HOST_PTR;
	if(size==0) return 0;

	cl_mem m;
	CL_CHECK_ERR( m , 
		clCreateBuffer(ocl_context, 
			flags , 
			size, 
			ptr, 
			&_err));
	return m;
}

void ocl_copy_to_host(void* dst, cl_mem &src, size_t size, size_t srcofs=0)
{
	cl_event kernel_completion;
	CL_CHECK(clEnqueueReadBuffer(ocl_command_queue, src, CL_TRUE, srcofs, size, dst, 0, NULL, &kernel_completion));
	CL_CHECK(clWaitForEvents(1, &kernel_completion));
	CL_CHECK(clReleaseEvent(kernel_completion));
}


int _ocl_arg;
size_t _ocl_globalworksize[2];
size_t _ocl_localworksize[2];
cl_kernel* _ocl_current_kernel;

void ocl_begin(cl_kernel* kernel, int globalx, int globaly, int localx,int localy)
{
	_ocl_arg=0;
	_ocl_localworksize[0]=localx;
	_ocl_localworksize[1]=localy;
	_ocl_globalworksize[0] = ocl_round_up((int)_ocl_localworksize[0], globalx);
	_ocl_globalworksize[1] = ocl_round_up((int)_ocl_localworksize[1], globaly);
	_ocl_current_kernel = kernel;
}
void ocl_param(size_t size, void* ptr)
{
	CL_CHECK(clSetKernelArg(*_ocl_current_kernel, _ocl_arg++, size, ptr));
}

int ocl_kernel_completion_count=0;
cl_event ocl_kernel_completion[100];

void ocl_begin_all_kernels()
{
	ocl_kernel_completion_count=0;
}

#define OCL_WAIT_COMPLETE

void ocl_end_all_kernels()
{
#ifdef OCL_WAIT_COMPLETE
	if(ocl_kernel_completion_count==0) return;
		
	CL_CHECK(clWaitForEvents(1, &ocl_kernel_completion[ocl_kernel_completion_count-1]));

	loopi(0,ocl_kernel_completion_count)
	{		
		CL_CHECK(clReleaseEvent(ocl_kernel_completion[i]));
	}	
#endif
}

#ifdef OCL_WAIT_COMPLETE
#define ocl_end()\
{\
	cl_event kernel_completion;\
	CL_CHECK(clEnqueueNDRangeKernel(ocl_command_queue, *_ocl_current_kernel, 2, NULL,\
										_ocl_globalworksize, _ocl_localworksize, 0, NULL,\
										&ocl_kernel_completion[ocl_kernel_completion_count++]));\
}
#else
#define ocl_end()\
{\
	cl_event kernel_completion;\
	CL_CHECK(clEnqueueNDRangeKernel(ocl_command_queue, *_ocl_current_kernel, 2, NULL,\
										_ocl_globalworksize, _ocl_localworksize, 0, NULL,NULL));\
}
#endif


void ocl_memcpy(cl_mem& dst, uint dstofs,cl_mem &src, uint srcofs,uint size)
{
	static cl_kernel kernel=ocl_get_kernel("memcpy");
	srcofs/=4;
	dstofs/=4;
	size/=4;
	ocl_begin(&kernel,size,1,256,1);
	ocl_param(sizeof(cl_mem), &dst);
	ocl_param(sizeof(cl_uint), &dstofs);
	ocl_param(sizeof(cl_mem), &src);
	ocl_param(sizeof(cl_uint), &srcofs);
	ocl_end();
}

void ocl_memset(cl_mem& dst, uint dstofs,uint val,uint size)
{
	dstofs/=4;
	size/=4;
	static cl_kernel kernel=ocl_get_kernel("memset");
	ocl_begin(&kernel,size,1,256,1);
	ocl_param(sizeof(cl_mem), &dst);
	ocl_param(sizeof(cl_uint), &dstofs);
	ocl_param(sizeof(cl_uint), &val);
	ocl_end();
}
