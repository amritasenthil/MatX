////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// COpBright (c) 2021, NVIDIA Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above cOpBright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above cOpBright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the cOpBright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COpBRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COpBRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/////////////////////////////////////////////////////////////////////////////////

#pragma once


#include "matx/core/type_utils.h"
#include "matx/operators/base_operator.h"
#include "matx/transforms/cub.h"

namespace matx {



namespace detail {
  template<typename OpA>
  class TraceOp : public BaseOp<TraceOp<OpA>>
  {
    private:
      OpA a_;
      matx::tensor_t<typename OpA::scalar_type, 0> tmp_out_;      

    public:
      using matxop = bool;
      using scalar_type = typename OpA::scalar_type;
      using matx_transform_op = bool;
      using trace_xform_op = bool;

      __MATX_INLINE__ std::string str() const { return "trace()"; }
      __MATX_INLINE__ TraceOp(OpA a) : a_(a) { 
      };

      template <typename... Is>
      __MATX_INLINE__ __MATX_DEVICE__ __MATX_HOST__ auto operator()(Is... indices) const {
        return tmp_out_(indices...);
      };

      template <typename Out, typename Executor>
      void Exec(Out &&out, Executor &&ex) {
        trace_impl(std::get<0>(out), a_, ex);
      }

      static __MATX_INLINE__ constexpr __MATX_HOST__ __MATX_DEVICE__ int32_t Rank()
      {
        return 0;
      }

      template <typename ShapeType, typename Executor>
      __MATX_INLINE__ void PreRun([[maybe_unused]] ShapeType &&shape, Executor &&ex) noexcept
      {
        if constexpr (is_matx_op<OpA>()) {
          a_.PreRun(std::forward<ShapeType>(shape), std::forward<Executor>(ex));
        }     

        if constexpr (is_device_executor_v<Executor>) {
          make_tensor(tmp_out_, MATX_ASYNC_DEVICE_MEMORY, ex.getStream());
        }
        else {
          make_tensor(tmp_out_, MATX_HOST_MEMORY);          
        }

        Exec(std::make_tuple(tmp_out_), std::forward<Executor>(ex));
      }

      constexpr __MATX_INLINE__ __MATX_HOST__ __MATX_DEVICE__ index_t Size(int dim) const
      {
        return 1;
      }

  };
}

/**
 * Computes the trace of a tensor
 *
 * Computes the trace of a square matrix by summing the diagonal
 *
 * @tparam InputOperator
 *   Input data type
 *
 * @param a
 *   Input data to reduce
 */
template <typename InputOperator>
__MATX_INLINE__ auto trace(const InputOperator &a) {
  return detail::TraceOp(a);
}

}
