// serialization_benchmark.cpp
#include <iostream>
#include <iomanip>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/chrono.hpp>
#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/static_assert.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/limits.hpp>

#include "dvidmsg.pb.h"
#include "timer.h"

using std::size_t;
using boost::int8_t;
using boost::uint8_t;
using boost::int16_t;
using boost::uint16_t;
using boost::int32_t;
using boost::uint32_t;
using boost::int64_t;
using boost::uint64_t;

using namespace ::dvidmsg;

typedef boost::shared_ptr<Array> ArrayPtr ;

// ******************************************************************
// @details
// Return random number uniformly distributed within the range of
//  the specified numeric type.
// ******************************************************************
template <typename T_>
T_ get_uniform_random()
{
    static const T_ lowest = boost::numeric::bounds<T_>::lowest() ;
    static const T_ highest = boost::numeric::bounds<T_>::highest() ;

    static boost::random::uniform_int_distribution<T_> dist(lowest, highest);
    static boost::random::mt19937 gen;
    return dist(gen) ;
}

// ******************************************************************
// @details
// Populate a vector with uniformly distributed random data
// ******************************************************************
template <typename T_>
void populate_random( std::vector<T_> & vec, size_t len )
{
    vec.resize(len);
    BOOST_FOREACH( T_ & x, vec )
    {
        x = get_uniform_random<uint32_t>() ;
    }
}

// ******************************************************************
// @details
// This function is specialized for each numeric type to return the
//  type name string used in the DVID messaging spec.
// ******************************************************************
template <typename T_> std::string get_type_name()
{ BOOST_STATIC_ASSERT( sizeof(T_) == 0 && "Type name string not defined for this type!" ) ; }
template <> std::string get_type_name<uint8_t>()    { return "u8" ; }
template <> std::string get_type_name<int8_t>()     { return "i8" ; }
template <> std::string get_type_name<uint16_t>()   { return "u16" ; }
template <> std::string get_type_name<int16_t>()    { return "i16" ; }
template <> std::string get_type_name<uint32_t>()   { return "u32" ; }
template <> std::string get_type_name<int32_t>()    { return "i32" ; }
template <> std::string get_type_name<uint64_t>()   { return "u64" ; }
template <> std::string get_type_name<int64_t>()    { return "i64" ; }
template <> std::string get_type_name<float>()      { return "float32" ; }
template <> std::string get_type_name<double>()     { return "float64" ; }

template<typename T_>
::google::protobuf::RepeatedField< T_ >* get_mutable_data_field( Array::ArrayData * pDataFields )
{ BOOST_STATIC_ASSERT( sizeof(T_) == 0 && "No message field member defined for this type!" ) ; }

// Note: uint8 and int8 are not implemented because they use std::string instead of RepeatedField<T>
template <> ::google::protobuf::RepeatedField<uint32_t>* get_mutable_data_field( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datau32() ; }
template <> ::google::protobuf::RepeatedField<int32_t>*  get_mutable_data_field( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datai32() ; }
template <> ::google::protobuf::RepeatedField<uint64_t>* get_mutable_data_field( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datau64() ; }
template <> ::google::protobuf::RepeatedField<int64_t>*  get_mutable_data_field( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datai64() ; }
template <> ::google::protobuf::RepeatedField<float>*    get_mutable_data_field( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datafloat32() ; }
template <> ::google::protobuf::RepeatedField<double>*   get_mutable_data_field( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datafloat64() ; }

template <typename T_>
void populate_description( DatasetDescription * pDescription )
{
    Bounds * pBounds = pDescription->mutable_bounds() ;

    uint32_t start[] = {0,0,0,0,0};
    uint32_t stop[] = {0,0,0,0,0};
    std::string axes[] = {"t", "x", "y", "z", "c"} ;

    BOOST_FOREACH( uint32_t x, start )
    {
        pBounds->add_start(x);
    }
    BOOST_FOREACH( uint32_t x, stop )
    {
        pBounds->add_stop(x);
    }
    BOOST_FOREACH( std::string const & s, axes )
    {
        pDescription->add_axisnames( s ) ;
    }

    std::string type_name = get_type_name<T_>() ;
    std::cout << "Type name is: " << type_name << std::endl ;
    pDescription->set_datatype( type_name ) ;
}

