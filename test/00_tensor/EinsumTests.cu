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

#include "assert.h"
#include "matx.h"
#include "test_types.h"
#include "utilities.h"
#include "gtest/gtest.h"

using namespace matx;

template <typename T> class EinsumTest : public ::testing::Test {

protected:
  void SetUp() override
  {
    pb = std::make_unique<detail::MatXPybind>();
  }

  void TearDown() { pb.reset(); }

  std::unique_ptr<detail::MatXPybind> pb;
  float thresh = 0.01f;
};


template <typename TensorType>
class EinsumTestsComplex : public EinsumTest<TensorType> {
};
template <typename TensorType>
class EinsumTestsFloat : public EinsumTest<TensorType> {
};
template <typename TensorType>
class EinsumTestsFloatNonComplex
    : public EinsumTest<TensorType> {
};
template <typename TensorType>
class EinsumTestsNumeric : public EinsumTest<TensorType> {
};
template <typename TensorType>
class EinsumTestsNumericNonComplex
    : public EinsumTest<TensorType> {
};

template <typename TensorType>
class EinsumTestsFloatNonComplexNonHalfTypes
    : public EinsumTest<TensorType> {
};

template <typename TensorType>
class EinsumTestsIntegral : public EinsumTest<TensorType> {
};
template <typename TensorType>
class EinsumTestsBoolean : public EinsumTest<TensorType> {
};
template <typename TensorType>
class EinsumTestsAll : public EinsumTest<TensorType> {
};

TYPED_TEST_SUITE(EinsumTestsAll, MatXAllTypes);
TYPED_TEST_SUITE(EinsumTestsComplex, MatXComplexTypes);
TYPED_TEST_SUITE(EinsumTestsFloat, MatXFloatTypes);
TYPED_TEST_SUITE(EinsumTestsFloatNonComplex, MatXFloatNonComplexTypes);
TYPED_TEST_SUITE(EinsumTestsFloatNonComplexNonHalfTypes, MatXFloatNonComplexNonHalfTypes);
TYPED_TEST_SUITE(EinsumTestsNumeric, MatXNumericTypes);
TYPED_TEST_SUITE(EinsumTestsIntegral, MatXAllIntegralTypes);
TYPED_TEST_SUITE(EinsumTestsNumericNonComplex, MatXNumericNonComplexTypes);
TYPED_TEST_SUITE(EinsumTestsBoolean, MatXBoolTypes);

#if MATX_ENABLE_CUTENSOR
TYPED_TEST(EinsumTestsFloatNonComplexNonHalfTypes, Contraction3D)
{
  MATX_ENTER_HANDLER();

  this->pb->template InitAndRunTVGenerator<TypeParam>(
      "00_operators", "contraction", "run", {});  

  // example-begin einsum-contraction-1
  auto a1 = make_tensor<TypeParam>({60});
  auto b1 = make_tensor<TypeParam>({24});
  auto c2 = make_tensor<TypeParam>({5,2});

  (a1 = linspace<0>(a1.Shape(), (TypeParam)0, static_cast<TypeParam>(a1.Size(0) - 1))).run();
  (b1 = linspace<0>(b1.Shape(), (TypeParam)0, static_cast<TypeParam>(b1.Size(0) - 1))).run();
  auto a = a1.View({3,4,5});
  auto b = b1.View({4,3,2});

  // Perform a 3D tensor contraction
  (c2 = cutensor::einsum("ijk,jil->kl", a, b)).run();
  // example-end einsum-contraction-1
  cudaStreamSynchronize(0);
  MATX_TEST_ASSERT_COMPARE(this->pb, c2, "c_float3d", 0.01);

  MATX_EXIT_HANDLER();
}

TYPED_TEST(EinsumTestsFloatNonComplexNonHalfTypes, Dot)
{
  MATX_ENTER_HANDLER();

  // example-begin einsum-dot-1
  auto a1 = make_tensor<TypeParam>({60});
  auto b1 = make_tensor<TypeParam>({60});
  auto c0 = make_tensor<TypeParam>();
  (a1 = ones(a1.Shape()) * 2).run();
  (b1 = ones(b1.Shape()) * 2).run(); 

  // Perform a dot product of b1 with itself and store in a1
  (c0 = cutensor::einsum("i,i->", a1, b1)).run();
  // example-end einsum-dot-1
  cudaStreamSynchronize(0);
  MATX_ASSERT_EQ(c0(), 4 * a1.Size(0));

  MATX_EXIT_HANDLER();
}

