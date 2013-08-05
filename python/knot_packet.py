from construct import *


#message_crc = Struct('message_crc', ULInt32('crc'))


payload_header = Struct('payload_header',
    ULInt8('seqno'),
    ULInt8('src_chan_num'),
    ULInt8('dst_chan_num'),
    ULInt8('cmd'),
    ULInt16('chksum'),
)

data_header = Struct('data_header',
    ULInt16('tlen'),
    )

data_payload = Struct('data_payload',
    Embed(payload_header),
    Embed(data_header),
    OptionalGreedyRange(ULInt8('data'))
    )

serial_query = Struct('serial_query',
    ULInt8('type')
    )

query_response = Struct('query_response',
    ULInt8('type'),
    ULInt16('rate'),
    CString('name'),
    )

if __name__ == "__main__":
    raw = data_payload.build(Container(
        seqno=1,
        src_chan_num=1,
        dst_chan_num=0,
        cmd= 1,
        chksum=1,
        tlen=0)
    )

    print raw.encode('hex')
