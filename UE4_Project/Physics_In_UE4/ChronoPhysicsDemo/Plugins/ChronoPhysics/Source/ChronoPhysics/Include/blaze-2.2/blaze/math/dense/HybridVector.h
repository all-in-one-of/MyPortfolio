//=================================================================================================
/*!
//  \file blaze/math/dense/HybridVector.h
//  \brief Header file for the HybridVector class template
//
//  Copyright (C) 2013 Klaus Iglberger - All Rights Reserved
//
//  This file is part of the Blaze library. You can redistribute it and/or modify it under
//
//  * The names of its contributors may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
*/
//=================================================================================================

#ifndef _BLAZE_MATH_DENSE_HYBRIDVECTOR_H_
#define _BLAZE_MATH_DENSE_HYBRIDVECTOR_H_


//*************************************************************************************************
// Includes
//*************************************************************************************************

#include <algorithm>
#include <stdexcept>
#include <blaze/math/dense/DenseIterator.h>
#include <blaze/math/expressions/DenseVector.h>
#include <blaze/math/expressions/SparseVector.h>
#include <blaze/math/Forward.h>
#include <blaze/math/Functions.h>
#include <blaze/math/Intrinsics.h>
#include <blaze/math/shims/Clear.h>
#include <blaze/math/shims/IsDefault.h>
#include <blaze/math/shims/Serial.h>
#include <blaze/math/traits/AddTrait.h>
#include <blaze/math/traits/CrossTrait.h>
#include <blaze/math/traits/DivTrait.h>
#include <blaze/math/traits/MathTrait.h>
#include <blaze/math/traits/MultTrait.h>
#include <blaze/math/traits/SubTrait.h>
#include <blaze/math/traits/SubvectorTrait.h>
#include <blaze/math/typetraits/IsResizable.h>
#include <blaze/math/typetraits/IsSparseVector.h>
#include <blaze/system/TransposeFlag.h>
#include <blaze/util/AlignedArray.h>
#include <blaze/util/Assert.h>
#include <blaze/util/constraints/Const.h>
#include <blaze/util/constraints/Numeric.h>
#include <blaze/util/constraints/Pointer.h>
#include <blaze/util/constraints/Reference.h>
#include <blaze/util/constraints/Vectorizable.h>
#include <blaze/util/constraints/Volatile.h>
#include <blaze/util/DisableIf.h>
#include <blaze/util/EnableIf.h>
#include <blaze/util/Memory.h>
#include <blaze/util/StaticAssert.h>
#include <blaze/util/Template.h>
#include <blaze/util/Types.h>
#include <blaze/util/typetraits/IsNumeric.h>
#include <blaze/util/typetraits/IsSame.h>
#include <blaze/util/typetraits/IsVectorizable.h>
#include <blaze/util/Unused.h>


namespace blaze {

//=================================================================================================
//
//  CLASS DEFINITION
//
//=================================================================================================

//*************************************************************************************************
/*!\defgroup hybrid_vector HybridVector
// \ingroup dense_vector
*/
/*!\brief Efficient implementation of a dynamically sized vector with static memory.
// \ingroup hybrid_vector
//
// The HybridVector class template combines the flexibility of a dynamically sized vector with
// the efficiency and performance of a fixed-size vector. It is implemented as a crossing between
// the blaze::StaticVector and the blaze::DynamicVector class templates: Similar to the static
// vector it uses static stack memory instead of dynamically allocated memory and similar to the
// dynamic vector it can be resized (within the extend of the static memory). The type of the
// elements, the maximum number of elements and the transpose flag of the vector can be specified
// via the three template parameters:

   \code
   template< typename Type, size_t N, bool TF >
   class HybridVector;
   \endcode

//  - Type: specifies the type of the vector elements. HybridVector can be used with any
//          non-cv-qualified, non-reference, non-pointer element type.
//  - N   : specifies the maximum number of vector elements, i.e. the maximum size of the vector.
//          It is expected that HybridVector is only used for tiny and small vectors.
//  - TF  : specifies whether the vector is a row vector (\a blaze::rowVector) or a column
//          vector (\a blaze::columnVector). The default value is \a blaze::columnVector.
//
// These contiguously stored elements can be directly accessed with the subscript operator. The
// numbering of the vector elements is

                             \f[\left(\begin{array}{*{4}{c}}
                             0 & 1 & \cdots & N-1 \\
                             \end{array}\right)\f]

// The use of HybridVector is very natural and intuitive. All operations (addition, subtraction,
// multiplication, scaling, ...) can be performed on all possible combinations of dense and sparse
// vectors with fitting element types. The following example gives an impression of the use of a
// 2-dimensional HybridVector:

   \code
   using blaze::HybridVector;
   using blaze::CompressedVector;
   using blaze::StaticMatrix;

   HybridVector<double,2UL> a( 2 );  // Non-initialized 2D vector of size 2
   a[0] = 1.0;                       // Initialization of the first element
   a[1] = 2.0;                       // Initialization of the second element

   HybridVector<double,2UL> b( 2, 2.0 );  // Directly, homogeneously initialized 2D vector
   CompressedVector<float>  c( 2 );       // Empty sparse single precision vector
   HybridVector<double,2UL> d;            // Default constructed hybrid vector
   StaticMatrix<double,2UL,2UL> A;        // Default constructed static row-major matrix

   d = a + b;  // Vector addition between vectors of equal element type
   d = a - c;  // Vector subtraction between a dense and sparse vector with different element types
   d = a * b;  // Component-wise vector multiplication

   a *= 2.0;      // In-place scaling of vector
   d  = a * 2.0;  // Scaling of vector a
   d  = 2.0 * a;  // Scaling of vector a

   d += a - b;  // Addition assignment
   d -= a + c;  // Subtraction assignment
   d *= a * b;  // Multiplication assignment

   double scalar = trans( a ) * b;  // Scalar/dot/inner product between two vectors

   A = a * trans( b );  // Outer product between two vectors
   \endcode
*/
template< typename Type                     // Data type of the vector
        , size_t N                          // Number of elements
        , bool TF = defaultTransposeFlag >  // Transpose flag
class HybridVector : public DenseVector< HybridVector<Type,N,TF>, TF >
{
 private:
   //**Type definitions****************************************************************************
   typedef IntrinsicTrait<Type>  IT;  //!< Intrinsic trait for the vector element type.
   //**********************************************************************************************

   //**********************************************************************************************
   //! Alignment adjustment
   enum { NN = N + ( IT::size - ( N % IT::size ) ) % IT::size };
   //**********************************************************************************************

 public:
   //**Type definitions****************************************************************************
   typedef HybridVector<Type,N,TF>    This;            //!< Type of this HybridVector instance.
   typedef This                       ResultType;      //!< Result type for expression template evaluations.
   typedef HybridVector<Type,N,!TF>   TransposeType;   //!< Transpose type for expression template evaluations.
   typedef Type                       ElementType;     //!< Type of the vector elements.
   typedef typename IT::Type          IntrinsicType;   //!< Intrinsic type of the vector elements.
   typedef const Type&                ReturnType;      //!< Return type for expression template evaluations.
   typedef const HybridVector&        CompositeType;   //!< Data type for composite expression templates.
   typedef Type&                      Reference;       //!< Reference to a non-constant vector value.
   typedef const Type&                ConstReference;  //!< Reference to a constant vector value.
   typedef Type*                      Pointer;         //!< Pointer to a non-constant vector value.
   typedef const Type*                ConstPointer;    //!< Pointer to a constant vector value.
   typedef DenseIterator<Type>        Iterator;        //!< Iterator over non-constant elements.
   typedef DenseIterator<const Type>  ConstIterator;   //!< Iterator over constant elements.
   //**********************************************************************************************