TYPED_TEST(EinsumTestsFloatNonComplexNonHalfTypes, GEMM)
{
  MATX_ENTER_HANDLER();

  // example-begin einsum-gemm-1
  auto a2 = make_tensor<TypeParam>({10,20});
  auto b2 = make_tensor<TypeParam>({20,10});
  auto c2 = make_tensor<TypeParam>({10,10});    
  auto c22 = make_tensor<TypeParam>({10,10});   
  (a2 = ones(a2.Shape())).run();
  (b2 = ones(b2.Shape())).run(); 

  // Perform a GEMM of a2 * b2. Compare results to traditional matmul call
  (c2 = cutensor::einsum("mk,kn->mn", a2, b2)).run();
  (c22 = matmul(a2, b2)).run();
  // example-end einsum-gemm-1
  cudaStreamSynchronize(0);

  for (auto i = 0; i < c2.Size(0); i++) {
    for (auto j = 0; j < c2.Size(1); j++) {
      MATX_ASSERT_EQ(c2(i,j), c22(i,j));
    }
  }

  MATX_EXIT_HANDLER();
}

TYPED_TEST(EinsumTestsFloatNonComplexNonHalfTypes, GEMMTranspose)
{
    // example-begin einsum-gemm-2
    auto a2 = make_tensor<TypeParam>({5,20});
    auto b2 = make_tensor<TypeParam>({20,10});
    auto c2 = make_tensor<TypeParam>({10,5});    
    auto c22 = make_tensor<TypeParam>({5,10});   
    (a2 = ones(a2.Shape())).run();
    (b2 = ones(b2.Shape())).run(); 

    // Perform a GEMM of a2 * b2 and store the results transposed
    (c2 = cutensor::einsum("mk,kn->nm", a2, b2)).run();
    // example-end einsum-gemm-2
    (c22 = matmul(a2, b2)).run();
    cudaStreamSynchronize(0);

    auto c22t = c22.Permute({1,0}); // Permute to match cutensor

    for (auto i = 0; i < c2.Size(0); i++) {
      for (auto j = 0; j < c2.Size(1); j++) {
        MATX_ASSERT_EQ(c2(i,j), c22t(i,j));
      }
    }
}

TYPED_TEST(EinsumTestsFloatNonComplexNonHalfTypes, Permute)
{
  // example-begin einsum-permute-1
  auto a = make_tensor<TypeParam>({5,20,4,3});
  auto b = make_tensor<TypeParam>({20,3,4,5});
  auto b2 = make_tensor<TypeParam>({20,3,4,5});
  (a = ones(a.Shape())).run();
  (b = ones(b.Shape())).run();

  // Permute a 4D tensor. This gives the same output as Permute, but is much faster
  (b = cutensor::einsum("ijkl->jlki", a)).run();
  (b2 = a.Permute({1,3,2,0})).run();
  // example-end einsum-permute-1
  cudaStreamSynchronize(0);

  for (auto i = 0; i < b.Size(0); i++) {
    for (auto j = 0; j < b.Size(1); j++) {
      for (auto k = 0; k < b.Size(2); k++) {
        for (auto l = 0; l < b.Size(3); l++) {
          MATX_ASSERT_EQ(b(i,j,k,l), b2(i,j,k,l));
        }
      }
    }
  }
}

TYPED_TEST(EinsumTestsFloatNonComplexNonHalfTypes, Sum)
{
  // example-begin einsum-sum-1
  auto a = matx::make_tensor<TypeParam>({2, 3});
  a.SetVals({
      {1, 2, 3},
      {4, 5, 6}
  });  

  auto b = matx::make_tensor<TypeParam>({3});
  // Sum the columns of "a"
  (b = matx::cutensor::einsum("ij->j", a)).run();
  // example-end einsum-sum-1
    
  cudaStreamSynchronize(0);
  for (auto i = 0; i < a.Size(1); i++) {
    TypeParam s = 0;
    for (auto j = 0; j < a.Size(0); j++) {
      s += a(j, i);
    }

    MATX_ASSERT_EQ(s, b(i));
  }
}

TYPED_TEST(EinsumTestsFloatNonComplexNonHalfTypes, Trace)
{
  MATX_ENTER_HANDLER();

  // example-begin einsum-trace-1
  auto a2 = make_tensor<TypeParam>({10,10});
  auto c0_0 = make_tensor<TypeParam>();
  auto c0_1 = make_tensor<TypeParam>();
  (a2 = ones(a2.Shape())).run();

  // Perform a GEMM of a2 * b2. Compare results to traditional matmul call
  (c0_0 = cutensor::einsum("ii->", a2)).run();
  (c0_1 = trace(a2)).run();

  // example-end einsum-trace-1
  cudaStreamSynchronize(0);

  MATX_ASSERT_EQ(c0_0(), c0_1());
  MATX_ASSERT_EQ(c0_0(), 10);

  MATX_EXIT_HANDLER();
}



#endif
