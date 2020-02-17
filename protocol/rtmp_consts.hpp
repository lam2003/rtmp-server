/*
 * @Author: linmin
 * @Date: 2020-02-11 20:15:08
 * @LastEditTime: 2020-02-17 12:45:40
 */
#ifndef RS_CONSTS_H
#define RS_CONSTS_H

#define RTMP_SEND_TIMEOUT_US (int64_t)(30 * 1000 * 1000LL)
#define RTMP_RECV_TIMEOUT_US (int64_t)(30 * 1000 * 1000LL)

#define RTMP_DEFAULT_WINDOW_ACK_SIZE (2.5 * 1000 * 1000)
#define RTMP_DEFAULT_PEER_BAND_WIDTH (2.5 * 1000 * 1000)

#define RTMP_DEFAULT_PORT "1935"
#define RTMP_DEFAULT_VHOST_PARAM "?vhost=__defaultVhost__"
#define RTMP_DEFAULT_VHOST "__defaultVhost__"

//rtmp chunk size
#define RTMP_DEFAULT_CHUNK_SIZE 128
#define RTMP_MIN_CHUNK_SIZE 128
#define RTMP_MAX_CHUNK_SIZE 65535
//rtmp chunk stream cache num
#define RTMP_CHUNK_STREAM_CHCAHE 16
//rtmp fmt0 header size(max base header)
#define RTMP_FMT0_HEADER_SIZE 16
//rtmp timestamp_delta when extended timestamp enabled
#define RTMP_EXTENDED_TIMESTAMP 0xffffff
//rtmp jitter duration
#define RTMP_MAX_JITTER_MS 250
#define RTMP_MAX_JITTER_MS_NEG -250
#define RTMP_DEFAULT_FRAME_TIME_MS 10
//rtmp marge read small bytes
#define RTMP_MR_SMALL_BYTES 4096
#define RTMP_MR_MSGS 128

//rtmp message header type
#define RTMP_FMT_TYPE0 0
#define RTMP_FMT_TYPE1 1
#define RTMP_FMT_TYPE2 2
#define RTMP_FMT_TYPE3 3

//rtmp cid
#define RTMP_CID_PROTOCOL_CONTROL 0x02
#define RTMP_CID_OVER_CONNECTION 0x03
#define RTMP_CID_OVER_CONNECTION2 0x04
#define RTMP_CID_VIDEO 0x06
#define RTMP_CID_AUDIO 0x07

//rtmp message type
#define RTMP_MSG_SET_CHUNK_SIZE 0x01
#define RTMP_MSG_ABORT 0x02
#define RTMP_MSG_ACK 0x03
#define RTMP_MSG_USER_CONTROL_MESSAGE 0x04
#define RTMP_MSG_WINDOW_ACK_SIZE 0x05
#define RTMP_MSG_SET_PEER_BANDWIDTH 0x06
#define RTMP_MSG_EDGE_AND_ORIGIN_SERVER_COMMAND 0x07
#define RTMP_MSG_AUDIO_MESSAGE 0x08
#define RTMP_MSG_VIDEO_MESSAGE 0x09
#define RTMP_MSG_AMF3_COMMAND 0x11
#define RTMP_MSG_AMF0_COMMAND 0x14
#define RTMP_MSG_AMF3_DATA 0x0f
#define RTMP_MSG_AMF0_DATA 0x12
#define RTMP_MSG_AMF3_SHARED_OBJ 0x10
#define RTMP_MSG_AMF0_SHARED_OBJ 0x13
#define RTMP_MSG_AGGREGATE 0x16

//amf0 command message
#define RTMP_AMF0_COMMAND_CONNECT "connect"
#define RTMP_AMF0_COMMAND_RESULT "_result"
#define RTMP_AMF0_COMMAND_RELEASE_STREAM "releaseStream"
#define RTMP_AMF0_COMMAND_FC_PUBLISH "FCPublish"
#define RTMP_AMF0_COMMAND_UNPUBLISH "FCUnpublish"
#define RTMP_AMF0_COMMAND_PUBLISH "publish"
#define RTMP_AMF0_COMMAND_CREATE_STREAM "createStream"
#define RTMP_AMF0_COMMAND_ON_STATUS "onStatus"
#define RTMP_AMF0_COMMAND_ERROR "error"
#define RTMP_AMF0_COMMAND_ON_FC_PUBLISH "onFCPublish"
#define RTMP_AMF0_COMMAND_ON_FC_UNPUBLISH "onFCUnpublish"
#define RTMP_AMF0_COMMAND_ON_METADATA "onMetaData"
#define RTMP_AMF0_COMMAND_SET_DATAFRAME "@setDataFrame"


#endif