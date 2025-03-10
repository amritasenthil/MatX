////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, NVIDIA Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
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
#include "matx/transforms/matmul.h"

namespace matx
{
  namespace detail {
    template <typename OpA, typename OpB, typename PermDims>
    class MatMulOp : public BaseOp<MatMulOp<OpA, OpB, PermDims>>
    {
      private:
        OpA a_;
        OpB b_;
        float alpha_;
        float beta_;
        PermDims perm_; 
        std::array<index_t, OpA::Rank()> out_dims_;
        matx::tensor_t<typename OpA::scalar_type, OpA::Rank()> tmp_out_;

      public:
        using matxop = bool;
        using scalar_type = typename OpA::scalar_type;
        using matx_transform_op = bool;
        using matmul_xform_op = bool;

        __MATX_INLINE__ std::string str() const { 
            return "matmul(" + get_type_str(a_) + ")";
        }

        __MATX_INLINE__ MatMulOp(OpA a, OpB b, float alpha, float beta, PermDims perm) : 
              a_(a), b_(b), alpha_(alpha), beta_(beta), perm_(perm) {
          if constexpr (!std::is_same_v<PermDims, no_permute_t>) {
            for (int r = 0; r < Rank(); r++) {
              if (perm_[r] == Rank() - 2) {
                out_dims_[r] = a_.Size(perm_[r]);
              }
              else if (perm_[r] == Rank() - 1) {
                out_dims_[r] = b_.Size(perm_[r]);
              }
              else {
                out_dims_[r] = a_.Size(r);
              }
            }
            printf("%lld %lld %lld\n", out_dims_[0], out_dims_[1], out_dims_[2]);
          }
          else {
            for (int r = 0; r < Rank() - 2; r++) {
              out_dims_[r] = a_.Size(r);
            }

            out_dims_[OpA::Rank() - 2] = a_.Size(OpA::Rank() - 2);
            out_dims_[OpB::Rank() - 1] = b_.Size(OpB::Rank() - 1);
          }
        }

        template <typename... Is>
        __MATX_INLINE__ __MATX_DEVICE__ __MATX_HOST__ auto operator()(Is... indices) const
        {
          return tmp_out_(indices...);
        }
   

        static __MATX_INLINE__ constexpr __MATX_HOST__ __MATX_DEVICE__ int32_t Rank()
        {
          return OpA::Rank();
        }
        constexpr __MATX_INLINE__ __MATX_HOST__ __MATX_DEVICE__ index_t Size(int dim) const
        {
          return out_dims_[dim];
        }

        template <typename Out, typename Executor>
        void Exec(Out &&out, Executor &&ex) {
          static_assert(is_device_executor_v<Executor>, "matmul() only supports the CUDA executor currently");
          if constexpr (!std::is_same_v<PermDims, no_permute_t>) {
            matmul_impl(permute(std::get<0>(out), perm_), a_, b_, ex.getStream(), alpha_, beta_);
          }
          else {
            matmul_impl(std::get<0>(out), a_, b_, ex.getStream(), alpha_, beta_);
          }
        }

        template <typename ShapeType, typename Executor>
        __MATX_INLINE__ void PreRun([[maybe_unused]] ShapeType &&shape, Executor &&ex) noexcept
        {
          if constexpr (is_matx_op<OpA>()) {
            a_.PreRun(std::forward<ShapeType>(shape), std::forward<Executor>(ex));
          }

          if constexpr (is_matx_op<OpB>()) {
            b_.PreRun(std::forward<ShapeType>(shape), std::forward<Executor>(ex));
          }          

          if constexpr (is_device_executor_v<Executor>) {
            make_tensor(tmp_out_, out_dims_, MATX_ASYNC_DEVICE_MEMORY, ex.getStream());
          }

          Exec(std::make_tuple(tmp_out_), std::forward<Executor>(ex));
        }
    };
  }


  /**
   * Run a GEMM (generic matrix multiply))
   *
   * Creates a new GEMM plan in the cache if none exists, and uses that to execute
   * the GEMM. This function is preferred over creating a plan directly for both
   * efficiency and simpler code. Since it only uses the signature of the GEMM to
   * decide if a plan is cached, it may be able to reused plans for different
   * A/B/C matrices as long as they were configured with the same dimensions.
   *
   * @tparam OpA
   *    Data type of A tensor or operator
   * @tparam OpB
   *    Data type of B tensor or operator
   *
   * @param A
   *   A A Tensor or Operator
   * @param B
   *   B B Tensor or Operator
   * @param alpha
   *   Scalar multiplier to apply to operator A
   * @param beta
   *   Scalar multiplier to apply to operator C on input
   */
  template<typename OpA, typename OpB>
  __MATX_INLINE__ auto matmul(const OpA A, const OpB B, float alpha = 1.0, float beta = 0.0) {
    return detail::MatMulOp(A, B, alpha, beta, detail::no_permute_t{});
  }

  /**
   * Run a GEMM (generic matrix multiply))
   *
   * Creates a new GEMM plan in the cache if none exists, and uses that to execute
   * the GEMM. This function is preferred over creating a plan directly for both
   * efficiency and simpler code. Since it only uses the signature of the GEMM to
   * decide if a plan is cached, it may be able to reused plans for different
   * A/B/C matrices as long as they were configured with the same dimensions.
   *
   * @tparam OpA
   *    Data type of A tensor or operator
   * @tparam OpB
   *    Data type of B tensor or operator
   *
   * @param A
   *   A A Tensor or Operator
   * @param B
   *   B B Tensor or Operator
  * @param axis
  *   the axis of the tensor or operator to perform the gemm along
   * @param alpha
   *   Scalar multiplier to apply to operator A
   * @param beta
   *   Scalar multiplier to apply to operator C on input
   */
  template<typename OpA, typename OpB>
  __MATX_INLINE__ auto matmul(const OpA A, const OpB B, const int32_t (&axis)[2], float alpha = 1.0, float beta = 0.0) {
    MATX_STATIC_ASSERT(OpA::Rank() == OpB::Rank(), "matmul: inputs must have same rank to use matmul with axis parameter");
    MATX_STATIC_ASSERT(OpA::Rank() == OpB::Rank(), "matmul: inputs and outputs must have same rank to use matmul with axis parameter");

    auto perm = detail::getPermuteDims<OpA::Rank()>(axis);
    auto in1 = permute(A, perm);
    auto in2 = permute(B, perm);

    return detail::MatMulOp(in1, in2, alpha, beta, perm);
  }  
}
