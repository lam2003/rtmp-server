#ifndef RS_CONSTS_H
#define RS_CONSTS_H

#define RS_CONSTS_RTMP_SEND_TIMEOUT_US (int64_t)(30 * 1000 * 1000LL)
#define RS_CONSTS_RTMP_RECV_TIMEOUT_US (int64_t)(30 * 1000 * 1000LL)
//rtmp chunk stream cache num
#define RS_CONSTS_CHUNK_STREAM_CHCAHE 16

//rtmp message header type
#define RTMP_FMT_TYPE0 0
#define RTMP_FMT_TYPE1 1
#define RTMP_FMT_TYPE2 2
#define RTMP_FMT_TYPE3 3
//rtmp cid
#define RTMP_CID_PROTOCOL_CONTROL 0x02
//rtmp timestamp_delta when extended timestamp enabled
#define RTMP_EXTENDED_TIMESTAMP 0xffffff

#define SRS_CONSTS_RTMP_PROTOCOL_CHUNK_SIZE 128
#endif