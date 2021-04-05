#if 0
#include <stdio.h>
__global__ void test() { printf("FuCK U NVIDIA!\n"); } int main(void) { test<<<1, 1>>>(); cudaDeviceSynchronize(); }
#else

#include "helper_cuda.h"
#include "helper_math.h"
#include "managed.cuh"
#include <cassert>
#include <cstdio>
#include <cmath>

const size_t Nx = 128;

template <class T>
__global__ void blur(T *arr)
{
  size_t ix = blockIdx.x * blockDim.x + threadIdx.x;
  if (ix < Nx) {
    *Subscriptor(*arr, ix).get() = ix + 1;
  }
}

int main(void)
{
  auto arr = new Dense<Pointer<Dense<Place<int>, 4>>, Nx / 4>();

  for (size_t ix = 0; ix < Nx; ix++) {
    Subscriptor(*arr, ix).activate();
    *Subscriptor(*arr, ix).get() = 3;
  }
  blur<<<(Nx + 1023) / 1024, (Nx < 1024 ? Nx : 1024)>>>(arr);
  checkCudaErrors(cudaDeviceSynchronize());
  for (size_t ix = 0; ix < Nx; ix++) {
    printf("%d\n", *Subscriptor(*arr, ix).get());
  }

  delete arr;
  return 0;
}
#endif
