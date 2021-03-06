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

template <>
float get_uniform_random<float>()
{
    // For some reason using the true max/min possible values is very slow.
    //static const double lowest = boost::numeric::bounds<float>::lowest() ;
    //static const double highest = boost::numeric::bounds<float>::highest() ;
    //static boost::random::uniform_real_distribution<float> dist(lowest, highest);

    // Just use max/min of -1/1 for speed.  Should be almost random data (except exponent...)
    static boost::random::uniform_real_distribution<float> dist(-1.0, 1.0);
    static boost::random::mt19937 gen;
    return dist(gen) ;
}

template <>
double get_uniform_random<double>()
{
    // For some reason using the true max/min possible values is very slow.
    //static const double lowest = boost::numeric::bounds<double>::lowest() ;
    //static const double highest = boost::numeric::bounds<double>::highest() ;
    //static boost::random::uniform_real_distribution<double> dist(lowest, highest);

    // Just use max/min of -1/1 for speed
    static boost::random::uniform_real_distribution<float> dist(-1.0, 1.0);
    static boost::random::mt19937 gen;
    return dist(gen) ;
}

// ******************************************************************
// @details
// Populate a vector with uniformly distributed random data
// ******************************************************************
template <typename T_>
void fill_random( std::vector<T_> & vec, size_t len )
{
    vec.resize(len);
    BOOST_FOREACH( T_ & x, vec )
    {
        x = get_uniform_random<T_>() ;
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

template <typename T_, typename T2_ = typename boost::disable_if_c<sizeof(T_) == sizeof(uint16_t)>::type>
::google::protobuf::RepeatedField<T_>* get_mutable_data_field( Array::ArrayData * pDataFields )
{ BOOST_STATIC_ASSERT( sizeof(T_) == 0 && "No message field member defined for this type!" ) ; }

// Note: uint8 and int8 are not implemented because they use std::string instead of RepeatedField<T>
template <> ::google::protobuf::RepeatedField<uint32_t>* get_mutable_data_field<uint32_t>( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datau32() ; }
template <> ::google::protobuf::RepeatedField<int32_t>*  get_mutable_data_field<int32_t>( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datai32() ; }
template <> ::google::protobuf::RepeatedField<uint64_t>* get_mutable_data_field<uint64_t>( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datau64() ; }
template <> ::google::protobuf::RepeatedField<int64_t>*  get_mutable_data_field<int64_t>( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datai64() ; }
template <> ::google::protobuf::RepeatedField<float>*    get_mutable_data_field<float>( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datafloat32() ; }
template <> ::google::protobuf::RepeatedField<double>*   get_mutable_data_field<double>( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datafloat64() ; }

// We have to use special cases for the 16-bit dtypes.
// Hence, we overload the above template function with two additional template functions,
// and provide a specialization for each.
template <typename T_, typename T2_ = typename boost::enable_if<boost::is_same<T_, uint16_t> >::type>
::google::protobuf::RepeatedField<uint32_t>* get_mutable_data_field( Array::ArrayData * pDataFields )
{ BOOST_STATIC_ASSERT( sizeof(T_) == 0 && "No message field member defined for this type!" ) ; }
template <> ::google::protobuf::RepeatedField<uint32_t>* get_mutable_data_field<uint16_t>( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datau32() ; }

template <typename T_, typename T2_ = typename boost::enable_if<boost::is_same<T_, int16_t> >::type>
::google::protobuf::RepeatedField<int32_t>* get_mutable_data_field( Array::ArrayData * pDataFields )
{ BOOST_STATIC_ASSERT( sizeof(T_) == 0 && "No message field member defined for this type!" ) ; }
template <> ::google::protobuf::RepeatedField<int32_t>* get_mutable_data_field<int16_t>( Array::ArrayData * pDataFields ) { return pDataFields->mutable_datai32() ; }

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
    pDescription->set_datatype( type_name ) ;
}

// ******************************************************************
// Default implementation of populate_data (everything but 8-bit)
// ******************************************************************
template <typename T_>
void populate_data( Array::ArrayData * pDataFields, std::vector<T_> const & vec
    , typename boost::disable_if_c<sizeof(T_) == sizeof(uint8_t)>::type* dummy = 0 )
{
    get_mutable_data_field<T_>( pDataFields )->Reserve( vec.size() ) ;
    BOOST_FOREACH( T_ d, vec )
    {
        get_mutable_data_field<T_>( pDataFields )->Add( d ) ;
    }
}

// ******************************************************************
// Special implementation for int8 and uint8
//  (which use strings instead of RepeatedField)
// ******************************************************************
template <typename T_>
void populate_data( Array::ArrayData * pDataFields, std::vector<T_> const & vec
    , typename boost::enable_if_c<sizeof(T_) == sizeof(uint8_t)>::type* dummy = 0 )
{
    std::string * pBytes = pDataFields->mutable_data8() ;
    pBytes->reserve( vec.size() ) ;
    std::copy( vec.begin(), vec.end(), std::back_inserter( *pBytes ) ) ;
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

template <typename T_>
boost::shared_ptr<std::vector<T_> > convert_from_protobuf( Array::ArrayData const & dataFields )
{
    // Instead of reading the array data into some image, we'll just copy it into a big vector.
    // This should be roughly the same as a malloc + memcpy, but this function is here for completeness.

    boost::shared_ptr<std::vector<T_> > pVec( new std::vector<T_>() ) ;
    if ( sizeof(T_) == sizeof(uint8_t) )
    {
        std::string const & bytes = dataFields.data8() ;
        std::copy( bytes.begin(), bytes.end(), std::back_inserter( *pVec ) ) ;
    }
    else if ( boost::is_same<T_, uint16_t>::value or boost::is_same<T_, uint32_t>::value ) // both 16-bit and 32-bit use the same field
    {
        ::google::protobuf::RepeatedField< uint32_t > const & datau32 = dataFields.datau32() ;
        std::copy( datau32.begin(), datau32.end(), std::back_inserter( *pVec ) ) ;
    }
    else if ( boost::is_same<T_, int16_t>::value or boost::is_same<T_, int32_t>::value ) // both 16-bit and 32-bit use the same field
    {
        ::google::protobuf::RepeatedField< int32_t > const & datai32 = dataFields.datai32() ;
        std::copy( datai32.begin(), datai32.end(), std::back_inserter( *pVec ) ) ;
    }
    else if ( boost::is_same<T_, uint64_t>::value )
    {
        ::google::protobuf::RepeatedField< uint64_t > const & datau64 = dataFields.datau64() ;
        std::copy( datau64.begin(), datau64.end(), std::back_inserter( *pVec ) ) ;
    }
    else if ( boost::is_same<T_, int64_t>::value )
    {
        ::google::protobuf::RepeatedField< int64_t > const & datai64 = dataFields.datai64() ;
        std::copy( datai64.begin(), datai64.end(), std::back_inserter( *pVec ) ) ;
    }
    else if ( boost::is_same<T_, float>::value )
    {
        ::google::protobuf::RepeatedField< float > const & datafloat32 = dataFields.datafloat32() ;
        std::copy( datafloat32.begin(), datafloat32.end(), std::back_inserter( *pVec ) ) ;
    }
    else if ( boost::is_same<T_, double>::value )
    {
        ::google::protobuf::RepeatedField< double > const & datafloat64 = dataFields.datafloat64() ;
        std::copy( datafloat64.begin(), datafloat64.end(), std::back_inserter( *pVec ) ) ;
    }
    return pVec ;
}

struct BenchmarkStats
{
    std::string type_name ;
    float image_size_mb ;

    float message_size_mb;

    float message_creation_seconds;
    float serialization_seconds;
    float deserialization_seconds;
    float array_creation_seconds;
};

// ******************************************************************
// @details
// Run a serialization and deserialization benchmark for an array of
//  the given datatype and length.
// ******************************************************************
template <typename T_>
BenchmarkStats run_benchmark( size_t len )
{
    BenchmarkStats stats;

    stats.type_name = get_type_name<T_>() ;
    stats.image_size_mb = len * sizeof(T_) / 1.0e6 ;

    std::vector<T_> random_data;
    fill_random<T_>( random_data, len );

    std::stringstream ss ;

    // Serialize
    {
        ArrayPtr pArray ;

        Timer creation_timer ;
        {
            Timer::Token token(creation_timer) ;
            pArray = convert_to_protobuf( random_data ) ;
        }
        stats.message_creation_seconds = creation_timer.seconds();

        Timer serialization_timer ;
        {
            Timer::Token token(serialization_timer) ;
            pArray->SerializeToOstream( &ss ) ;
        }
        stats.serialization_seconds = serialization_timer.seconds();
        //std::cout << "Serialization took: " << timer.seconds() << " seconds." << std::endl ;

        int bufferSize = ss.tellp() - ss.tellg() ;
        stats.message_size_mb = bufferSize / 1.0e6 ;
        //std::cout << std::fixed << std::setprecision(3) ;
        //std::cout << "Serialized data is " << bufferSize / 1.0e6 << " MB" << std::endl ;
    }

    // Deserialize
    {
        ArrayPtr pArray( new Array() ) ;

        Timer timer ;
        {
            Timer::Token token(timer) ;
            pArray->ParseFromIstream( &ss ) ;
        }
        stats.deserialization_seconds = timer.seconds();
        //std::cout << "Deserialization took: " << timer.seconds() << " seconds." << std::endl ;

        Timer conversion_timer ;
        {
            Timer::Token token(conversion_timer) ;
            boost::shared_ptr<std::vector<T_> > pVec = convert_from_protobuf<T_>( pArray->data() ) ;
        }
        stats.array_creation_seconds = conversion_timer.seconds() ;
    }

    return stats;
}

void print_stat_header()
{
    std::cout << "image size,"
              << "dtype,"
              << "encoded message size,"
              << "message creation time,"
              << "serialization time,"
              << "deserialization time,"
              << "array creation time"
              << std::endl ;
}

void print_stats( BenchmarkStats const & stats )
{
    std::cout << stats.image_size_mb << ','
              << stats.type_name << ','
              << stats.message_size_mb << ','
              << stats.message_creation_seconds << ','
              << stats.serialization_seconds << ','
              << stats.deserialization_seconds << ','
              << stats.array_creation_seconds
              << std::endl ;
}

// ******************************************************************
// Entry point.
// ******************************************************************
int main()
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::vector<size_t> sizes = boost::assign::list_of
        (100*100)
        (100*1000)
        (1000*1000)
        (10*1000*1000)
        (100*1000*1000)
        (1000*1000*1000);

    print_stat_header();

    BOOST_FOREACH(size_t size, sizes)
    {
        print_stats( run_benchmark<uint8_t>( size ) ) ;
        print_stats( run_benchmark<int8_t>( size ) ) ;

        print_stats( run_benchmark<uint16_t>( size ) ) ;
        print_stats( run_benchmark<int16_t>( size ) ) ;

        print_stats( run_benchmark<uint32_t>( size ) ) ;
        print_stats( run_benchmark<int32_t>( size ) ) ;

        print_stats( run_benchmark<float>( size ) ) ;

        print_stats( run_benchmark<uint64_t>( size ) ) ;
        print_stats( run_benchmark<int64_t>( size ) ) ;

        print_stats( run_benchmark<double>( size ) ) ;
    }

	return 0;
}
