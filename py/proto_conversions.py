import array
import collections
import numpy

from dvidmsg_pb2 import Array, DatasetDescription, Bounds

conversion_spec = collections.namedtuple('conversion_spec', 'dvid_type proto_type numpy_type msg_field')

conversion_specs = [ conversion_spec('i8',      'b',   numpy.int8,    'data8'),
                     conversion_spec('u8',      'B',   numpy.uint8,   'data8'),
                     conversion_spec('i16',     'h',   numpy.int16,   'dataI32'),
                     conversion_spec('u16',     'H',   numpy.uint16,  'dataU32'),
                     conversion_spec('i32',      int,  numpy.int32,   'dataI32'),
                     conversion_spec('u32',      long, numpy.uint32,  'dataU32'),
                     conversion_spec('i64',      long, numpy.int64,   'dataI64'),
                     conversion_spec('u64',      long, numpy.uint64,  'dataU64'),
                     conversion_spec('float32',  'f',  numpy.float32, 'dataFloat32'),
                     conversion_spec('float64',  'd',  numpy.float64, 'dataFloat64') ]

conversion_specs_from_dvid = { spec.dvid_type : spec for spec in conversion_specs }
conversion_specs_from_numpy = { spec.numpy_type : spec for spec in conversion_specs }

def convert_array_from_dvidmsg(msg):
    dvid_type, _, dtype, msg_field = conversion_specs_from_dvid[msg.description.datatype]
    msg_data = getattr( msg.data, msg_field )
    
    shape = numpy.array(msg.description.bounds.stop) - msg.description.bounds.start
    assert numpy.prod(shape) == len(msg_data), \
        "Array from server has mismatched length and shape: {}, {}".format( shape, len(msg_data) )

    # Use Fortran order, since that's the DVID convention
    result = numpy.ndarray( shape, dtype=dtype, order='F' )

    # numpy arrays are always indexed in C-order, 
    #  so we have to transpose before assigning the flat data.
    if dvid_type == 'i8' or dvid_type == 'u8':
        result.transpose().flat[:] = map(ord, msg_data)
    else:
        result.transpose().flat[:] = msg_data
    return result

def convert_array_to_dvidmsg(a, dvid_start=None):
    msg = Array()
    if dvid_start is None:
        dvid_start = (0,) * len(a.shape)
    msg.description.bounds.start.extend(dvid_start)
    msg.description.bounds.stop.extend(list(numpy.array(dvid_start) + a.shape))

    dvid_type, proto_type, _, msg_field = conversion_specs_from_numpy[a.dtype.type]
    msg.description.datatype = dvid_type

    # We assume this array is provided in fortran order, because that's the dvid convention.
    # But we need to transmit in C-order, so transpose() to get correct iteration order.
    flat_view = a.transpose().flat[:]

    if dvid_type == 'i8' or dvid_type == 'u8':
        # protobuf uses strings for 8-bit types, so convert to string.
        # There must be a better way of doing this than making a big copy like this...
        setattr( msg.data, msg_field, "".join(map(chr, flat_view.view(numpy.uint8))) )
    elif isinstance( proto_type, str ):
        getattr( msg.data, msg_field ).extend( array.array(proto_type, flat_view) )
    else:
        getattr( msg.data, msg_field ).extend( map(proto_type, flat_view) )

    return msg

