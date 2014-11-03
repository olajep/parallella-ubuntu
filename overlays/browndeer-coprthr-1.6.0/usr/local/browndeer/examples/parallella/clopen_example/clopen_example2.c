/* clopen_example2.c 
 *
 * This is a simple showing how to open a kernel file but delay JIT compilation
 * in order to pass in explicit compiler options.
 *
 * THIS FILE ONLY is placed in the public domain by Brown Deer Technology, LLC.
 * in January 2013. No copyright is claimed, and you may use it for any purpose
 * you like. There is ABSOLUTELY NO WARRANTY for any purpose, expressed or
 * implied, and any warranty of any kind is explicitly disclaimed.  This
 * statement applies ONLY TO THE COMPUTER SOURCE CODE IN THIS FILE and does
 * not extend to any other software, source code, documentation, or any other
 * materials in which it may be included, or with which it is co-distributed.
 */

/* DAR */


#include <stdio.h>
#include <stdcl.h>

int main()
{
   cl_uint n = 1024;

	/* use default contexts, if no ACCELERATOR use CPU */
   CLCONTEXT* cp = (stdacc)? stdacc : stdcpu;

   unsigned int devnum = 0;

	/******************************************************************
	 *** this example requires the .cl file to be available at run-time
	 *** and shows how to pass compiler options to the OCL compiler
	 ******************************************************************/

   void* clh = clopen(cp,"outerprod.cl",CLLD_NOBUILD);
   clbuild(cp,clh,"-D COEF=2", 0);
   cl_kernel krn = clsym(cp,clh,"outerprod_kern",0);

	if (!krn) { fprintf(stderr,"error: no OpenCL kernel\n"); exit(-1); }

   /* allocate OpenCL device-sharable memory */
   cl_float* a = (float*)clmalloc(cp,n*sizeof(cl_float),0);
   cl_float* b = (float*)clmalloc(cp,n*sizeof(cl_float),0);
   cl_float* c = (float*)clmalloc(cp,n*sizeof(cl_float),0);

   /* initialize vectors a[] and b[], zero c[] */
   int i; 
   for(i=0;i<n;i++) a[i] = 1.1f*i;
   for(i=0;i<n;i++) b[i] = 2.2f*i;
   for(i=0;i<n;i++) c[i] = 0.0f;

   /* non-blocking sync vectors a and b to device memory (copy to GPU)*/
   clmsync(cp,devnum,a,CL_MEM_DEVICE|CL_EVENT_WAIT);
   clmsync(cp,devnum,b,CL_MEM_DEVICE|CL_EVENT_WAIT);

   /* define the computational domain and workgroup size */
   clndrange_t ndr = clndrange_init1d( 0, n, 16);

   /* set the kernel arguments */
   clarg_set_global(cp,krn,0,a);
   clarg_set_global(cp,krn,1,b);
   clarg_set_global(cp,krn,2,c);

   /* non-blocking fork of the OpenCL kernel to execute on the GPU */
   clfork(cp,devnum,krn,&ndr,CL_EVENT_NOWAIT);

   /* block on completion of operations in command queue */
   clwait(cp,devnum,CL_ALL_EVENT);

   /* non-blocking sync vector c to host memory (copy back to host) */
   clmsync(cp,0,c,CL_MEM_HOST|CL_EVENT_NOWAIT);

   /* block on completion of operations in command queue */
   clwait(cp,devnum,CL_ALL_EVENT);

   for(i=0;i<n;i++) printf("%d %f %f %f\n",i,a[i],b[i],c[i]);

   clfree(a);
   clfree(b);
   clfree(c);

   clclose(cp,clh);
}