   //**Rebind struct definition********************************************************************
   /*!\brief Rebind mechanism to obtain a HybridVector with different data/element type.
   */
   template< typename ET >  // Data type of the other vector
   struct Rebind {
      typedef HybridVector<ET,N,TF>  Other;  //!< The type of the other HybridVector.
   };
   //**********************************************************************************************

   //**Compilation flags***************************************************************************
   //! Compilation flag for intrinsic optimization.
   /*! The \a vectorizable compilation flag indicates whether expressions the vector is involved
       in can be optimized via intrinsics. In case the element type of the vector is a vectorizable
       data type, the \a vectorizable compilation flag is set to \a true, otherwise it is set to
       \a false. */
   enum { vectorizable = IsVectorizable<Type>::value };

   //! Compilation flag for SMP assignments.
   /*! The \a smpAssignable compilation flag indicates whether the vector can be used in SMP
       (shared memory parallel) assignments (both on the left-hand and right-hand side of the
       assignment). */
   enum { smpAssignable = 0 };
   //**********************************************************************************************

   //**Constructors********************************************************************************
   /*!\name Constructors */
   //@{
                              explicit inline HybridVector();
                              explicit inline HybridVector( size_t n );
                              explicit inline HybridVector( size_t n, const Type& init );
   template< typename Other > explicit inline HybridVector( size_t n, const Other* array );

   template< typename Other, size_t M >
   explicit inline HybridVector( const Other (&array)[M] );

                           inline HybridVector( const HybridVector& v );
   template< typename VT > inline HybridVector( const Vector<VT,TF>& v );
   //@}
   //**********************************************************************************************

   //**Destructor**********************************************************************************
   // No explicitly declared destructor.
   //**********************************************************************************************

   //**Data access functions***********************************************************************
   /*!\name Data access functions */
   //@{
   inline Reference      operator[]( size_t index );
   inline ConstReference operator[]( size_t index ) const;
   inline Pointer        data  ();
   inline ConstPointer   data  () const;
   inline Iterator       begin ();
   inline ConstIterator  begin () const;
   inline ConstIterator  cbegin() const;
   inline Iterator       end   ();
   inline ConstIterator  end   () const;
   inline ConstIterator  cend  () const;
   //@}
   //**********************************************************************************************

   //**Assignment operators************************************************************************
   /*!\name Assignment operators */
   //@{
   template< typename Other, size_t M >
   inline HybridVector& operator=( const Other (&array)[M] );

                           inline HybridVector& operator= ( const Type& rhs );
                           inline HybridVector& operator= ( const HybridVector& rhs );
   template< typename VT > inline HybridVector& operator= ( const Vector<VT,TF>& rhs );
   template< typename VT > inline HybridVector& operator+=( const Vector<VT,TF>& rhs );
   template< typename VT > inline HybridVector& operator-=( const Vector<VT,TF>& rhs );
   template< typename VT > inline HybridVector& operator*=( const Vector<VT,TF>& rhs );

   template< typename Other >
   inline typename EnableIf< IsNumeric<Other>, HybridVector >::Type&
      operator*=( Other rhs );

   template< typename Other >
   inline typename EnableIf< IsNumeric<Other>, HybridVector >::Type&
      operator/=( Other rhs );
   //@}
   //**********************************************************************************************

   //**Utility functions***************************************************************************
   /*!\name Utility functions */
   //@{
                              inline size_t        size() const;
                              inline size_t        capacity() const;
                              inline size_t        nonZeros() const;
                              inline void          reset();
                              inline void          clear();
                              inline void          resize( size_t n, bool preserve=true );
                              inline void          extend( size_t n, bool preserve=true );
   template< typename Other > inline HybridVector& scale( const Other& scalar );
                              inline void          swap( HybridVector& v ) /* throw() */;
   //@}
   //**********************************************************************************************

   //**Memory functions****************************************************************************
   /*!\name Memory functions */
   //@{
   static inline void* operator new  ( std::size_t size );
   static inline void* operator new[]( std::size_t size );
   static inline void* operator new  ( std::size_t size, const std::nothrow_t& );
   static inline void* operator new[]( std::size_t size, const std::nothrow_t& );

   static inline void operator delete  ( void* ptr );
   static inline void operator delete[]( void* ptr );
   static inline void operator delete  ( void* ptr, const std::nothrow_t& );
   static inline void operator delete[]( void* ptr, const std::nothrow_t& );
   //@}
   //**********************************************************************************************

 private:
   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename VT >
   struct VectorizedAssign {
      enum { value = vectorizable && VT::vectorizable &&
                     IsSame<Type,typename VT::ElementType>::value };
   };
   /*! \endcond */
   //**********************************************************************************************

   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename VT >
   struct VectorizedAddAssign {
      enum { value = vectorizable && VT::vectorizable &&
                     IsSame<Type,typename VT::ElementType>::value &&
                     IntrinsicTrait<Type>::addition };
   };
   /*! \endcond */
   //**********************************************************************************************

   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename VT >
   struct VectorizedSubAssign {
      enum { value = vectorizable && VT::vectorizable &&
                     IsSame<Type,typename VT::ElementType>::value &&
                     IntrinsicTrait<Type>::subtraction };
   };
   /*! \endcond */
   //**********************************************************************************************

   //**********************************************************************************************
   /*! \cond BLAZE_INTERNAL */
   //! Helper structure for the explicit application of the SFINAE principle.
   template< typename VT >
   struct VectorizedMultAssign {
      enum { value = vectorizable && VT::vectorizable &&
                     IsSame<Type,typename VT::ElementType>::value &&
                     IntrinsicTrait<Type>::multiplication };
   };
   /*! \endcond */
   //**********************************************************************************************

 public:
   //**Expression template evaluation functions****************************************************
   /*!\name Expression template evaluation functions */
   //@{
   template< typename Other > inline bool canAlias ( const Other* alias ) const;
   template< typename Other > inline bool isAliased( const Other* alias ) const;

   inline bool isAligned() const;

   inline IntrinsicType load  ( size_t index ) const;
   inline IntrinsicType loadu ( size_t index ) const;
   inline void          store ( size_t index, const IntrinsicType& value );
   inline void          storeu( size_t index, const IntrinsicType& value );
   inline void          stream( size_t index, const IntrinsicType& value );

   template< typename VT >
   inline typename DisableIf< VectorizedAssign<VT> >::Type
      assign( const DenseVector<VT,TF>& rhs );

   template< typename VT >
   inline typename EnableIf< VectorizedAssign<VT> >::Type
      assign( const DenseVector<VT,TF>& rhs );

   template< typename VT > inline void assign( const SparseVector<VT,TF>& rhs );

   template< typename VT >
   inline typename DisableIf< VectorizedAddAssign<VT> >::Type
      addAssign( const DenseVector<VT,TF>& rhs );

   template< typename VT >
   inline typename EnableIf< VectorizedAddAssign<VT> >::Type
      addAssign( const DenseVector<VT,TF>& rhs );

   template< typename VT > inline void addAssign( const SparseVector<VT,TF>& rhs );

   template< typename VT >
   inline typename DisableIf< VectorizedSubAssign<VT> >::Type
      subAssign( const DenseVector<VT,TF>& rhs );

   template< typename VT >
   inline typename EnableIf< VectorizedSubAssign<VT> >::Type
      subAssign( const DenseVector<VT,TF>& rhs );

   template< typename VT > inline void subAssign( const SparseVector<VT,TF>& rhs );

   template< typename VT >
   inline typename DisableIf< VectorizedMultAssign<VT> >::Type
      multAssign( const DenseVector<VT,TF>& rhs );

   template< typename VT >
   inline typename EnableIf< VectorizedMultAssign<VT> >::Type
      multAssign( const DenseVector<VT,TF>& rhs );

   template< typename VT > inline void multAssign( const SparseVector<VT,TF>& rhs );
   //@}
   //**********************************************************************************************

