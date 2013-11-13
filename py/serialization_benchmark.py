import contextlib
import numpy
import StringIO
from lazyflow.utility import Timer

from dvidmsg_pb2 import Array
import proto_conversions

# Prepare array data
send_data = numpy.random.random( (100,100,100) )
send_data *= 100
send_data = send_data.astype( numpy.int32 )
data_mb = numpy.prod( send_data.shape ) * send_data.dtype.type().nbytes / 1e6
print "Array data is {} MB".format( data_mb )

# Format into message
with Timer() as timer:
    send_msg = proto_conversions.convert_array_to_dvidmsg( send_data )
print "Formatting message structure took {} seconds".format( timer.seconds() )

with contextlib.closing( StringIO.StringIO() ) as strbuf:
    # Serialize it
    with Timer() as timer:
        strbuf.write( send_msg.SerializeToString() )
    print "Serialization took {} seconds".format( timer.seconds() )

    # Read the data into a string before we discard strbuf
    strbuf.seek(0)
    buffer_data = strbuf.read()
    print "Serialized data is {} MB".format( len(buffer_data) / 1e6 )

# Deserialize
rcv_msg = Array()
with Timer() as timer:
    rcv_msg.ParseFromString( buffer_data )
print "Deserialization took {} seconds".format( timer.seconds() )

# Convert back to ndarray
with Timer() as timer:
    rcv_data = proto_conversions.convert_array_from_dvidmsg( rcv_msg )
print "Converting message structure to ndarray took {} seconds".format( timer.seconds() )

# Check the results.
print "Checking for errors..."
assert rcv_msg.description == send_msg.description, "Message description was not echoed correctly."
assert (rcv_data == send_data).all(), "Data mismatch after conversion"

print "Successful round-trip (de)serialization!"

