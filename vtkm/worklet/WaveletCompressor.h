//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_waveletcompressor_h
#define vtk_m_worklet_waveletcompressor_h

#include <vtkm/worklet/wavelets/WaveletDWT.h>

#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace worklet {

class WaveletCompressor : public vtkm::worklet::wavelets::WaveletDWT
{
public:

  // Constructor
  WaveletCompressor( wavelets::WaveletName name ) : WaveletDWT( name ) {} 

  // Multi-level 1D wavelet decomposition
  template< typename SignalArrayType, typename CoeffArrayType, typename DeviceTag >
  VTKM_CONT_EXPORT
  vtkm::Id WaveDecompose( const SignalArrayType      &sigIn,   // Input
                                vtkm::Id             nLevels,  // n levels of DWT
                                CoeffArrayType       &coeffOut,
                                std::vector<vtkm::Id> &L,
                                DeviceTag                     )
  {
    vtkm::Id sigInLen = sigIn.GetNumberOfValues();
    if( nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel( sigInLen ) )
    {
      throw vtkm::cont::ErrorControlBadValue("Number of levels of transform is not supported! ");
    }
    if( nLevels == 0 )  //  0 levels means no transform
    {
      //WaveletBase::DeviceCopy( sigIn, coeffOut, DeviceTag );
      vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( sigIn, coeffOut );
      return 0;
    }

    L.resize( size_t(nLevels + 2) );
    this->ComputeL( sigInLen, nLevels, L );
    vtkm::Id CLength = this->ComputeCoeffLength( L, nLevels );
    VTKM_ASSERT( CLength == sigInLen );


    vtkm::Id sigInPtr = 0;  // pseudo pointer for the beginning of input array 
    vtkm::Id len = sigInLen;
    vtkm::Id cALen = WaveletBase::GetApproxLength( len );
    vtkm::Id cptr;          // pseudo pointer for the beginning of output array
    vtkm::Id tlen = 0;
    std::vector<vtkm::Id> L1d(3, 0);

    // Use an intermediate array
    typedef typename CoeffArrayType::ValueType          OutputValueType;
    typedef vtkm::cont::ArrayHandle< OutputValueType >  InterArrayType;

    // Define a few more types
    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >      IdArrayType;
    typedef vtkm::cont::ArrayHandlePermutation< IdArrayType, CoeffArrayType > 
              PermutArrayType;

    //WaveletBase::DeviceCopy( sigIn, coeffOut );
    vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( sigIn, coeffOut );

    for( vtkm::Id i = nLevels; i > 0; i-- )
    {
      tlen += L[ size_t(i) ];
      cptr = 0 + CLength - tlen - cALen;
      
      // make input array (permutation array)
      IdArrayType       inputIndices( sigInPtr, 1, len );
      PermutArrayType   input( inputIndices, coeffOut ); 
      // make output array 
      InterArrayType    output;

      WaveletDWT::DWT1D( input, output, L1d );

      // move intermediate results to final array
      WaveletBase::DeviceCopyStartX( output, coeffOut, cptr );

      // update pseudo pointers
      len = cALen;
      cALen = WaveletBase::GetApproxLength( cALen );
      sigInPtr = cptr;
    }

    return 0;
  }

  // Multi-level 1D wavelet reconstruction
  template< typename CoeffArrayType, typename SignalArrayType, typename DeviceTag >
  VTKM_CONT_EXPORT
  vtkm::Id WaveReconstruct( const CoeffArrayType     &coeffIn,   // Input
                                  vtkm::Id           nLevels,    // n levels of DWT
                                  std::vector<vtkm::Id> &L,
                                  SignalArrayType    &sigOut,
                                  DeviceTag                  )
  {
    VTKM_ASSERT( nLevels > 0 );
    vtkm::Id LLength = nLevels + 2;
    VTKM_ASSERT( vtkm::Id(L.size()) == LLength );

    //vtkm::Id L1d[3] = {L[0], L[1], 0};
    std::vector<vtkm::Id> L1d(3, 0);  // three elements
    L1d[0] = L[0];
    L1d[1] = L[1];

    typedef typename SignalArrayType::ValueType              OutValueType;
    typedef vtkm::cont::ArrayHandle< OutValueType >          OutArrayBasic;
    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >      IdArrayType;
    typedef vtkm::cont::ArrayHandlePermutation< IdArrayType, SignalArrayType > 
                  PermutArrayType;

    //WaveletBase::DeviceCopy( coeffIn, sigOut );
    vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( coeffIn, sigOut );

    for( vtkm::Id i = 1; i <= nLevels; i++ )
    {
      L1d[2] = this->GetApproxLengthLevN( L[ size_t(LLength-1) ], nLevels-i );

      // Make an input array
      IdArrayType inputIndices( 0, 1, L1d[2] );
      PermutArrayType input( inputIndices, sigOut ); 
      
      // Make an output array
      OutArrayBasic output;
      
      WaveletDWT::IDWT1D( input, L1d, output );
      VTKM_ASSERT( output.GetNumberOfValues() == L1d[2] );

      // Move output to intermediate array
      WaveletBase::DeviceCopyStartX( output, sigOut, 0 );

      L1d[0] = L1d[2];
      L1d[1] = L[ size_t(i+1) ];
    }

    return 0;
  }