 private:
   //**Member variables****************************************************************************
   /*!\name Member variables */
   //@{
   AlignedArray<Type,NN> v_;  //!< The statically allocated vector elements.
                              /*!< Access to the vector values is gained via the subscript
                                   operator. The order of the elements is
                                   \f[\left(\begin{array}{*{4}{c}}
                                   0 & 1 & \cdots & N-1 \\
                                   \end{array}\right)\f] */
   size_t size_;              //!< The current size/dimension of the vector.
   //@}
   //**********************************************************************************************

   //**Compile time checks*************************************************************************
   /*! \cond BLAZE_INTERNAL */
   BLAZE_CONSTRAINT_MUST_NOT_BE_POINTER_TYPE  ( Type );
   BLAZE_CONSTRAINT_MUST_NOT_BE_REFERENCE_TYPE( Type );
   BLAZE_CONSTRAINT_MUST_NOT_BE_CONST         ( Type );
   BLAZE_CONSTRAINT_MUST_NOT_BE_VOLATILE      ( Type );
   BLAZE_STATIC_ASSERT( NN % IT::size == 0UL );
   BLAZE_STATIC_ASSERT( NN >= N );
   /*! \endcond */
   //**********************************************************************************************
};
//*************************************************************************************************




//=================================================================================================
//
//  CONSTRUCTORS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief The default constructor for HybridVector.
//
// The size of a default constructed HybridVector is initially set to 0.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline HybridVector<Type,N,TF>::HybridVector()
   : v_   ()       // The statically allocated vector elements
   , size_( 0UL )  // The current size/dimension of the vector
{
   BLAZE_STATIC_ASSERT( IsVectorizable<Type>::value || NN == N );

   if( IsNumeric<Type>::value ) {
      for( size_t i=0UL; i<NN; ++i )
         v_[i] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Constructor for a vector of size \a n.
//
// \param n The size of the vector.
// \exception std::invalid_argument Invalid size for hybrid vector.
//
// This constructor creates a hybrid vector of size \a n and initializes all vector elements to
// the default value (for instance 0 for integral types). In case \a n is larger than the maximum
// allowed number of elements (i.e. \a n > N) a \a std::invalid_argument exception is thrown.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline HybridVector<Type,N,TF>::HybridVector( size_t n )
   : v_   ()     // The statically allocated vector elements
   , size_( n )  // The current size/dimension of the vector
{
   BLAZE_STATIC_ASSERT( IsVectorizable<Type>::value || NN == N );

   if( n > N )
      throw std::invalid_argument( "Invalid size for hybrid vector" );

   if( IsNumeric<Type>::value ) {
      for( size_t i=0UL; i<NN; ++i )
         v_[i] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Constructor for a homogeneous initialization of all \a n vector elements.
//
// \param n The size of the vector.
// \param init The initial value of the vector elements.
// \exception std::invalid_argument Invalid size for hybrid vector.
//
// This constructor creates a hybrid vector of size \a n and initializes all vector elements with
// the specified value. In case \a n is larger than the maximum allowed number of elements (i.e.
// \a n > N) a \a std::invalid_argument exception is thrown.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline HybridVector<Type,N,TF>::HybridVector( size_t n, const Type& init )
   : v_   ()     // The statically allocated vector elements
   , size_( n )  // The current size/dimension of the vector
{
   BLAZE_STATIC_ASSERT( IsVectorizable<Type>::value || NN == N );

   if( n > N )
      throw std::invalid_argument( "Invalid size for hybrid vector" );

   for( size_t i=0UL; i<n; ++i )
      v_[i] = init;

   if( IsNumeric<Type>::value ) {
      for( size_t i=n; i<NN; ++i )
         v_[i] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Array initialization of all vector elements.
//
// \param n The size of the vector.
// \param array Dynamic array for the initialization.
// \exception std::invalid_argument Invalid size for hybrid vector.
//
// This assignment operator offers the option to directly initialize the elements of the vector
// with a dynamic array:

   \code
   double* array = new double[6];
   // ... Initialization of the dynamic array
   blaze::HybridVector<double,6> v( array, 6UL );
   delete[] array;
   \endcode

// The vector is sized according to the size of the array and initialized with the values from
// the given array. In case the size of the given array exceeds the maximum size of the hybrid
// vector (i.e. is larger than N), a \a std::invalid_argument exception is thrown.\n
// Note that it is expected that the given \a array has at least \a n elements. Providing an
// array with less elements results in undefined behavior!
*/
template< typename Type     // Data type of the vector
        , size_t N          // Number of elements
        , bool TF >         // Transpose flag
template< typename Other >  // Data type of the initialization array
inline HybridVector<Type,N,TF>::HybridVector( size_t n, const Other* array )
   : v_   ()     // The statically allocated vector elements
   , size_( n )  // The current size/dimension of the vector
{
   BLAZE_STATIC_ASSERT( IsVectorizable<Type>::value || NN == N );

   if( n > N )
      throw std::invalid_argument( "Invalid setup of hybrid vector" );

   for( size_t i=0UL; i<n; ++i )
      v_[i] = array[i];

   if( IsNumeric<Type>::value ) {
      for( size_t i=n; i<NN; ++i )
         v_[i] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Array initialization of all vector elements.
//
// \param array M-dimensional array for the initialization.
//
// This assignment operator offers the option to directly initialize the elements of the vector
// with a static array:

   \code
   const double init[2] = { 1.0, 2.0 };
   blaze::HybridVector<double,4> v( init );
   \endcode

// The vector is sized according to the size of the array and initialized with the values from
// the given array. This constructor only works for arrays with a size smaller-or-equal than the
// maximum number of elements of the hybrid vector (i.e. M <= N). The attempt to use a larger
// array will result in a compile time error.
*/
template< typename Type   // Data type of the vector
        , size_t N        // Number of elements
        , bool TF >       // Transpose flag
template< typename Other  // Data type of the initialization array
        , size_t M >      // Number of elements of the initialization array
inline HybridVector<Type,N,TF>::HybridVector( const Other (&array)[M] )
   : v_   ()     // The statically allocated vector elements
   , size_( M )  // The current size/dimension of the vector
{
   BLAZE_STATIC_ASSERT( M <= N );
   BLAZE_STATIC_ASSERT( IsVectorizable<Type>::value || NN == N );

   for( size_t i=0UL; i<M; ++i )
      v_[i] = array[i];

   if( IsNumeric<Type>::value ) {
      for( size_t i=M; i<NN; ++i )
         v_[i] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief The copy constructor for HybridVector.
//
// \param v Vector to be copied.
//
// The copy constructor is explicitly defined in order to enable/facilitate NRV optimization.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline HybridVector<Type,N,TF>::HybridVector( const HybridVector& v )
   : v_   ()           // The statically allocated vector elements
   , size_( v.size_ )  // The current size/dimension of the vector
{
   BLAZE_STATIC_ASSERT( IsVectorizable<Type>::value || NN == N );

   for( size_t i=0UL; i<size_; ++i )
      v_[i] = v.v_[i];

   if( IsNumeric<Type>::value ) {
      for( size_t i=size_; i<NN; ++i )
         v_[i] = Type();
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Conversion constructor from different vectors.
//
// \param v Vector to be copied.
// \exception std::invalid_argument Invalid setup of hybrid vector.
//
// This constructor initializes the hybrid vector from the given vector. In case the size of
// the given vector exceeds the maximum size of the hybrid vector (i.e. is larger than N), a
// \a std::invalid_argument exception is thrown.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the foreign vector
inline HybridVector<Type,N,TF>::HybridVector( const Vector<VT,TF>& v )
   : v_   ()               // The statically allocated vector elements
   , size_( (~v).size() )  // The current size/dimension of the vector
{
   using blaze::assign;

   BLAZE_STATIC_ASSERT( IsVectorizable<Type>::value || NN == N );

   if( (~v).size() > N )
      throw std::invalid_argument( "Invalid setup of hybrid vector" );

   for( size_t i=( IsSparseVector<VT>::value ? 0UL : size_ );
               i<( IsNumeric<Type>::value    ? NN  : size_ ); ++i ) {
      v_[i] = Type();
   }

   assign( *this, ~v );
}
//*************************************************************************************************




//=================================================================================================
//
//  DATA ACCESS FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Subscript operator for the direct access to the vector elements.
//
// \param index Access index. The index has to be in the range \f$[0..N-1]\f$.
// \return Reference to the accessed value.
//
// In case BLAZE_USER_ASSERT() is active, this operator performs an index check.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::Reference
   HybridVector<Type,N,TF>::operator[]( size_t index )
{
   BLAZE_USER_ASSERT( index < size_, "Invalid vector access index" );
   return v_[index];
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Subscript operator for the direct access to the vector elements.
//
// \param index Access index. The index has to be in the range \f$[0..N-1]\f$.
// \return Reference-to-const to the accessed value.
//
// In case BLAZE_USER_ASSERT() is active, this operator performs an index check.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::ConstReference
   HybridVector<Type,N,TF>::operator[]( size_t index ) const
{
   BLAZE_USER_ASSERT( index < size_, "Invalid vector access index" );
   return v_[index];
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Low-level data access to the vector elements.
//
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage of the hybrid vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::Pointer HybridVector<Type,N,TF>::data()
{
   return v_;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Low-level data access to the vector elements.
//
// \return Pointer to the internal element storage.
//
// This function returns a pointer to the internal storage of the hybrid vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::ConstPointer HybridVector<Type,N,TF>::data() const
{
   return v_;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator to the first element of the hybrid vector.
//
// \return Iterator to the first element of the hybrid vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::Iterator HybridVector<Type,N,TF>::begin()
{
   return Iterator( v_ );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator to the first element of the hybrid vector.
//
// \return Iterator to the first element of the hybrid vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::ConstIterator HybridVector<Type,N,TF>::begin() const
{
   return ConstIterator( v_ );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator to the first element of the hybrid vector.
//
// \return Iterator to the first element of the hybrid vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::ConstIterator HybridVector<Type,N,TF>::cbegin() const
{
   return ConstIterator( v_ );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator just past the last element of the hybrid vector.
//
// \return Iterator just past the last element of the hybrid vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::Iterator HybridVector<Type,N,TF>::end()
{
   BLAZE_INTERNAL_ASSERT( size_ <= N, "Invalid size detected" );
   return Iterator( v_ + size_ );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator just past the last element of the hybrid vector.
//
// \return Iterator just past the last element of the hybrid vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::ConstIterator HybridVector<Type,N,TF>::end() const
{
   BLAZE_INTERNAL_ASSERT( size_ <= N, "Invalid size detected" );
   return ConstIterator( v_ + size_ );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns an iterator just past the last element of the hybrid vector.
//
// \return Iterator just past the last element of the hybrid vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::ConstIterator HybridVector<Type,N,TF>::cend() const
{
   BLAZE_INTERNAL_ASSERT( size_ <= N, "Invalid size detected" );
   return ConstIterator( v_ + size_ );
}
//*************************************************************************************************




//=================================================================================================
//
//  ASSIGNMENT OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Array assignment to all vector elements.
//
// \param array M-dimensional array for the assignment.
// \return Reference to the assigned vector.
//
// This assignment operator offers the option to directly set all elements of the vector:

   \code
   const double init[2] = { 1.0, 2.0 };
   blaze::HybridVector<double,4> v;
   v = init;
   \endcode

// The vector is sized according to the size of the array and assigned the values of the given
// array. This assignment operator only works for arrays with a size smaller-or-equal than the
// maximum number of elements of the hybrid vector. (i.e. M<= N). The attempt to use a larger
// array will result in a compile time error.
*/
template< typename Type   // Data type of the vector
        , size_t N        // Number of elements
        , bool TF >       // Transpose flag
template< typename Other  // Data type of the initialization array
        , size_t M >      // Number of elements of the initialization array
inline HybridVector<Type,N,TF>& HybridVector<Type,N,TF>::operator=( const Other (&array)[M] )
{
   BLAZE_STATIC_ASSERT( M <= N );

   resize( M );

   for( size_t i=0UL; i<M; ++i )
      v_[i] = array[i];

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Homogenous assignment to all vector elements.
//
// \param rhs Scalar value to be assigned to all vector elements.
// \return Reference to the assigned vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline HybridVector<Type,N,TF>& HybridVector<Type,N,TF>::operator=( const Type& rhs )
{
   BLAZE_INTERNAL_ASSERT( size_ <= N, "Invalid size detected" );

   for( size_t i=0UL; i<size_; ++i )
      v_[i] = rhs;
   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Copy assignment operator for HybridVector.
//
// \param rhs Vector to be copied.
// \return Reference to the assigned vector.
//
// Explicit definition of a copy assignment operator for performance reasons.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline HybridVector<Type,N,TF>& HybridVector<Type,N,TF>::operator=( const HybridVector& rhs )
{
   using blaze::assign;

   BLAZE_INTERNAL_ASSERT( size_ <= N, "Invalid size detected" );

   resize( rhs.size() );
   assign( *this, ~rhs );

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Assignment operator for different vectors.
//
// \param rhs Vector to be copied.
// \return Reference to the assigned vector.
// \exception std::invalid_argument Invalid assignment to hybrid vector.
//
// This constructor initializes the vector as a copy of the given vector. In case the size
// of the given vector is larger than N, a \a std::invalid_argument exception is thrown.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side vector
inline HybridVector<Type,N,TF>& HybridVector<Type,N,TF>::operator=( const Vector<VT,TF>& rhs )
{
   using blaze::assign;

   if( (~rhs).size() > N )
      throw std::invalid_argument( "Invalid assignment to hybrid vector" );

   if( (~rhs).canAlias( this ) ) {
      HybridVector tmp( ~rhs );
      swap( tmp );
   }
   else {
      resize( (~rhs).size() );
      if( IsSparseVector<VT>::value )
         reset();
      assign( *this, ~rhs );
   }

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Addition assignment operator for the addition of a vector (\f$ \vec{a}+=\vec{b} \f$).
//
// \param rhs The right-hand side vector to be added to the vector.
// \return Reference to the vector.
// \exception std::invalid_argument Vector sizes do not match.
//
// In case the current sizes of the two vectors don't match, a \a std::invalid_argument exception
// is thrown.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side vector
inline HybridVector<Type,N,TF>& HybridVector<Type,N,TF>::operator+=( const Vector<VT,TF>& rhs )
{
   using blaze::addAssign;

   if( (~rhs).size() != size_ )
      throw std::invalid_argument( "Vector sizes do not match" );

   if( (~rhs).canAlias( this ) ) {
      typename VT::ResultType tmp( ~rhs );
      addAssign( *this, tmp );
   }
   else {
      addAssign( *this, ~rhs );
   }

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Subtraction assignment operator for the subtraction of a vector (\f$ \vec{a}-=\vec{b} \f$).
//
// \param rhs The right-hand side vector to be subtracted from the vector.
// \return Reference to the vector.
// \exception std::invalid_argument Vector sizes do not match.
//
// In case the current sizes of the two vectors don't match, a \a std::invalid_argument exception
// is thrown.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side vector
inline HybridVector<Type,N,TF>& HybridVector<Type,N,TF>::operator-=( const Vector<VT,TF>& rhs )
{
   using blaze::subAssign;

   if( (~rhs).size() != size_ )
      throw std::invalid_argument( "Vector sizes do not match" );

   if( (~rhs).canAlias( this ) ) {
      typename VT::ResultType tmp( ~rhs );
      subAssign( *this, tmp );
   }
   else {
      subAssign( *this, ~rhs );
   }

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Multiplication assignment operator for the multiplication of a vector
//        (\f$ \vec{a}*=\vec{b} \f$).
//
// \param rhs The right-hand side vector to be multiplied with the vector.
// \return Reference to the vector.
// \exception std::invalid_argument Vector sizes do not match.
//
// In case the current sizes of the two vectors don't match, a \a std::invalid_argument exception
// is thrown.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side vector
inline HybridVector<Type,N,TF>& HybridVector<Type,N,TF>::operator*=( const Vector<VT,TF>& rhs )
{
   using blaze::multAssign;

   if( (~rhs).size() != size_ )
      throw std::invalid_argument( "Vector sizes do not match" );

   if( (~rhs).canAlias( this ) || IsSparseVector<VT>::value ) {
      HybridVector tmp( *this * (~rhs) );
      this->operator=( tmp );
   }
   else {
      multAssign( *this, ~rhs );
   }

   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Multiplication assignment operator for the multiplication between a vector and
//        a scalar value (\f$ \vec{a}*=s \f$).
//
// \param rhs The right-hand side scalar value for the multiplication.
// \return Reference to the vector.
*/
template< typename Type     // Data type of the vector
        , size_t N          // Number of elements
        , bool TF >         // Transpose flag
template< typename Other >  // Data type of the right-hand side scalar
inline typename EnableIf< IsNumeric<Other>, HybridVector<Type,N,TF> >::Type&
   HybridVector<Type,N,TF>::operator*=( Other rhs )
{
   using blaze::assign;

   assign( *this, (*this) * rhs );
   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Division assignment operator for the division of a vector by a scalar value
//        (\f$ \vec{a}/=s \f$).
//
// \param rhs The right-hand side scalar value for the division.
// \return Reference to the vector.
//
// \b Note: A division by zero is only checked by an user assert.
*/
template< typename Type     // Data type of the vector
        , size_t N          // Number of elements
        , bool TF >         // Transpose flag
template< typename Other >  // Data type of the right-hand side scalar
inline typename EnableIf< IsNumeric<Other>, HybridVector<Type,N,TF> >::Type&
   HybridVector<Type,N,TF>::operator/=( Other rhs )
{
   using blaze::assign;

   BLAZE_USER_ASSERT( rhs != Other(0), "Division by zero detected" );

   assign( *this, (*this) / rhs );
   return *this;
}
//*************************************************************************************************




//=================================================================================================
//
//  UTILITY FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Returns the current size/dimension of the vector.
//
// \return The size of the vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline size_t HybridVector<Type,N,TF>::size() const
{
   return size_;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the maximum capacity of the vector.
//
// \return The capacity of the vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline size_t HybridVector<Type,N,TF>::capacity() const
{
   return NN;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns the number of non-zero elements in the vector.
//
// \return The number of non-zero elements in the vector.
//
// Note that the number of non-zero elements is always less than or equal to the current size
// of the vector.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline size_t HybridVector<Type,N,TF>::nonZeros() const
{
   size_t nonzeros( 0 );

   for( size_t i=0UL; i<size_; ++i ) {
      if( !isDefault( v_[i] ) )
         ++nonzeros;
   }

   return nonzeros;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Reset to the default initial values.
//
// \return void
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::reset()
{
   using blaze::clear;
   for( size_t i=0UL; i<size_; ++i )
      clear( v_[i] );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Clearing the vector.
//
// \return void
//
// After the clear() function, the size of the vector is 0.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::clear()
{
   resize( 0UL );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Changing the size of the vector.
//
// \param n The new size of the vector.
// \param preserve \a true if the old values of the vector should be preserved, \a false if not.
// \return void
// \exception std::invalid_argument Invalid size for hybrid vector.
//
// This function resizes the vector to the given size \a n. In case the given size \a n is larger
// than the maximum number of vector elements (i.e. if n > N) a \a std::invalid_argument exception
// is thrown. Note that this function may invalidate all existing views (subvectors, ...) on the
// vector if it used to shrink the vector. Additionally, during this operation all vector elements
// are potentially changed. In order to preserve the old vector values, the \a preserve flag can be
// set to \a true.
//
// Note that in case the size of the vector is increased new vector elements are not initialized!
// This is illustrated by the following example, which demonstrates the resizing of a vector of
// size 2 to a vector of size 4. The new, uninitialized elements are marked with \a x:

                              \f[
                              \left(\begin{array}{*{2}{c}}
                              1 & 2 \\
                              \end{array}\right)

                              \Longrightarrow

                              \left(\begin{array}{*{4}{c}}
                              1 & 2 & x & x \\
                              \end{array}\right)
                              \f]
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::resize( size_t n, bool preserve )
{
   UNUSED_PARAMETER( preserve );

   if( n > N )
      throw std::invalid_argument( "Invalid size for hybrid vector" );

   if( IsVectorizable<Type>::value && n < size_ ) {
      for( size_t i=n; i<size_; ++i )
         v_[i] = Type();
   }

   size_ = n;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Extending the size of the vector.
//
// \param n Number of additional vector elements.
// \param preserve \a true if the old values of the vector should be preserved, \a false if not.
// \return void
//
// This function increases the vector size by \a n elements. In case the resulting size
// of the vector is larger than the maximum number of vector elements (i.e. if n > N) a
// \a std::invalid_argument exception is thrown. During this operation, all vector elements
// are potentially changed. In order to preserve the old vector values, the \a preserve flag
// can be set to \a true.\n
// Note that new vector elements are not initialized!
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::extend( size_t n, bool preserve )
{
   UNUSED_PARAMETER( preserve );
   resize( size_+n );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Scaling of the vector by the scalar value \a scalar (\f$ \vec{a}*=s \f$).
//
// \param scalar The scalar value for the vector scaling.
// \return Reference to the vector.
*/
template< typename Type     // Data type of the vector
        , size_t N          // Number of elements
        , bool TF >         // Transpose flag
template< typename Other >  // Data type of the scalar value
inline HybridVector<Type,N,TF>& HybridVector<Type,N,TF>::scale( const Other& scalar )
{
   for( size_t i=0; i<size_; ++i )
      v_[i] *= scalar;
   return *this;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Swapping the contents of two hybrid vectors.
//
// \param v The vector to be swapped.
// \return void
// \exception no-throw guarantee.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::swap( HybridVector& v ) /* throw() */
{
   using std::swap;

   const size_t maxsize( max( size_, v.size_ ) );
   for( size_t i=0UL; i<maxsize; ++i )
      swap( v_[i], v.v_[i] );
   swap( size_, v.size_ );
}
//*************************************************************************************************




//=================================================================================================
//
//  MEMORY FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Class specific implementation of operator new.
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the StaticVector class template.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void* HybridVector<Type,N,TF>::operator new( std::size_t size )
{
   UNUSED_PARAMETER( size );

   BLAZE_INTERNAL_ASSERT( size == sizeof( HybridVector ), "Invalid number of bytes detected" );

   return allocate<HybridVector>( 1UL );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of operator new[].
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridVector class template.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void* HybridVector<Type,N,TF>::operator new[]( std::size_t size )
{
   BLAZE_INTERNAL_ASSERT( size >= sizeof( HybridVector )       , "Invalid number of bytes detected" );
   BLAZE_INTERNAL_ASSERT( size %  sizeof( HybridVector ) == 0UL, "Invalid number of bytes detected" );

   return allocate<HybridVector>( size/sizeof(HybridVector) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of the no-throw operator new.
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridVector class template.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void* HybridVector<Type,N,TF>::operator new( std::size_t size, const std::nothrow_t& )
{
   UNUSED_PARAMETER( size );

   BLAZE_INTERNAL_ASSERT( size == sizeof( HybridVector ), "Invalid number of bytes detected" );

   return allocate<HybridVector>( 1UL );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of the no-throw operator new[].
//
// \param size The total number of bytes to be allocated.
// \return Pointer to the newly allocated memory.
// \exception std::bad_alloc Allocation failed.
//
// This class-specific implementation of operator new provides the functionality to allocate
// dynamic memory based on the alignment restrictions of the HybridVector class template.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void* HybridVector<Type,N,TF>::operator new[]( std::size_t size, const std::nothrow_t& )
{
   BLAZE_INTERNAL_ASSERT( size >= sizeof( HybridVector )       , "Invalid number of bytes detected" );
   BLAZE_INTERNAL_ASSERT( size %  sizeof( HybridVector ) == 0UL, "Invalid number of bytes detected" );

   return allocate<HybridVector>( size/sizeof(HybridVector) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of operator delete.
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::operator delete( void* ptr )
{
   deallocate( static_cast<HybridVector*>( ptr ) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of operator delete[].
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::operator delete[]( void* ptr )
{
   deallocate( static_cast<HybridVector*>( ptr ) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of no-throw operator delete.
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::operator delete( void* ptr, const std::nothrow_t& )
{
   deallocate( static_cast<HybridVector*>( ptr ) );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Class specific implementation of no-throw operator delete[].
//
// \param ptr The memory to be deallocated.
// \return void
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::operator delete[]( void* ptr, const std::nothrow_t& )
{
   deallocate( static_cast<HybridVector*>( ptr ) );
}
//*************************************************************************************************




//=================================================================================================
//
//  EXPRESSION TEMPLATE EVALUATION FUNCTIONS
//
//=================================================================================================

//*************************************************************************************************
/*!\brief Returns whether the vector can alias with the given address \a alias.
//
// \param alias The alias to be checked.
// \return \a true in case the alias corresponds to this vector, \a false if not.
//
// This function returns whether the given address can alias with the vector. In contrast
// to the isAliased() function this function is allowed to use compile time expressions
// to optimize the evaluation.
*/
template< typename Type     // Data type of the vector
        , size_t N          // Number of elements
        , bool TF >         // Transpose flag
template< typename Other >  // Data type of the foreign expression
inline bool HybridVector<Type,N,TF>::canAlias( const Other* alias ) const
{
   return static_cast<const void*>( this ) == static_cast<const void*>( alias );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns whether the vector is aliased with the given address \a alias.
//
// \param alias The alias to be checked.
// \return \a true in case the alias corresponds to this vector, \a false if not.
//
// This function returns whether the given address is aliased with the vector. In contrast
// to the canAlias() function this function is not allowed to use compile time expressions
// to optimize the evaluation.
*/
template< typename Type     // Data type of the vector
        , size_t N          // Number of elements
        , bool TF >         // Transpose flag
template< typename Other >  // Data type of the foreign expression
inline bool HybridVector<Type,N,TF>::isAliased( const Other* alias ) const
{
   return static_cast<const void*>( this ) == static_cast<const void*>( alias );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns whether the vector is properly aligned in memory.
//
// \return \a true in case the vector is aligned, \a false if not.
//
// This function returns whether the vector is guaranteed to be properly aligned in memory, i.e.
// whether the beginning and the end of the vector are guaranteed to conform to the alignment
// restrictions of the element type \a Type.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline bool HybridVector<Type,N,TF>::isAligned() const
{
   return true;
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Aligned load of an intrinsic element of the vector.
//
// \param index Access index. The index must be smaller than the number of vector elements.
// \return The loaded intrinsic element.
//
// This function performs an aligned load of a specific intrinsic element of the dense vector.
// The index must be smaller than the number of vector elements and it must be a multiple of
// the number of values inside the intrinsic element. This function must \b NOT be called
// explicitly! It is used internally for the performance optimized evaluation of expression
// templates. Calling this function explicitly might result in erroneous results and/or in
// compilation errors.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::IntrinsicType
   HybridVector<Type,N,TF>::load( size_t index ) const
{
   using blaze::load;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( index            <  size_, "Invalid vector access index" );
   BLAZE_INTERNAL_ASSERT( index + IT::size <= NN   , "Invalid vector access index" );
   BLAZE_INTERNAL_ASSERT( index % IT::size == 0UL  , "Invalid vector access index" );

   return load( &v_[index] );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Unaligned load of an intrinsic element of the vector.
//
// \param index Access index. The index must be smaller than the number of vector elements.
// \return The loaded intrinsic element.
//
// This function performs an unaligned load of a specific intrinsic element of the dense vector.
// The index must be smaller than the number of vector elements and it must be a multiple of
// the number of values inside the intrinsic element. This function must \b NOT be called
// explicitly! It is used internally for the performance optimized evaluation of expression
// templates. Calling this function explicitly might result in erroneous results and/or in
// compilation errors.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline typename HybridVector<Type,N,TF>::IntrinsicType
   HybridVector<Type,N,TF>::loadu( size_t index ) const
{
   using blaze::loadu;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( index            <  size_, "Invalid vector access index" );
   BLAZE_INTERNAL_ASSERT( index + IT::size <= NN   , "Invalid vector access index" );

   return loadu( &v_[index] );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Aligned store of an intrinsic element of the vector.
//
// \param index Access index. The index must be smaller than the number of vector elements.
// \param value The intrinsic element to be stored.
// \return void
//
// This function performs an aligned store of a specific intrinsic element of the dense vector.
// The index must be smaller than the number of vector elements and it must be a multiple of
// the number of values inside the intrinsic element. This function must \b NOT be called
// explicitly! It is used internally for the performance optimized evaluation of expression
// templates. Calling this function explicitly might result in erroneous results and/or in
// compilation errors.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::store( size_t index, const IntrinsicType& value )
{
   using blaze::store;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( index            <  size_, "Invalid vector access index" );
   BLAZE_INTERNAL_ASSERT( index + IT::size <= NN   , "Invalid vector access index" );
   BLAZE_INTERNAL_ASSERT( index % IT::size == 0UL  , "Invalid vector access index" );

   store( &v_[index], value );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Unaligned store of an intrinsic element of the vector.
//
// \param index Access index. The index must be smaller than the number of vector elements.
// \param value The intrinsic element to be stored.
// \return void
//
// This function performs an unaligned store of a specific intrinsic element of the dense vector.
// The index must be smaller than the number of vector elements and it must be a multiple of the
// number of values inside the intrinsic element. This function must \b NOT be called explicitly!
// It is used internally for the performance optimized evaluation of expression templates.
// Calling this function explicitly might result in erroneous results and/or in compilation
// errors.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::storeu( size_t index, const IntrinsicType& value )
{
   using blaze::storeu;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( index            <  size_, "Invalid vector access index" );
   BLAZE_INTERNAL_ASSERT( index + IT::size <= NN   , "Invalid vector access index" );

   storeu( &v_[index], value );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Aligned, non-temporal store of an intrinsic element of the vector.
//
// \param index Access index. The index must be smaller than the number of vector elements.
// \param value The intrinsic element to be stored.
// \return void
//
// This function performs an aligned, non-temporal store of a specific intrinsic element of
// the dense vector. The index must be smaller than the number of vector elements and it must
// be a multiple of the number of values inside the intrinsic element. This function must
// \b NOT be called explicitly! It is used internally for the performance optimized evaluation
// of expression templates. Calling this function explicitly might result in erroneous results
// and/or in compilation errors.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void HybridVector<Type,N,TF>::stream( size_t index, const IntrinsicType& value )
{
   using blaze::stream;

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   BLAZE_INTERNAL_ASSERT( index            <  size_, "Invalid vector access index" );
   BLAZE_INTERNAL_ASSERT( index + IT::size <= NN   , "Invalid vector access index" );
   BLAZE_INTERNAL_ASSERT( index % IT::size == 0UL  , "Invalid vector access index" );

   stream( &v_[index], value );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the assignment of a dense vector.
//
// \param rhs The right-hand side dense vector to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side dense vector
inline typename DisableIf< typename HybridVector<Type,N,TF>::BLAZE_TEMPLATE VectorizedAssign<VT> >::Type
   HybridVector<Type,N,TF>::assign( const DenseVector<VT,TF>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   for( size_t i=0UL; i<size_; ++i )
      v_[i] = (~rhs)[i];
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Intrinsic optimized implementation of the assignment of a dense vector.
//
// \param rhs The right-hand side dense vector to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side dense vector
inline typename EnableIf< typename HybridVector<Type,N,TF>::BLAZE_TEMPLATE VectorizedAssign<VT> >::Type
   HybridVector<Type,N,TF>::assign( const DenseVector<VT,TF>& rhs )
{
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t i=0UL; i<size_; i+=IT::size ) {
      store( v_+i, (~rhs).load(i) );
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the assignment of a sparse vector.
//
// \param rhs The right-hand side sparse vector to be assigned.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side sparse vector
inline void HybridVector<Type,N,TF>::assign( const SparseVector<VT,TF>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   for( typename VT::ConstIterator element=(~rhs).begin(); element!=(~rhs).end(); ++element )
      v_[element->index()] = element->value();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the addition assignment of a dense vector.
//
// \param rhs The right-hand side dense vector to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side dense vector
inline typename DisableIf< typename HybridVector<Type,N,TF>::BLAZE_TEMPLATE VectorizedAddAssign<VT> >::Type
   HybridVector<Type,N,TF>::addAssign( const DenseVector<VT,TF>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   for( size_t i=0UL; i<size_; ++i )
      v_[i] += (~rhs)[i];
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Intrinsic optimized implementation of the addition assignment of a dense vector.
//
// \param rhs The right-hand side dense vector to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side dense vector
inline typename EnableIf< typename HybridVector<Type,N,TF>::BLAZE_TEMPLATE VectorizedAddAssign<VT> >::Type
   HybridVector<Type,N,TF>::addAssign( const DenseVector<VT,TF>& rhs )
{
   using blaze::load;
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t i=0UL; i<size_; i+=IT::size ) {
      store( v_+i, load( v_+i ) + (~rhs).load(i) );
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the addition assignment of a sparse vector.
//
// \param rhs The right-hand side sparse vector to be added.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side sparse vector
inline void HybridVector<Type,N,TF>::addAssign( const SparseVector<VT,TF>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   for( typename VT::ConstIterator element=(~rhs).begin(); element!=(~rhs).end(); ++element )
      v_[element->index()] += element->value();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the subtraction assignment of a dense vector.
//
// \param rhs The right-hand side dense vector to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side dense vector
inline typename DisableIf< typename HybridVector<Type,N,TF>::BLAZE_TEMPLATE VectorizedSubAssign<VT> >::Type
   HybridVector<Type,N,TF>::subAssign( const DenseVector<VT,TF>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   for( size_t i=0UL; i<size_; ++i )
      v_[i] -= (~rhs)[i];
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Intrinsic optimized implementation of the subtraction assignment of a dense vector.
//
// \param rhs The right-hand side dense vector to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side dense vector
inline typename EnableIf< typename HybridVector<Type,N,TF>::BLAZE_TEMPLATE VectorizedSubAssign<VT> >::Type
   HybridVector<Type,N,TF>::subAssign( const DenseVector<VT,TF>& rhs )
{
   using blaze::load;
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t i=0UL; i<size_; i+=IT::size ) {
      store( v_+i, load( v_+i ) - (~rhs).load(i) );
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the subtraction assignment of a sparse vector.
//
// \param rhs The right-hand side sparse vector to be subtracted.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side sparse vector
inline void HybridVector<Type,N,TF>::subAssign( const SparseVector<VT,TF>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   for( typename VT::ConstIterator element=(~rhs).begin(); element!=(~rhs).end(); ++element )
      v_[element->index()] -= element->value();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the multiplication assignment of a dense vector.
//
// \param rhs The right-hand side dense vector to be multiplied.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side dense vector
inline typename DisableIf< typename HybridVector<Type,N,TF>::BLAZE_TEMPLATE VectorizedMultAssign<VT> >::Type
   HybridVector<Type,N,TF>::multAssign( const DenseVector<VT,TF>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   for( size_t i=0UL; i<size_; ++i )
      v_[i] *= (~rhs)[i];
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Intrinsic optimized implementation of the multiplication assignment of a dense vector.
//
// \param rhs The right-hand side dense vector to be multiplied.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side dense vector
inline typename EnableIf< typename HybridVector<Type,N,TF>::BLAZE_TEMPLATE VectorizedMultAssign<VT> >::Type
   HybridVector<Type,N,TF>::multAssign( const DenseVector<VT,TF>& rhs )
{
   using blaze::load;
   using blaze::store;

   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE( Type );

   for( size_t i=0UL; i<size_; i+=IT::size ) {
      store( v_+i, load( v_+i ) * (~rhs).load(i) );
   }
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Default implementation of the multiplication assignment of a sparse vector.
//
// \param rhs The right-hand side sparse vector to be multiplied.
// \return void
//
// This function must \b NOT be called explicitly! It is used internally for the performance
// optimized evaluation of expression templates. Calling this function explicitly might result
// in erroneous results and/or in compilation errors. Instead of using this function use the
// assignment operator.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
template< typename VT >  // Type of the right-hand side sparse vector
inline void HybridVector<Type,N,TF>::multAssign( const SparseVector<VT,TF>& rhs )
{
   BLAZE_INTERNAL_ASSERT( (~rhs).size() == size_, "Invalid vector sizes" );

   const HybridVector tmp( serial( *this ) );

   reset();

   for( typename VT::ConstIterator element=(~rhs).begin(); element!=(~rhs).end(); ++element )
      v_[element->index()] = tmp[element->index()] * element->value();
}
//*************************************************************************************************








//=================================================================================================
//
//  UNDEFINED CLASS TEMPLATE SPECIALIZATION
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
/*!\brief Specialization of HybridVector for 0 elements.
// \ingroup hybrid_vector
//
// This specialization of the HybridVector class template is left undefined and therefore
// prevents the instantiation for 0 elements.
*/
template< typename Type  // Data type of the vector
        , bool TF >      // Transpose flag
class HybridVector<Type,0UL,TF>;
/*! \endcond */
//*************************************************************************************************








//=================================================================================================
//
//  HYBRIDVECTOR OPERATORS
//
//=================================================================================================

//*************************************************************************************************
/*!\name HybridVector operators */
//@{
template< typename Type, size_t N, bool TF >
inline void reset( HybridVector<Type,N,TF>& v );

template< typename Type, size_t N, bool TF >
inline void clear( HybridVector<Type,N,TF>& v );

template< typename Type, size_t N, bool TF >
inline bool isDefault( const HybridVector<Type,N,TF>& v );

template< typename Type, size_t N, bool TF >
inline void swap( HybridVector<Type,N,TF>& a, HybridVector<Type,N,TF>& b ) /* throw() */;

template< typename Type, size_t N, bool TF >
inline void move( HybridVector<Type,N,TF>& dst, HybridVector<Type,N,TF>& src ) /* throw() */;
//@}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Resetting the given hybrid vector.
// \ingroup hybrid_vector
//
// \param v The vector to be resetted.
// \return void
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void reset( HybridVector<Type,N,TF>& v )
{
   v.reset();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Clearing the given hybrid vector.
// \ingroup hybrid_vector
//
// \param v The vector to be cleared.
// \return void
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void clear( HybridVector<Type,N,TF>& v )
{
   v.clear();
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Returns whether the given hybrid vector is in default state.
// \ingroup hybrid_vector
//
// \param v The hybrid vector to be tested for its default state.
// \return \a true in case the given vector's size is zero, \a false otherwise.
//
// This function checks whether the hybrid vector is in default (constructed) state, i.e. if
// it's size is 0. In case it is in default state, the function returns \a true, else it will
// return \a false. The following example demonstrates the use of the \a isDefault() function:

   \code
   blaze::HybridVector<double,3> a;
   // ... Resizing and initialization
   if( isDefault( a ) ) { ... }
   \endcode
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline bool isDefault( const HybridVector<Type,N,TF>& v )
{
   return ( v.size() == 0UL );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Swapping the contents of two hybrid vectors.
// \ingroup hybrid_vector
//
// \param a The first vector to be swapped.
// \param b The second vector to be swapped.
// \return void
// \exception no-throw guarantee.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void swap( HybridVector<Type,N,TF>& a, HybridVector<Type,N,TF>& b ) /* throw() */
{
   a.swap( b );
}
//*************************************************************************************************


//*************************************************************************************************
/*!\brief Moving the contents of one hybrid vector to another.
// \ingroup hybrid_vector
//
// \param dst The destination vector.
// \param src The source vector.
// \return void
// \exception no-throw guarantee.
*/
template< typename Type  // Data type of the vector
        , size_t N       // Number of elements
        , bool TF >      // Transpose flag
inline void move( HybridVector<Type,N,TF>& dst, HybridVector<Type,N,TF>& src ) /* throw() */
{
   dst = src;
}
//*************************************************************************************************




//=================================================================================================
//
//  ISRESIZABLE SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T, size_t N, bool TF >
struct IsResizable< HybridVector<T,N,TF> > : public TrueType
{
   enum { value = 1 };
   typedef TrueType  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  ADDTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M, bool TF, typename T2, size_t N >
struct AddTrait< HybridVector<T1,M,TF>, StaticVector<T2,N,TF> >
{
   typedef StaticVector< typename AddTrait<T1,T2>::Type, N, TF >  Type;
};

template< typename T1, size_t M, bool TF, typename T2, size_t N >
struct AddTrait< StaticVector<T1,M,TF>, HybridVector<T2,N,TF> >
{
   typedef StaticVector< typename AddTrait<T1,T2>::Type, M, TF >  Type;
};

template< typename T1, size_t M, bool TF, typename T2, size_t N >
struct AddTrait< HybridVector<T1,M,TF>, HybridVector<T2,N,TF> >
{
   typedef HybridVector< typename AddTrait<T1,T2>::Type, ( M < N )?( M ):( N ), TF >  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  SUBTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t M, bool TF, typename T2, size_t N >
struct SubTrait< HybridVector<T1,M,TF>, StaticVector<T2,N,TF> >
{
   typedef StaticVector< typename SubTrait<T1,T2>::Type, N, TF >  Type;
};

template< typename T1, size_t M, bool TF, typename T2, size_t N >
struct SubTrait< StaticVector<T1,M,TF>, HybridVector<T2,N,TF> >
{
   typedef StaticVector< typename SubTrait<T1,T2>::Type, M, TF >  Type;
};

template< typename T1, size_t M, bool TF, typename T2, size_t N >
struct SubTrait< HybridVector<T1,M,TF>, HybridVector<T2,N,TF> >
{
   typedef HybridVector< typename SubTrait<T1,T2>::Type, ( M < N )?( M ):( N ), TF >  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  MULTTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t N, bool TF, typename T2 >
struct MultTrait< HybridVector<T1,N,TF>, T2 >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, N, TF >  Type;
   BLAZE_CONSTRAINT_MUST_BE_NUMERIC_TYPE( T2 );
};

template< typename T1, typename T2, size_t N, bool TF >
struct MultTrait< T1, HybridVector<T2,N,TF> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, N, TF >  Type;
   BLAZE_CONSTRAINT_MUST_BE_NUMERIC_TYPE( T1 );
};

template< typename T1, size_t M, bool TF, typename T2, size_t N >
struct MultTrait< HybridVector<T1,M,TF>, StaticVector<T2,N,TF> >
{
   typedef StaticVector< typename MultTrait<T1,T2>::Type, N, TF >  Type;
};

template< typename T1, size_t M, typename T2, size_t N >
struct MultTrait< HybridVector<T1,M,false>, StaticVector<T2,N,true> >
{
   typedef DynamicMatrix< typename MultTrait<T1,T2>::Type, false >  Type;
};

template< typename T1, size_t M, typename T2, size_t N >
struct MultTrait< HybridVector<T1,M,true>, StaticVector<T2,N,false> >
{
   typedef typename MultTrait<T1,T2>::Type  Type;
};

template< typename T1, size_t M, bool TF, typename T2, size_t N >
struct MultTrait< StaticVector<T1,M,TF>, HybridVector<T2,N,TF> >
{
   typedef StaticVector< typename MultTrait<T1,T2>::Type, M, TF >  Type;
};

template< typename T1, size_t M, typename T2, size_t N >
struct MultTrait< StaticVector<T1,M,false>, HybridVector<T2,N,true> >
{
   typedef DynamicMatrix< typename MultTrait<T1,T2>::Type, false >  Type;
};

template< typename T1, size_t M, typename T2, size_t N >
struct MultTrait< StaticVector<T1,M,true>, HybridVector<T2,N,false> >
{
   typedef typename MultTrait<T1,T2>::Type  Type;
};

template< typename T1, size_t M, bool TF, typename T2, size_t N >
struct MultTrait< HybridVector<T1,M,TF>, HybridVector<T2,N,TF> >
{
   typedef HybridVector< typename MultTrait<T1,T2>::Type, ( M < N )?( M ):( N ), TF >  Type;
};

template< typename T1, size_t M, typename T2, size_t N >
struct MultTrait< HybridVector<T1,M,false>, HybridVector<T2,N,true> >
{
   typedef DynamicMatrix< typename MultTrait<T1,T2>::Type, false >  Type;
};

template< typename T1, size_t M, typename T2, size_t N >
struct MultTrait< HybridVector<T1,M,true>, HybridVector<T2,N,false> >
{
   typedef typename MultTrait<T1,T2>::Type  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  CROSSTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t N, typename T2 >
struct CrossTrait< HybridVector<T1,N,false>, StaticVector<T2,3UL,false> >
{
 private:
   typedef typename MultTrait<T1,T2>::Type  T;

 public:
   typedef StaticVector< typename SubTrait<T,T>::Type, 3UL, false >  Type;
};

template< typename T1, typename T2, size_t N >
struct CrossTrait< StaticVector<T1,3UL,false>, HybridVector<T2,N,false> >
{
 private:
   typedef typename MultTrait<T1,T2>::Type  T;

 public:
   typedef StaticVector< typename SubTrait<T,T>::Type, 3UL, false >  Type;
};

template< typename T1, size_t M, typename T2, size_t N >
struct CrossTrait< HybridVector<T1,M,false>, HybridVector<T2,N,false> >
{
 private:
   typedef typename MultTrait<T1,T2>::Type  T;

 public:
   typedef StaticVector< typename SubTrait<T,T>::Type, 3UL, false >  Type;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  DIVTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t N, bool TF, typename T2 >
struct DivTrait< HybridVector<T1,N,TF>, T2 >
{
   typedef HybridVector< typename DivTrait<T1,T2>::Type, N, TF >  Type;
   BLAZE_CONSTRAINT_MUST_BE_NUMERIC_TYPE( T2 );
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  MATHTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t N, bool TF, typename T2 >
struct MathTrait< HybridVector<T1,N,TF>, HybridVector<T2,N,TF> >
{
   typedef StaticVector< typename MathTrait<T1,T2>::HighType, N, TF >  HighType;
   typedef StaticVector< typename MathTrait<T1,T2>::LowType , N, TF >  LowType;
};
/*! \endcond */
//*************************************************************************************************




//=================================================================================================
//
//  SUBVECTORTRAIT SPECIALIZATIONS
//
//=================================================================================================

//*************************************************************************************************
/*! \cond BLAZE_INTERNAL */
template< typename T1, size_t N, bool TF >
struct SubvectorTrait< HybridVector<T1,N,TF> >
{
   typedef HybridVector<T1,N,TF>  Type;
};
/*! \endcond */
//*************************************************************************************************

} // namespace blaze

#endif
