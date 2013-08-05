from zlib import crc32
from protocolwrapper import (
    ProtocolWrapper, ProtocolStatus)
from knot_packet import (
    payload_header, data_header, 
    data_payload, serial_query, Container)


PROTOCOL_HEADER = '\x12'
PROTOCOL_FOOTER = '\x13'
PROTOCOL_DLE = '\x7D'



def build_query(thingType):
    dp = data_payload.build(Container(
        seqno=1,
        src_chan_num=0,
        dst_chan_num=0,
        cmd=1,
        chksum=0,
        tlen=1,
        data="")
    )
    
    query = serial_query.build(Container(
        type=thingType)
    )
    msg = dp + query   
    pw = ProtocolWrapper(   
        header=PROTOCOL_HEADER,
        footer=PROTOCOL_FOOTER,
        dle=PROTOCOL_DLE)
    return pw.wrap(msg)

def build_message_to_send(  
        seqno, src_chan_num, dst_chan_num,
        cmd):
    """ Given the data, builds a message for 
        transmittion, computing the CRC and packing
        the protocol.
        Returns the packed message ready for 
        transmission on the serial port.
    """   
    # Build the raw message string. CRC is empty 
    # for now 
    #
    raw = data_payload.build(Container(
        seqno=seqno,
        src_chan_num=src_chan_num,
        dst_chan_num=dst_chan_num,
        cmd=cmd,
        chksum=0,
        tlen=0,
        data="")
    )
    # Compute the CRC field and append it to the
    # message instead of the empty CRC specified
    # initially.
    #
    #msg_crc = message_crc.build(Container(
        #crc=crc32(msg_without_crc)))
    
    # Append the CRC field
    #
    #msg = msg_without_crc + msg_crc
    
    pw = ProtocolWrapper(   
            header=PROTOCOL_HEADER,
            footer=PROTOCOL_FOOTER,
            dle=PROTOCOL_DLE)
    
    return pw.wrap(raw)
        


if __name__ == "__main__":
    # Sample: sending a message
    #
    msg = build_message_to_send(
        seqno=1,
        src_chan_num=1,
        dst_chan_num=0,
        cmd= 1,
        )
    
    print msg.encode('hex')

    # Sample: receiving a message
    #
    pw = ProtocolWrapper(   
            header=PROTOCOL_HEADER,
            footer=PROTOCOL_FOOTER,
            dle=PROTOCOL_DLE)
    
    
