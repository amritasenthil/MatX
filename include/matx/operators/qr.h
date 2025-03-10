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
#include "matx/transforms/qr.h"

namespace matx {

namespace detail {
  template<typename OpA>
  class QROp : public BaseOp<QROp<OpA>>
  {
    private:
      OpA a_;

    public:
      using matxop = bool;
      using scalar_type = typename OpA::scalar_type;
      using matx_transform_op = bool;
      using qr_xform_op = bool;

      __MATX_INLINE__ std::string str() const { return "qr(" + get_type_str(a_) + ")"; }
      __MATX_INLINE__ QROp(OpA a) : a_(a) { };

      // This should never be called
      template <typename... Is>
      __MATX_INLINE__ __MATX_DEVICE__ __MATX_HOST__ auto operator()(Is... indices) const = delete;

      template <typename Out, typename Executor>
      void Exec(Out &&out, Executor &&ex) {
        static_assert(is_device_executor_v<Executor>, "svd() only supports the CUDA executor currently");
        static_assert(std::tuple_size_v<remove_cvref_t<Out>> == 3, "Must use mtie with 3 outputs on qr(). ie: (mtie(Q, R) = qr(A))");

        qr_impl(std::get<0>(out), std::get<1>(out), a_, ex.getStream());
      }

      static __MATX_INLINE__ constexpr __MATX_HOST__ __MATX_DEVICE__ int32_t Rank()
      {
        return OpA::Rank();
      }

      template <typename ShapeType, typename Executor>
      __MATX_INLINE__ void PreRun([[maybe_unused]] ShapeType &&shape, Executor &&ex) noexcept
      {
        MATX_ASSERT_STR(false, matxNotSupported, "qr() must only be called with a single assignment");
      }

      // Size is not relevant in qr() since there are multiple return values and it
      // is not allowed to be called in larger expressions
      constexpr __MATX_INLINE__ __MATX_HOST__ __MATX_DEVICE__ index_t Size(int dim) const
      {
        return a_.Size(dim);
      }

  };
}


/**
 * Perform QR decomposition on a matrix using housholders reflections. If rank > 2 operations are batched.
 *
 * @tparam AType
 *   Tensor or operator type for output of A input tensors.
 *
 * @param A
 *   Input tensor or operator for tensor A input.
 * @returns Operator to generate Q/R outputs
 */
template<typename AType>
__MATX_INLINE__ auto qr(AType A) {
  return detail::QROp(A);
}


namespace detail {
  template<typename OpA>
  class CuSolverQROp : public BaseOp<CuSolverQROp<OpA>>
  {
    private:
      OpA a_;
      matx::tensor_t<typename OpA::scalar_type, OpA::Rank()> tmp_out_;

    public:
      using matxop = bool;
      using scalar_type = typename OpA::scalar_type;
      using matx_transform_op = bool;
      using cusolver_qr_xform_op = bool;

      __MATX_INLINE__ std::string str() const { return "cusolver_qr()"; }
      __MATX_INLINE__ CuSolverQROp(OpA a) : a_(a) { };

      // This should never be called
      template <typename... Is>
      __MATX_INLINE__ __MATX_DEVICE__ __MATX_HOST__ auto operator()(Is... indices) const = delete;

      template <typename Out, typename Executor>
      void Exec(Out &&out, Executor &&ex) {
        static_assert(is_device_executor_v<Executor>, "cusolver_qr() only supports the CUDA executor currently");
        static_assert(std::tuple_size_v<remove_cvref_t<Out>> == 3, "Must use mtie with 2 outputs on cusolver_qr(). ie: (mtie(A, tau) = eig(A))");     

        cusolver_qr_impl(std::get<0>(out), std::get<1>(out), a_, ex.getStream());
      }

      static __MATX_INLINE__ constexpr __MATX_HOST__ __MATX_DEVICE__ int32_t Rank()
      {
        return OpA::Rank();
      }

      template <typename ShapeType, typename Executor>
      __MATX_INLINE__ void PreRun([[maybe_unused]] ShapeType &&shape, Executor &&ex) noexcept
      {
        MATX_ASSERT_STR(false, matxNotSupported, "cusolver_qr() must only be called with a single assignment since it has multiple return types");
      }

      // Size is not relevant in cusolver_qr() since there are multiple return values and it
      // is not allowed to be called in larger expressions
      constexpr __MATX_INLINE__ __MATX_HOST__ __MATX_DEVICE__ index_t Size(int dim) const
      {
        return a_.Size(dim);
      }

  };
}

template<typename OpA>
__MATX_INLINE__ auto cusolver_qr(const OpA &a) {
  return detail::CuSolverQROp(a);
}

}