  // Squash coefficients smaller than a threshold
  template< typename CoeffArrayType, typename DeviceTag >
  vtkm::Id SquashCoefficients( CoeffArrayType   &coeffIn,
                               vtkm::Float64    ratio,
                                   DeviceTag            )
  {
    if( ratio > 1 )
    {
      vtkm::Id coeffLen = coeffIn.GetNumberOfValues();
      typedef typename CoeffArrayType::ValueType ValueType;
      typedef vtkm::cont::ArrayHandle< ValueType > CoeffArrayBasic;
      CoeffArrayBasic sortedArray;
      //WaveletBase::DeviceCopy( coeffIn, sortedArray );
      vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( coeffIn, sortedArray );
      WaveletBase::DeviceSort( sortedArray, DeviceTag() );
      
      vtkm::Id n = coeffLen - 
                   static_cast<vtkm::Id>( static_cast<vtkm::Float64>(coeffLen)/ratio );
      typedef vtkm::worklet::wavelets::ThresholdWorklet ThresholdType;
      ThresholdType tw( n );
      vtkm::worklet::DispatcherMapField< ThresholdType > dispatcher( tw  );
      dispatcher.Invoke( coeffIn, sortedArray );
    } 

    return 0;
  }


  // Report statistics on reconstructed array
  template< typename ArrayType, typename DeviceTag >
  vtkm::Id EvaluateReconstruction( const ArrayType &original,
                                   const ArrayType &reconstruct,
                                         DeviceTag )
  {
    #define VAL        vtkm::Float64
    #define MAKEVAL(a) (static_cast<VAL>(a))
    VAL VarOrig = WaveletBase::DeviceCalculateVariance( original, DeviceTag() );

    typedef typename ArrayType::ValueType ValueType;
    typedef vtkm::cont::ArrayHandle< ValueType > ArrayBasic;
    ArrayBasic errorArray, errorSquare;

    // Use a worklet to calculate point-wise error, and its square
    typedef vtkm::worklet::wavelets::Differencer DifferencerWorklet;
    DifferencerWorklet dw;
    vtkm::worklet::DispatcherMapField< DifferencerWorklet > dwDispatcher( dw  );
    dwDispatcher.Invoke( original, reconstruct, errorArray );

    typedef vtkm::worklet::wavelets::SquareWorklet SquareWorklet;
    SquareWorklet sw;
    vtkm::worklet::DispatcherMapField< SquareWorklet > swDispatcher( sw );
    swDispatcher.Invoke( errorArray, errorSquare );

    VAL varErr   = WaveletBase::DeviceCalculateVariance( errorArray, DeviceTag() );
    VAL snr, decibels;
    if( varErr != 0.0 )
    {
        snr      = VarOrig / varErr;
        decibels = 10 * vtkm::Log10( snr );
    }
    else
    {
        snr      = vtkm::Infinity64();
        decibels = vtkm::Infinity64();
    }

    VAL origMax  = WaveletBase::DeviceMax( original, DeviceTag() );
    VAL origMin  = WaveletBase::DeviceMin( original, DeviceTag() );
    VAL errorMax = WaveletBase::DeviceMaxAbs( errorArray, DeviceTag() );
    VAL range    = origMax - origMin;

    VAL squareSum = WaveletBase::DeviceSum( errorSquare, DeviceTag() );
    VAL rmse      = vtkm::Sqrt( MAKEVAL(squareSum) / MAKEVAL(errorArray.GetNumberOfValues()) );

    std::cout << "Data range             = " << range << std::endl;
    std::cout << "SNR                    = " << snr << std::endl;
    std::cout << "SNR in decibels        = " << decibels << std::endl;
    std::cout << "L-infy norm            = " << errorMax 
              << ", after normalization  = " << errorMax / range << std::endl;
    std::cout << "RMSE                   = " << rmse 
              << ", after normalization  = " << rmse / range << std::endl;

    #undef MAKEVAL
    #undef VAL

    return 0;
  }

                      
  // Compute the book keeping array L for 1D wavelet decomposition
  void ComputeL( vtkm::Id               sigInLen, 
                 vtkm::Id               nLevels, 
                 std::vector<vtkm::Id>  &L )
  {
    VTKM_ASSERT( vtkm::Id( L.size() )  == (nLevels + 2) );
    L[ size_t(nLevels+1) ] = sigInLen;
    L[ size_t(nLevels)   ] = sigInLen;
    for( size_t i = size_t(nLevels); i > 0; i-- )
    {
      L[i-1] = WaveletBase::GetApproxLength( L[i] );
      L[i]   = WaveletBase::GetDetailLength( L[i] );
    }
  }

  // Compute the length of coefficients
  vtkm::Id ComputeCoeffLength( std::vector<vtkm::Id> &L,
                               vtkm::Id nLevels )
  {
    VTKM_ASSERT( vtkm::Id(L.size()) == (nLevels + 2) );
    vtkm::Id sum = L[0];        // 1st level cA
    for( size_t i = 1; i <= size_t(nLevels); i++ )
      sum += L[i];
    return sum;
  }

  // Compute approximate coefficient length at a specific level
  vtkm::Id GetApproxLengthLevN( vtkm::Id sigInLen, vtkm::Id levN )
  {
    vtkm::Id cALen = sigInLen;
    for( vtkm::Id i = 0; i < levN; i++ )
    {
      cALen = WaveletBase::GetApproxLength( cALen );
      if( cALen == 0 )    
        return cALen;
    }

    return cALen;
  }


};    // class WaveletCompressor

}     // namespace worklet
}     // namespace vtkm

#endif 