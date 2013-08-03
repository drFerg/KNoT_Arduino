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
