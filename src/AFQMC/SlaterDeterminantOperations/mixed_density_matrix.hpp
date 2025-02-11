////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source
// License.  See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by:
// Miguel A. Morales, moralessilva2@llnl.gov 
//    Lawrence Livermore National Laboratory 
// Alfredo Correa, correaa@llnl.gov 
//    Lawrence Livermore National Laboratory 
//
// File created by:
// Miguel A. Morales, moralessilva2@llnl.gov 
//    Lawrence Livermore National Laboratory 
////////////////////////////////////////////////////////////////////////////////


#ifndef  AFQMC_DENSITY_MATRIX_HPP 
#define  AFQMC_DENSITY_MATRIX_HPP 

#include "AFQMC/config.h"
#include "AFQMC/Numerics/ma_operations.hpp"
#include "AFQMC/Numerics/tensor_operations.hpp"
#include <Utilities/FairDivide.h>

namespace qmcplusplus
{

namespace afqmc 
{

namespace SlaterDeterminantOperations
{

namespace base 
{

/*
 * Calculates the 1-body mixed density matrix:
 *   < A | c+i cj | B > / <A|B> = conj(A) * ( T(B) * conj(A) )^-1 * T(B) 
 *   If compact == True, returns [NEL x M] matrix:
 *   < A | c+i cj | B > / <A|B> = ( T(B) * conj(A) )^-1 * T(B) 
 * Parameters:
 *  - hermA = conjugateTranspose(A)
 *  - B
 *  - C = < A | c+i cj | B > / <A|B>
 *  - T1: [ NEL x NEL ] work matrix   
 *  - T2: (only used if compact = False) [ NEL x M ] work matrix   
 *  - IWORK: [ N ] integer biffer for invert. Dimensions must be at least NEL. 
 *  - WORK: [ >=NEL ] Work space for invert. Dimensions must be at least NEL.   
 *  - compact (default = True)
 *  returns:
 *  - <A|B> = det[ T(conj(A)) * B ]  
 */
// Serial Implementation
template< class Tp,
          class MatA,
          class MatB,
          class MatC,
          class Mat1,
          class Mat2,
          class IBuffer,
          class TBuffer 
        >
Tp MixedDensityMatrix(const MatA& hermA, const MatB& B, MatC&& C, Tp LogOverlapFactor, Mat1&& T1, Mat2&& T2, IBuffer& IWORK, TBuffer& WORK, bool compact=true)
{
  // check dimensions are consistent
  assert( hermA.size(1) == B.size(0) );
  assert( hermA.size(0) == B.size(1) );
  assert( hermA.size(0) == T1.size(0) );
  assert( B.size(1) == T1.size(1) );
  if(compact) {
    assert( C.size(0) == T1.size(1) );
    assert( C.size(1) == B.size(0) );
  } else {
    assert( T2.size(1) == B.size(0) );
    assert( T2.size(0) == T1.size(1) );
    assert( C.size(0) == hermA.size(1) );
    assert( C.size(1) == T2.size(1) );
  }

  using ma::T;

  // T(B)*conj(A) 
  ma::product(hermA,B,std::forward<Mat1>(T1));  

  // NOTE: Using C as temporary 
  // T1 = T1^(-1)
  Tp ovlp = ma::invert(std::forward<Mat1>(T1),IWORK,WORK,LogOverlapFactor);
  if(std::abs(ovlp) == Tp(0.0)) {
    using element = typename std::decay<MatC>::type::element;
    ma::fill(C,element(0.0));
    return ovlp;
  }

  if(compact) {

    // C = T(T1) * T(B)
    ma::product(T(T1),T(B),std::forward<MatC>(C)); 

  } else {

    // T2 = T(T1) * T(B)
    ma::product(T(T1),T(B),std::forward<Mat2>(T2)); 

    // C = conj(A) * T2
    ma::product(T(hermA),T2,std::forward<MatC>(C));

  }
  return ovlp;
}

template< class Tp,
          class integer,
          class MatA,
          class MatB,
          class MatC,
          class MatD,
          class Mat1,
          class Mat2,
          class Mat3,
          class IBuffer,
          class TBuffer 
        >
Tp MixedDensityMatrixForWoodbury(const MatA& hermA, const MatB& B, MatC&& C, Tp LogOverlapFactor, MatD&& QQ0, integer *ref, Mat1&& TNN, Mat2&& TAB, Mat3&& TNM, IBuffer& IWORK, TBuffer& WORK, bool compact=true)
{
  // check dimensions are consistent
  int NEL = B.size(1);
  assert( hermA.size(1) == B.size(0) );
  assert( hermA.size(0) == TAB.size(0) );
  assert( B.size(1) == TAB.size(1) );
  assert( B.size(1) == TNN.size(0) );
  assert( B.size(1) == TNN.size(1) );
  assert( hermA.size(0) == QQ0.size(0) );
  assert( B.size(1) == QQ0.size(1) );
  if(compact) {
    assert( C.size(0) == TNN.size(1) );
    assert( C.size(1) == B.size(0) );
  } else {
    assert( TNM.size(1) == B.size(0) );
    assert( TNM.size(0) == TNN.size(1) );
    assert( C.size(0) == hermA.size(1) );
    assert( C.size(1) == TNM.size(1) );
  }

  using ma::T;

  // TAB = herm(A)*B
  ma::product(hermA,B,std::forward<Mat2>(TAB));  

  // TNN = TAB[ref,:] 
  for(int i=0; i<NEL; i++)
    std::copy_n(to_address(TAB[*(ref+i)].origin()),NEL,to_address(TNN[i].origin()));

  // TNN = TNN^(-1)
  Tp ovlp = ma::invert(std::forward<Mat1>(TNN),IWORK,WORK,LogOverlapFactor);
  if(std::abs(ovlp) == Tp(0.0)) {
    using element = typename std::decay<MatC>::type::element;
    ma::fill(C,element(0.0));
    return ovlp;
  }

  // QQ0 = TAB * inv(TNN) 
  ma::product(TAB,TNN,std::forward<MatD>(QQ0));

  if(compact) {

    // C = T(TNN) * T(B)
    ma::product(T(TNN),T(B),std::forward<MatC>(C)); 

  } else {

    // TNM = T(TNN) * T(B)
    ma::product(T(TNN),T(B),std::forward<Mat3>(TNM)); 

    // C = conj(A) * TNM
    ma::product(T(hermA),TNM,std::forward<MatC>(C));

  }
  return ovlp;
}

template< class Tp,
          class integer,
          class MatA,
          class MatB,
          class MatC,
          class Mat1,
          class Mat2,
          class Mat3,
          class IBuffer,
          class TBuffer 
        >
Tp MixedDensityMatrixFromConfiguration(const MatA& hermA, const MatB& B, MatC&& C, Tp LogOverlapFactor, integer *ref, Mat1&& TNN, Mat2&& TAB, Mat3&& TNM, IBuffer& IWORK, TBuffer& WORK, bool compact=true)
{
  // check dimensions are consistent
  int NEL = B.size(1);
  assert( hermA.size(1) == B.size(0) );
  assert( hermA.size(0) == TAB.size(0) );
  assert( B.size(1) == TAB.size(1) );
  assert( B.size(1) == TNN.size(0) );
  assert( B.size(1) == TNN.size(1) );
  if(compact) {
    assert( C.size(0) == TNN.size(1) );
    assert( C.size(1) == B.size(0) );
  } else {
    assert( TNM.size(1) == B.size(0) );
    assert( TNM.size(0) == TNN.size(1) );
    assert( C.size(0) == hermA.size(1) );
    assert( C.size(1) == TNM.size(1) );
  }

  using ma::T;

  // TAB = herm(A)*B
  ma::product(hermA,B,std::forward<Mat2>(TAB));  

  // TNN = TAB[ref,:] 
  for(int i=0; i<NEL; i++)
    std::copy_n(to_address(TAB[*(ref+i)].origin()),NEL,to_address(TNN[i].origin()));

  // TNN = TNN^(-1)
  Tp ovlp = ma::invert(std::forward<Mat1>(TNN),IWORK,WORK,LogOverlapFactor);
  if(std::abs(ovlp) == Tp(0.0)) {
    using element = typename std::decay<MatC>::type::element;
    ma::fill(C,element(0.0));
    return ovlp;
  }

  if(compact) {

    // C = T(TNN) * T(B)
    ma::product(T(TNN),T(B),std::forward<MatC>(C)); 

  } else {

    // TNM = T(TNN) * T(B)
    ma::product(T(TNN),T(B),std::forward<Mat3>(TNM)); 

    // C = conj(A) * TNM
    ma::product(T(hermA),TNM,std::forward<MatC>(C));

  }
  return ovlp;
}

/*
 * Calculates the 1-body mixed density matrix:
 *   < A | c+i cj | B > / <A|B> = conj(A) * ( T(B) * conj(A) )^-1 * T(B) 
 *   If compact == True, returns [NEL x M] matrix:
 *   < A | c+i cj | B > / <A|B> = ( T(B) * conj(A) )^-1 * T(B) 
 * Parameters:
 *  - A = A
 *  - B
 *  - C = < A | c+i cj | B > / <A|B>
 *  - T1: [ NEL x NEL ] work matrix   
 *  - T2: (only used if compact = False) [ NEL x M ] work matrix   
 *  - IWORK: [ N ] integer biffer for invert. Dimensions must be at least NEL. 
 *  - WORK: [ >=NEL ] Work space for invert. Dimensions must be at least NEL.   
 *  - compact (default = True)
 *  returns:
 *  - <A|B> = det[ T(conj(A)) * B ]  
 */
// Serial Implementation
template< class Tp,
          class MatA,
          class MatB,
          class MatC,
          class Mat1,
          class Mat2,
          class IBuffer,
          class TBuffer 
        >
Tp MixedDensityMatrix_noHerm(const MatA& A, const MatB& B, MatC&& C, Tp LogOverlapFactor, Mat1&& T1, Mat2&& T2, IBuffer& IWORK, TBuffer& WORK, bool compact=true)
{
  // check dimensions are consistent
  assert( A.size(0) == B.size(0) );
  assert( A.size(1) == B.size(1) );
  assert( A.size(1) == T1.size(0) );
  assert( B.size(1) == T1.size(1) );
  if(compact) {
    assert( C.size(0) == T1.size(1) );
    assert( C.size(1) == B.size(0) );
  } else {
    assert( T2.size(1) == B.size(0) );
    assert( T2.size(0) == T1.size(1) );
    assert( C.size(0) == A.size(0) );
    assert( C.size(1) == T2.size(1) );
  }

  using ma::T;
  using ma::H;

  // T1 = H(A)*B
  ma::product(H(A),B,std::forward<Mat1>(T1));  

  // NOTE: Using C as temporary 
  // T1 = T1^(-1)
  Tp ovlp = ma::invert(std::forward<Mat1>(T1),IWORK,WORK,LogOverlapFactor);
  if(std::abs(ovlp) == Tp(0.0)) {
    using element = typename std::decay<MatC>::type::element;
    ma::fill(C,element(0.0));
    return ovlp;
  }

  if(compact) {

    // C = T(T1) * T(B)
    ma::product(T(T1),T(B),std::forward<MatC>(C)); 

  } else {

    // T2 = T1 * H(A) 
    ma::product(T1,H(A),std::forward<Mat2>(T2)); 

    // C = T( B * T2) = T(T2) * T(B)
    ma::product(T(T2),T(B),std::forward<MatC>(C));

  }
  return ovlp;
}

template< class Tp,
          class MatA,
          class MatB,
          class MatC,
          class Mat1,
          class RVec,
          class IBuffer, 
          class TBuffer 
        >
Tp MixedDensityMatrix_noHerm_wSVD(const MatA& A, const MatB& B, MatC&& C, Tp LogOverlapFactor, RVec&& S, Mat1&& U, Mat1&& VT, Mat1&& BV, Mat1&& UA, IBuffer& IWORK, TBuffer& WORK, bool compact=true)
{
  // check dimensions are consistent
  assert( A.size(0) == B.size(0) );
  assert( A.size(1) == B.size(1) );
  assert( A.size(1) == U.size(0) );   // [U] = [NxN]
  assert( A.size(1) == U.size(1) );
  assert( A.size(1) == VT.size(0) );  // [V] = [NxN]
  assert( A.size(1) == VT.size(1) );
  assert( A.size(1) <= (6*S.size(0)+1) );   // [S] = [N+1]
  assert( A.size(1) == UA.size(0) );  // [UA] = [NxM]
  assert( A.size(0) == UA.size(1) );
  if(compact) {
    assert( C.size(0) == B.size(1) );
    assert( C.size(1) == B.size(0) );
  } else {
    assert( A.size(0) == BV.size(0) );  // [BV] = [MxN]
    assert( A.size(1) == BV.size(1) );
    assert( C.size(0) == A.size(0) );
    assert( C.size(1) == A.size(0) );
  }

  using ma::T;
  using ma::H;
  using ma::term_by_term_matrix_vector;
  using ma::determinant_from_geqrf;
  using ma::real;

  int N(U.size(0));

  // T1 = H(A)*B
  ma::product(H(A),B,U);  

  // keep a copy of U in UA temporarily
  using std::copy_n; 
  copy_n(U.origin(),U.num_elements(),UA.origin());  

  // get determinant, since you can't get the phase trivially from SVD
  Tp ovlp = ma::determinant(U,IWORK,WORK,LogOverlapFactor);
//  ma::geqrf(U,VT[0],WORK);
//  determinant_from_geqrf(N,U.origin(),U.stride(0),VT[1].origin(),LogOverlapFactor,ovlp);
//  if you want the correct phase of the determinant
//  ma::gqr(U,S.sliced(0,N),WORK);
//  ComplexType ovQ = ma::determinant(U,IWORK,WORK,0.0);  
//  *ovlp *= ovQ;

  // restore U
  copy_n(UA.origin(),U.num_elements(),U.origin());  

  // H(A)*B = AtB = U * S * VT
  // inv(H(A)*B) = inv(AtB) = H(VT) * inv(S) * H(U)
  ma::gesvd('O','A',U,S.sliced(0,N),U,VT,WORK,S.sliced(N,6*N));

  // testing
  boost::multi::array<double,1> Sh(S.sliced(0,N));
  double ov_(0.0);
  for(int i=0; i<N; i++)
    ov_ += std::log(Sh[i]);
  ov_ = std::exp(ov_ - real(LogOverlapFactor));
//  std::cout<<" SVD: " <<ov0 <<" " <<ov_ <<" " <<ov0/ov_ <<" " <<Sh[0] <<" " <<Sh[N-1] <<" " <<Sh[0]/Sh[N-1] <<std::endl;

  // mod Sh
  // S = Sh;

  if(compact) {

    // G = T( B * inv(AtB) )
    //   = T( B * H(VT) * inv(S) * H(U) )   
    //   = T(H(VT) * inv(S) * H(U)) * T(B)


    // VT = VT * inv(S), which works since S is diagonal and real
    term_by_term_matrix_vector(ma::TOp_DIV,0,VT.size(0),VT.size(1),ma::pointer_dispatch(VT.origin()),
                VT.stride(0),ma::pointer_dispatch(S.origin()),1);

    // BV = H(VT) * H(U)
    ma::product(H(VT),H(U),BV.sliced(0,N));

    // G = T(BV) * T(B) 
    product(T(BV.sliced(0,N)),T(B),C);

  } else {

    // G = T( B * inv(AtB) * H(A) )
    //   = T( B * H(VT) * inv(S) * H(U) * H(A) )   
    //   = T( BV * inv(S) * UA )
    //   = T(UA) * T(BV * inv(S))
  
    // BV = B * H(VT)
    ma::product(B,H(VT),BV);

    // BV = BV * inv(S), which works since S is diagonal and real
    term_by_term_matrix_vector(ma::TOp_DIV,1,BV.size(0),BV.size(1),ma::pointer_dispatch(BV.origin()),
                BV.stride(0),ma::pointer_dispatch(S.origin()),1);

    // UA = H(U) * H(A)
    ma::product(H(U),H(A),UA);
    
    // G = T(UA) * T(BV * inv(S))
    product(T(UA),T(BV),C);

  }
  return ovlp;
}

/*
 * Returns the overlap of 2 Slater determinants:  <A|B> = det[ T(B) * conj(A) ]  
 * Parameters:
 *  - hermA = conjugateTranspose(A)
 *  - B
 *  - IWORK: [ M ] integer work matrix   
 *  - T1: [ NEL x NEL ] work matrix   
 *  returns:
 *  - <A|B> = det[ T(conj(A)) * B ]  
 */
// Serial Implementation
template< class Tp,
          class MatA,
          class MatB,
          class Mat,
          class Buffer,
          class IBuffer
        >
Tp Overlap(const MatA& hermA, const MatB& B, Tp LogOverlapFactor, Mat&& T1, IBuffer& IWORK, Buffer& WORK)
{
  // check dimensions are consistent
  assert( hermA.size(1) == B.size(0) );
  assert( hermA.size(0) == B.size(1) );
  assert( hermA.size(0) == T1.size(0) );
  assert( B.size(1) == T1.size(1) );

  using ma::T;

  // T(B)*conj(A) 
  ma::product(hermA,B,std::forward<Mat>(T1));   

  return ma::determinant(std::forward<Mat>(T1),IWORK,WORK,LogOverlapFactor);
}

template< class Tp,
          class integer,
          class MatA,
          class MatB,
          class MatC,
          class MatD,
          class MatE,
          class IBuffer,
          class TBuffer
        >
Tp OverlapForWoodbury(const MatA& hermA, const MatB& B, Tp LogOverlapFactor, MatC&& QQ0, integer *ref, 
                             MatD && TNN, MatE && TMN, IBuffer& IWORK, TBuffer& WORK)
{
  // check dimensions are consistent
  int NEL = B.size(1);
  assert( hermA.size(1) == B.size(0) );
  assert( hermA.size(0) == TMN.size(0) );
  assert( B.size(1) == TMN.size(1) );
  assert( B.size(1) == TNN.size(0) );
  assert( B.size(1) == TNN.size(1) );
  assert( hermA.size(0) == QQ0.size(0) );
  assert( B.size(1) == QQ0.size(1) );

  using ma::T;

  // TMN = herm(A)*B 
  ma::product(hermA,B,std::forward<MatD>(TMN));

  // TNN = TMN[ref,:]
  for(int i=0; i<NEL; i++)
    std::copy_n(to_address(TMN[*(ref+i)].origin()),NEL,to_address(TNN[i].origin())); 
 
  // TNN -> inv(TNN)
  Tp ovlp = ma::invert(std::forward<MatD>(TNN),IWORK,WORK,LogOverlapFactor);
  if(std::abs(ovlp) == Tp(0.0)) {
    using element = typename std::decay<MatC>::type::element;
    ma::fill(QQ0,element(0.0));
    return ovlp;
  }

  // QQ0 = TMN * inv(TNN) 
  ma::product(TMN,TNN,std::forward<MatC>(QQ0));  

  return ovlp;
}

/*
 * Returns the overlap of 2 Slater determinants:  <A|B> = det[ T(B) * conj(A) ]  
 * Parameters:
 *  - A = conj(A)
 *  - B
 *  - IWORK: [ M ] integer work matrix   
 *  - T1: [ NEL x NEL ] work matrix   
 *  returns:
 *  - <A|B> = det[ T(conj(A)) * B ]  
 */
// Serial Implementation
template< class Tp,
          class MatA,
          class MatB,
          class Mat,
          class Buffer,
          class IBuffer
        >
Tp Overlap_noHerm(const MatA& A, const MatB& B, Tp LogOverlapFactor, Mat&& T1, IBuffer& IWORK, Buffer& WORK)
{
  // check dimensions are consistent
  assert( A.size(0) == B.size(0) );
  assert( A.size(1) == B.size(1) );
  assert( A.size(1) == T1.size(0) );
  assert( B.size(1) == T1.size(1) );

  using ma::T;
  using ma::H;

  // T(B)*conj(A) 
  ma::product(H(A),B,std::forward<Mat>(T1));    

  return ma::determinant(std::forward<Mat>(T1),IWORK,WORK,LogOverlapFactor);
}

} // namespace base

namespace shm 
{

/*
 * Calculates the 1-body mixed density matrix:
 *   < A | c+i cj | B > / <A|B> = conj(A) * ( T(B) * conj(A) )^-1 * T(B) 
 *   If compact == True, returns [NEL x M] matrix:
 *   < A | c+i cj | B > / <A|B> = ( T(B) * conj(A) )^-1 * T(B) 
 * Parameters:
 *  - hermA = conjugateTranspose(A)
 *  - B
 *  - C = < A | c+i cj | B > / <A|B>
 *  - T1: [ NEL x NEL ] work matrix   
 *  - T2: (only used if compact = False) [ NEL x M ] work matrix   
 *  - IWORK: [ N ] integer biffer for invert. Dimensions must be at least NEL. 
 *  - WORK: [ >=NEL ] Work space for invert. Dimensions must be at least NEL.   
 *  - compact (default = True)
 *  returns:
 *  - <A|B> = det[ T(B) * conj(A) ]  
 */
// Serial Implementation
template< class Tp,
          class MatA,
          class MatB,
          class MatC,
          class Mat,
          class IBuffer,
          class TBuffer,
          class communicator 
        >
Tp MixedDensityMatrix(const MatA& hermA, const MatB& B, MatC&& C, Tp LogOverlapFactor, Mat&& T1, Mat&& T2, IBuffer& IWORK, TBuffer& WORK, communicator& comm, bool compact=true)
{
  // check dimensions are consistent
  assert( hermA.size(1) == B.size(0) );
  assert( hermA.size(0) == B.size(1) );
  assert( hermA.size(0) == T1.size(0) );
  assert( B.size(1) == T1.size(1) );
  if(compact) {
    assert( C.size(0) == T1.size(1) );
    assert( C.size(1) == B.size(0) );
  } else {
    assert( T2.size(1) == B.size(0) );
    assert( T2.size(0) == T1.size(1) );
    assert( C.size(0) == hermA.size(1) );
    assert( C.size(1) == T2.size(1) );
  }

  using ma::T;

  int N0,Nn,sz=B.size(1);
  std::tie(N0,Nn) = FairDivideBoundary(comm.rank(),sz,comm.size());

  // T(B)*conj(A) 
  if(N0!=Nn)
    ma::product(hermA,
              B(B.extension(0),{N0,Nn}),
              T1(T1.extension(0),{N0,Nn}));  

  comm.barrier();

  // NOTE: Using C as temporary 
  // T1 = T1^(-1)
  Tp ovlp;
  if(comm.rank()==0) {
    ovlp = ma::invert(std::forward<Mat>(T1),IWORK,WORK,LogOverlapFactor);
    if(std::abs(ovlp) == Tp(0.0)) {
      using element = typename std::decay<MatC>::type::element;
      ma::fill(C,element(0.0));
    }
  }
  comm.broadcast_n(&ovlp,1,0);  
  if(std::abs(ovlp) == Tp(0.0)) 
    return ovlp;

  if(compact) {

    // C = T(T1) * T(B)
    //ma::product(T1.sliced(N0,Nn),
    //            T(B),
    //            C.sliced(N0,Nn)); 
    if(N0!=Nn)
      ma::product(T(T1(T1.extension(0),{N0,Nn})),
                T(B),
                C.sliced(N0,Nn)); 

  } else {

    // T2 = T(T1) * T(B)
    //ma::product(T1.sliced(N0,Nn),
    //            T(B),
    //            T2.sliced(N0,Nn)); 
    if(N0!=Nn)
      ma::product(T(T1(T1.extension(0),{N0,Nn})),
                T(B),
                T2.sliced(N0,Nn)); 

    comm.barrier();
    
    sz=T2.size(1);
    std::tie(N0,Nn) = FairDivideBoundary(comm.rank(),sz,comm.size());

    // C = conj(A) * T2
    if(N0!=Nn)
      ma::product(T(hermA),
                T2(T2.extension(0),{N0,Nn}),
                C(C.extension(0),{N0,Nn}));

  }
  comm.barrier();
  return ovlp;
}


/*
 * Returns the overlap of 2 Slater determinants:  <A|B> = det[ T(conj(A)) * B ]  
 * Parameters:
 *  - hermA = conjugateTranspose(A)
 *  - B
 *  - IWORK: [ M ] integer work matrix   
 *  - T1: [ NEL x NEL ] work matrix   
 *  returns:
 *  - <A|B> = det[ hermA * B ]  
 */
// Serial Implementation
template< class Tp,
          class MatA,
          class MatB,
          class Mat,
          class IBuffer,
          class Buffer,
          class communicator
        >
Tp Overlap(const MatA& hermA, const MatB& B, Tp LogOverlapFactor, Mat&& T1, IBuffer& IWORK, Buffer& WORK, communicator& comm)
{
  // check dimensions are consistent
  assert( hermA.size(1) == B.size(0) );
  assert( hermA.size(0) == B.size(1) );
  assert( hermA.size(0) == T1.size(0) );
  assert( B.size(1) == T1.size(1) );

  using ma::T;

  int N0,Nn,sz = B.size(1);
  std::tie(N0,Nn) = FairDivideBoundary(comm.rank(),sz,comm.size());

  // T(B)*conj(A) 
  if(N0!=Nn)
    ma::product(hermA,
              B(B.extension(0),{N0,Nn}),
              T1(T1.extension(0),{N0,Nn}));

  comm.barrier();

  Tp ovlp;
  if(comm.rank()==0)
   ovlp = ma::determinant(std::forward<Mat>(T1),IWORK,WORK,LogOverlapFactor);
  comm.broadcast_n(&ovlp,1,0);  
  return ovlp;
}

// Serial Implementation
template< class Tp,
          class integer,
          class MatA,
          class MatB,
          class MatC,
          class MatD,
          class MatE,
          class IBuffer,
          class TBuffer,
          class communicator
        >
Tp OverlapForWoodbury(const MatA& hermA, const MatB& B, Tp LogOverlapFactor, MatC&& QQ0, integer *ref, MatD&& TNN, MatE& TMN, IBuffer& IWORK, TBuffer& WORK, communicator& comm)
{
  // check dimensions are consistent
  int NEL = B.size(1);
  assert( hermA.size(1) == B.size(0) );
  assert( hermA.size(0) == TMN.size(0) );
  assert( B.size(1) == TMN.size(1) );
  assert( B.size(1) == TNN.size(0) );
  assert( B.size(1) == TNN.size(1) );
  assert( hermA.size(0) == QQ0.size(0) );
  assert( B.size(1) == QQ0.size(1) );

  using ma::T;

  int N0,Nn,sz = B.size(1);
  std::tie(N0,Nn) = FairDivideBoundary(comm.rank(),sz,comm.size());
 
  Tp ovlp;
  // T(B)*conj(A) 
  if(N0!=Nn)
    ma::product(hermA,
              B(B.extension(0),{N0,Nn}),
              TMN(TMN.extension(0),{N0,Nn}));
  comm.barrier();
  if(comm.rank()==0) {
    for(int i=0; i<NEL; i++)
      std::copy_n(to_address(TMN[*(ref+i)].origin()),NEL,to_address(TNN[i].origin()));
    ovlp = ma::invert(std::forward<MatD>(TNN),IWORK,WORK,LogOverlapFactor);
  }
  comm.broadcast_n(&ovlp,1,0);

  int M0,Mn;
  sz = TMN.size(0);
  std::tie(M0,Mn) = FairDivideBoundary(comm.rank(),sz,comm.size());

  // QQ0 = TMN * inv(TNN) 
  ma::product(TMN.sliced(M0,Mn),TNN,
              QQ0({M0,Mn},QQ0.extension(1))); //.sliced(M0,Mn)); 
              //QQ0.sliced(M0,Mn)); 
  comm.barrier();
  return ovlp;
}

template< class Tp,
          class integer,
          class MatA,
          class MatB,
          class MatC,
          class MatD,
          class Mat1,
          class Mat2,
          class Mat3,
          class IBuffer,
          class TBuffer,
          class communicator 
        >
Tp MixedDensityMatrixForWoodbury(const MatA& hermA, const MatB& B, MatC&& C, Tp LogOverlapFactor, MatD&& QQ0, integer *ref, Mat1&& TNN, Mat2&& TAB, Mat3&& TNM, IBuffer& IWORK, TBuffer& WORK, communicator& comm, bool compact=true)
{
  // check dimensions are consistent
  int NEL = B.size(1);
  assert( hermA.size(1) == B.size(0) );
  assert( hermA.size(0) == TAB.size(0) );
  assert( B.size(1) == TAB.size(1) );
  assert( B.size(1) == TNN.size(0) );
  assert( B.size(1) == TNN.size(1) );
  assert( hermA.size(0) == QQ0.size(0) );
  assert( B.size(1) == QQ0.size(1) );
  if(compact) {
    assert( C.size(0) == TNN.size(1) );
    assert( C.size(1) == B.size(0) );
  } else {
    assert( TNM.size(1) == B.size(0) );
    assert( TNM.size(0) == TNN.size(1) );
    assert( C.size(0) == hermA.size(1) );
    assert( C.size(1) == TNM.size(1) );
  }

  using ma::T;

  int N0,Nn;
  std::tie(N0,Nn) = FairDivideBoundary(comm.rank(),NEL,comm.size());

  // TAB = herm(A)*B
  if(N0!=Nn) {
    ma::product(hermA,
              B(B.extension(0),{N0,Nn}),
              TAB(TAB.extension(0),{N0,Nn}));  

    // TNN = TAB[ref,:] 
    for(int i=0; i<NEL; i++)
      std::copy_n(to_address(TAB[*(ref+i)].origin())+N0,Nn-N0,to_address(TNN[i].origin())+N0);
  }

  comm.barrier();

  // TNN = TNN^(-1)
  Tp ovlp;
  if(comm.rank()==0)
    ovlp = ma::invert(std::forward<Mat1>(TNN),IWORK,WORK,LogOverlapFactor);
  comm.broadcast_n(&ovlp,1,0);

  int P0,Pn;
  std::tie(P0,Pn) = FairDivideBoundary(comm.rank(),int(TAB.size(0)),comm.size());

  // QQ0 = TAB * inv(TNN) 
  if(P0!=Pn)  
    ma::product(TAB.sliced(P0,Pn),
              TNN,
              QQ0({P0,Pn},QQ0.extension(1)));  
              //QQ0.sliced(P0,Pn));
  if(compact) {

    // C = T(TNN) * T(B)
    if(N0!=Nn)
      ma::product(T(TNN(TNN.extension(0),{N0,Nn})),
                  T(B),
                  C({N0,Nn},C.extension(1))); 

  } else {

    // TNM = T(TNN) * T(B)
    if(N0!=Nn)    
      ma::product(T(TNN(TNN.extension(0),{N0,Nn})),
                  T(B),
                  TNM.sliced(N0,Nn)); 

    int sz=TNM.size(1);
    std::tie(N0,Nn) = FairDivideBoundary(comm.rank(),sz,comm.size());   
    comm.barrier();

    // C = conj(A) * TNM
    ma::product(T(hermA),
                TNM(TNM.extension(0),{N0,Nn}),
                C(C.extension(0), {N0,Nn})); 

  }

  comm.barrier();
  return ovlp;
}

} // namespace shm 

namespace batched
{

template< class MatA,
          class MatB,
          class MatC,
          class Mat,
          class TVec,
          class IBuffer,
          class TBuffer,
          class Tp
        >
void MixedDensityMatrix(const MatA& hermA, std::vector<MatB> &Bi, MatC&& C, Tp LogOverlapFactor, TVec&& ovlp, Mat&& TNN3D, Mat&& TNM3D, IBuffer& IWORK, TBuffer& WORK, bool compact=true)
{
  static_assert( std::decay<TVec>::type::dimensionality == 1, " TVec::dimensionality == 1" );
  static_assert( std::decay<MatB>::type::dimensionality == 2, " MatB::dimensionality == 2" );
  static_assert( std::decay<MatC>::type::dimensionality == 3, " MatC::dimensionality == 3" );
  static_assert( std::decay<Mat>::type::dimensionality == 3, "std::decay<Mat>::type::dimensionality == 3" );

  using ma::T;
  using ma::gemmBatched;
  using ma::getrfBatched;
  using ma::getriBatched;

  int nbatch = Bi.size();
  int NEL = hermA.size(0);
  int NMO = hermA.size(1);
  int wsz = ma::invert_optimal_workspace_size(TNN3D[0]);

  assert( Bi[0].size(0) == NMO );  
  assert( Bi[0].size(1) == NEL );  
  assert( C.size(0) == nbatch );
  assert( C.size(2) == NMO );
  if(compact)
    assert( C.size(1) == NEL );
  else
    assert( C.size(1) == NMO );
  assert( ovlp.size() == nbatch ); 
  assert( TNN3D.size(1) == NEL );
  assert( TNN3D.size(2) == NEL );
  if( not compact) {
    assert( TNM3D.size(0) == nbatch );
    assert( TNM3D.size(1) == NEL );
    assert( TNM3D.size(2) == NMO );
  }
  assert( WORK.num_elements() >= nbatch*wsz );
  assert( IWORK.num_elements() >= nbatch*(NEL+1) );

  using pointer = typename std::decay<MatC>::type::element_ptr;

  int ldw = Bi[0].stride(0);
  int ldN = TNN3D.stride(1);
  int ldC = C.stride(1);
  std::vector<pointer> Carray;
  std::vector<pointer> Warray;
  std::vector<pointer> workArray;
  std::vector<pointer> NNarray;
  Carray.reserve(nbatch);
  Warray.reserve(nbatch);
  workArray.reserve(nbatch);
  NNarray.reserve(nbatch);
  for(int i=0; i<nbatch; i++) {
    NNarray.emplace_back(TNN3D[i].origin());
    workArray.emplace_back(WORK.origin()+i*wsz);
    Carray.emplace_back(C[i].origin());
    Warray.emplace_back(Bi[i].origin());
  }

    // T(conj(A))*B 
    for(int b=0; b<nbatch; ++b)
      ma::product(hermA,Bi[b],TNN3D[b]);
  

    // T1 = T1^(-1)
//    for(int b=0; b<nbatch; ++b) 
//      ma::invert(TNN3D[b],IWORK,WORK,to_address(ovlp.origin())+b);
    // Invert
    getrfBatched(NEL,NNarray.data(),ldN, IWORK.origin(), IWORK.origin()+nbatch*NEL, nbatch);

    for(int i=0; i<nbatch; i++) {

      using ma::determinant_from_getrf;
      ovlp[i] = determinant_from_getrf(NEL, NNarray[i], ldN, IWORK.origin()+i*NEL,LogOverlapFactor);

    }

    getriBatched(NEL,NNarray.data(),ldN, IWORK.origin(), workArray.data(), wsz, 
                 IWORK.origin()+nbatch*NEL, nbatch);

    if(compact) {

      // C = T(T1) * T(B)
//      for(int b=0; b<nbatch; ++b)
//        ma::product(T(TNN3D[b]),T(Bi[b]),C[b]);
      // careful with fortan ordering
      gemmBatched('T','T',NMO,NEL,NEL,ComplexType(1.0),Warray.data(),ldw,NNarray.data(),ldN,
                  ComplexType(0.0),Carray.data(),ldC,nbatch);

    } else {

      int ldM = TNM3D.stride(1);
      std::vector<pointer> NMarray;
      NMarray.reserve(nbatch);
      for(int i=0; i<nbatch; i++) 
        NMarray.emplace_back(TNM3D[i].origin());

      // T2 = T(T1) * T(B)
      //for(int b=0; b<nbatch; ++b)
      //  ma::product(T(TNN3D[b]),T(Bi[b]),TNM3D[b]);
      gemmBatched('T','T',NMO,NEL,NEL,ComplexType(1.0),Warray.data(),ldw,NNarray.data(),ldN,
                  ComplexType(0.0),NMarray.data(),ldM,nbatch);

      // C = conj(A) * T2
      for(int b=0; b<nbatch; ++b)
        ma::product(T(hermA),TNM3D[b],C[b]);

    }


}

template< class MatA,
          class MatB,
          class Mat,
          class TVec,
          class IBuffer,
          class Tp
        >
void Overlap(const MatA& hermA, std::vector<MatB> &Bi, Tp LogOverlapFactor, TVec&& ovlp, Mat&& TNN3D, IBuffer& IWORK)
{
  static_assert( std::decay<TVec>::type::dimensionality == 1, " TVec::dimensionality == 1" );
  static_assert( std::decay<MatB>::type::dimensionality == 2, " MatB::dimensionality == 2" );
  static_assert( std::decay<Mat>::type::dimensionality == 3, "std::decay<Mat>::type::dimensionality == 3" );

  using ma::T;
  using ma::gemmBatched;
  using ma::getrfBatched;

  int nbatch = Bi.size();
  int NEL = hermA.size(0);
  int NMO = hermA.size(1);

  assert( Bi[0].size(0) == NMO );  
  assert( Bi[0].size(1) == NEL );  
  assert( ovlp.size() == nbatch ); 
  assert( TNN3D.size(1) == NEL );
  assert( TNN3D.size(2) == NEL );
  assert( IWORK.num_elements() >= nbatch*(NEL+1) );

  using pointer = typename std::decay<Mat>::type::element_ptr;

  int ldw = Bi[0].stride(0);
  int ldN = TNN3D.stride(1);
  std::vector<pointer> Warray;
  std::vector<pointer> NNarray;
  Warray.reserve(nbatch);
  NNarray.reserve(nbatch);
  for(int i=0; i<nbatch; i++) {
    NNarray.emplace_back(TNN3D[i].origin());
    Warray.emplace_back(Bi[i].origin());
  }

    // T(conj(A))*B 
    for(int b=0; b<nbatch; ++b)
      ma::product(hermA,Bi[b],TNN3D[b]);
  

    // T1 = T1^(-1)
//    for(int b=0; b<nbatch; ++b) 
//      ma::invert(TNN3D[b],IWORK,WORK,to_address(ovlp.origin())+b);
    // Invert
    getrfBatched(NEL,NNarray.data(),ldN, IWORK.origin(), IWORK.origin()+nbatch*NEL, nbatch);

    for(int i=0; i<nbatch; i++) {

      using ma::determinant_from_getrf;
      ovlp[i] = determinant_from_getrf(NEL, NNarray[i], ldN, IWORK.origin()+i*NEL,LogOverlapFactor);

    }

}


} // namespace batched

} // namespace SlaterDeterminantOperations

} // namespace afqmc

} // namespace qmcplusplus 

#endif