template <typename T_>
void populate_data( Array::ArrayData * pDataFields, std::vector<T_> const & vec )
{
    get_mutable_data_field<T_>( pDataFields )->Reserve( vec.size() ) ;
    BOOST_FOREACH( T_ d, vec )
    {
        get_mutable_data_field<T_>( pDataFields )->Add( d ) ;
    }
}

// Special implementation for int8 and uint8
template<typename T_>
void populate_byte_data( Array::ArrayData * pDataFields, std::vector<T_> const & vec )

{
    std::string * pBytes = pDataFields->mutable_data8() ;
    pBytes->reserve( vec.size() ) ;
    std::copy( vec.begin(), vec.end(), std::back_inserter( *pBytes ) ) ;
}

template <>
void populate_data<uint8_t>( Array::ArrayData * pDataFields, std::vector<uint8_t> const & vec )
{
    populate_byte_data<uint8_t>( pDataFields, vec ) ;
}

template <>
void populate_data<int8_t>( Array::ArrayData * pDataFields, std::vector<int8_t> const & vec )
{
    populate_byte_data<int8_t>( pDataFields, vec ) ;
}

// ******************************************************************
// @details
// Create a protobuf message struct and populate it with the data
//  from the provided vector.
//
// Note: The description fields are given default (nonsense) values for now.
// ******************************************************************
template <typename T_>
ArrayPtr convert_to_protobuf( std::vector<T_> const & vec )
{
    ArrayPtr pResult( new Array() );
    populate_description<T_>( pResult->mutable_description() ) ;
    populate_data( pResult->mutable_data(), vec ) ;
    return pResult ;
}

// ******************************************************************
// @details
// Run a serialization and deserialization benchmark for an array of
//  the given datatype and length.
// ******************************************************************
template <typename T_>
void run_benchmark( size_t len )
{
    std::vector<T_> random_data;
    populate_random<T_>( random_data, len );

    std::stringstream ss ;

    // Serialize
    {
        ArrayPtr pArray = convert_to_protobuf( random_data ) ;

        Timer timer ;
        {
            Timer::Token token(timer) ;
            pArray->SerializeToOstream( &ss ) ;
        }
        std::cout << "Serialization took: " << timer.seconds() << " seconds." << std::endl ;

        int bufferSize = ss.tellp() - ss.tellg() ;
        std::cout << std::fixed << std::setprecision(3) ;
        std::cout << "Serialized data is " << bufferSize / 1.0e6 << " MB" << std::endl ;
    }

    // Deserialize
    {
        ArrayPtr pArray( new Array() ) ;

        Timer timer ;
        {
            Timer::Token token(timer) ;
            pArray->ParseFromIstream( &ss ) ;
        }
        std::cout << "Deserialization took: " << timer.seconds() << " seconds." << std::endl ;
    }

    std::cout << "Finished." << std::endl ;
}


// ******************************************************************
// Entry point.
// ******************************************************************
int main()
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    run_benchmark<uint8_t>( 100*100*100 ) ;
    run_benchmark<int8_t>( 100*100*100 ) ;

//    run_benchmark<uint16_t>( 100*100*100 ) ;
//    run_benchmark<int16_t>( 100*100*100 ) ;

    run_benchmark<uint32_t>( 100*100*100 ) ;
    run_benchmark<int32_t>( 100*100*100 ) ;

    run_benchmark<uint64_t>( 100*100*100 ) ;
    run_benchmark<int64_t>( 100*100*100 ) ;

    run_benchmark<float>( 100*100*100 ) ;
    run_benchmark<double>( 100*100*100 ) ;

	return 0;
}
