#include "matx.h"
#include <nvbench/nvbench.cuh>
#include "matx/core/half_complex.h"
#include "matx/core/nvtx.h"

using namespace matx;

using conv_types =
    nvbench::type_list<matxFp16Complex, cuda::std::complex<float>, cuda::std::complex<double>, float, double>;

/* FFT benchmarks */
template <typename ValueType>
void conv1d_4d_batch(nvbench::state &state,
                            nvbench::type_list<ValueType>)
{
  auto out = make_tensor<ValueType>({4, 2, 14, 288 + 4096 + 133 - 1});
  auto at = make_tensor<ValueType>({ 4, 2, 14, 133});
  auto bt = make_tensor<ValueType>({ 4, 2, 14, 288 + 4096});

  out.PrefetchDevice(0);
  at.PrefetchDevice(0);
  bt.PrefetchDevice(0);

  cudaDeviceSynchronize();
  MATX_NVTX_START_RANGE( "Exec", matx_nvxtLogLevels::MATX_NVTX_LOG_ALL, 1 )
  state.exec(
      [&out, &at, &bt](nvbench::launch &launch) { (out = conv1d(at, bt, MATX_C_MODE_FULL)).run(cudaExecutor(launch.get_stream())); });
  MATX_NVTX_END_RANGE( 1 )
  
}
NVBENCH_BENCH_TYPES(conv1d_4d_batch, NVBENCH_TYPE_AXES(conv_types));


template <typename ValueType>
void conv1d_2d_batch(nvbench::state &state,
                            nvbench::type_list<ValueType>)
{


  auto out = make_tensor<ValueType>({4 * 2* 14, 288 + 4096 + 133 - 1});
  auto at = make_tensor<ValueType>({ 4 * 2* 14, 133});
  auto bt = make_tensor<ValueType>({ 4 * 2* 14, 288 + 4096});

  out.PrefetchDevice(0);
  at.PrefetchDevice(0);
  bt.PrefetchDevice(0);

  cudaDeviceSynchronize();

  state.exec(
      [&out, &at, &bt](nvbench::launch &launch) { (out = conv1d(at, bt, MATX_C_MODE_FULL)).run(cudaExecutor(launch.get_stream())); });
}
NVBENCH_BENCH_TYPES(conv1d_2d_batch, NVBENCH_TYPE_AXES(conv_types));

template <typename ValueType>
void conv1d_large(nvbench::state &state,
                            nvbench::type_list<ValueType>)
{
  auto at = make_tensor<ValueType>({100000000});
  auto bt = make_tensor<ValueType>({1000});
  auto out = make_tensor<ValueType>({at.Size(at.Rank()-1) + bt.Size(bt.Rank()-1) - 1});

  out.PrefetchDevice(0);
  at.PrefetchDevice(0);
  bt.PrefetchDevice(0);

  cudaDeviceSynchronize();

  state.exec(
      [&out, &at, &bt](nvbench::launch &launch) { (out = conv1d(at, bt, MATX_C_MODE_FULL)).run(cudaExecutor(launch.get_stream())); });
}
NVBENCH_BENCH_TYPES(conv1d_large, NVBENCH_TYPE_AXES(conv_types));

template <typename ValueType>
void conv2d_batch(nvbench::state &state,
                            nvbench::type_list<ValueType>)
{
  auto at = make_tensor<ValueType>({256, 1024, 1024});
  auto bt = make_tensor<ValueType>({256, 16, 16});
  auto out = make_tensor<ValueType>({256, 
                                     at.Size(1) + bt.Size(1) - 1,
                                     at.Size(2) + bt.Size(2) - 1});

  out.PrefetchDevice(0);
  at.PrefetchDevice(0);
  bt.PrefetchDevice(0);

  cudaDeviceSynchronize();

  state.exec(
      [&out, &at, &bt](nvbench::launch &launch) { (out = conv2d(at, bt, MATX_C_MODE_FULL)).run(cudaExecutor(launch.get_stream())); });

  auto seconds = state.get_summary("Batch GPU").get_float64("value");
  auto &flops = state.add_summary("TFLOPS");

  flops.set_string("hint", "item_rate");
  flops.set_string("short_name", "TFLOPS");
  flops.set_string("description", "Trillions of operations per second");

  if constexpr (is_complex_v<ValueType>) {
    flops.set_float64("value", (double)2 * out.Size(2) * out.Size(1) * out.Size(0) * bt.Size(2) * bt.Size(1) * 4 / seconds / 1e12);
  } else {
    flops.set_float64("value", (double)2 * out.Size(2) * out.Size(1) * out.Size(0) * bt.Size(2) * bt.Size(1) / seconds / 1e12);
  }
}
NVBENCH_BENCH_TYPES(conv2d_batch, NVBENCH_TYPE_AXES(conv_types));
