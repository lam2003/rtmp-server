#include <protocol/rtmp/packet.hpp>
#include <protocol/rtmp/defines.hpp>
#include <protocol/rtmp/stack.hpp>
#include <protocol/amf/amf0.hpp>
#include <common/utils.hpp>
#include <common/log.hpp>
#include <common/error.hpp>

namespace rtmp
{

Packet::Packet()
{
}

Packet::~Packet()
{
}

int Packet::GetPreferCID()
{
    return 0;
}

int Packet::GetMessageType()
{
    return 0;
}

int Packet::GetSize()
{
    return 0;
}

int Packet::Encode(int &psize, char *&ppayload)
{
    int ret = ERROR_SUCCESS;

    int size = GetSize();
    char *payload = nullptr;

    BufferManager manager;
    if (size > 0)
    {
        payload = new char[size];

        if ((ret = manager.Initialize(payload, size)) != ERROR_SUCCESS)
        {
            rs_error("initialize buffer manager failed. ret=%d", ret);
            rs_freepa(payload);
            return ret;
        }
    }

    if ((ret = EncodePacket(&manager)) != ERROR_SUCCESS)
    {
        rs_error("encode the packet failed. ret=%d", ret);
        rs_freepa(payload);
        return ret;
    }

    psize = size;
    ppayload = payload;

    return ret;
}

int Packet::Decode(BufferManager *manager)
{
    return ERROR_SUCCESS;
}

int Packet::EncodePacket(BufferManager *manager)
{
    return ERROR_SUCCESS;
}

SetChunkSizePacket::SetChunkSizePacket()
{
    chunk_size = RTMP_DEFAULT_CHUNK_SIZE;
}

SetChunkSizePacket::~SetChunkSizePacket()
{
}

int SetChunkSizePacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(4))
    {
        ret = ERROR_RTMP_MESSAGE_DECODE;
        rs_error("decode set_chunk_size packet failed. ret=%d", ret);
        return ret;
    }

    chunk_size = manager->Read4Bytes();

    rs_trace("decode set_chunk_size packet success");

    return ret;
}

int SetChunkSizePacket::GetPreferCID()
{
    return RTMP_CID_PROTOCOL_CONTROL;
}

int SetChunkSizePacket::GetMessageType()
{
    return RTMP_MSG_SET_CHUNK_SIZE;
}

int SetChunkSizePacket::GetSize()
{
    return 4;
}

int SetChunkSizePacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(4))
    {
        ret = ERROR_RTMP_MESSAGE_ENCODE;
        rs_error("encode set_chunk_size packet failed. ret=%d", ret);
        return ret;
    }

    manager->Write4Bytes(chunk_size);

    rs_trace("encode set_chunk_size packet success");

    return ret;
}

ConnectAppPacket::ConnectAppPacket()
{
    command_name = RTMP_AMF0_COMMAND_CONNECT;
    transaction_id = 1;
    command_object = AMF0Any::Object();
    args = nullptr;
}

ConnectAppPacket::~ConnectAppPacket()
{
    rs_freep(command_object);
    rs_freep(args);
}

int ConnectAppPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0ReadString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("decode connect_app packet: amf0 read command failed. ret=%d", ret);
        return ret;
    }

    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_CONNECT)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode connect_app packet: amf0 read command failed. require=%s, actual=%s, ret=%d",
                 RTMP_AMF0_COMMAND_CONNECT,
                 (command_name.empty() ? "[EMPTY]" : command_name.c_str()),
                 ret);
        return ret;
    }

    if ((ret = AMF0ReadNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("decode connect_app packet: amf0 read transaction_id failed. ret=%d", ret);
        return ret;
    }

    if (transaction_id != 1.0)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode connect_app packet: amf0 read transaction_id failed. require=1.0, actual=%.1f, ret=%d", transaction_id, ret);
    }

    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode connect_app packet: amf0 read object failed. ret=%d", ret);
            return ret;
        }
        if (!p->IsObject())
        {
            rs_freep(p);
            ret = ERROR_PROTOCOL_AMF0_DECODE;
            rs_error("decode connect_app packet: amf0 read object failed. type wrong, ret=%d", ret);
            return ret;
        }
        else
        {
            rs_freep(command_object);
            command_object = p->ToObject();
        }
    }
    {
        AMF0Any *p = nullptr;
        if (!manager->Empty())
        {
            if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
            {
                rs_freep(p);
                rs_error("decode connect_app packet: amf0 read args failed. ret=%d", ret);
                return ret;
            }

            if (!p->IsObject())
            {
                rs_warn("decode connect_app packet: drop args, marker=%#x", p->marker);
                rs_freep(p);
            }
            else
            {
                rs_freep(args);
                args = p->ToObject();
            }
        }
    }

    rs_trace("decode connect_app packet success");

    return ret;
}

int ConnectAppPacket::GetPreferCID()
{
    return RTMP_CID_OVER_CONNECTION;
}
int ConnectAppPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_COMMAND;
}

int ConnectAppPacket::GetSize()
{
    int size = 0;
    size += AMF0_LEN_STR(command_name);
    size += AMF0_LEN_NUMBER;
    size += AMF0_LEN_OBJECT(command_object);
    if (args)
    {
        size += AMF0_LEN_OBJECT(args);
    }
    return size;
}

int ConnectAppPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0WriteString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("encode connect_app packet: amf0 write command failed. ret=%d", ret);
        return ret;
    }

    if ((ret = AMF0WriteNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("encode connect_app packet: amf0 write transaction_id failed. ret=%d");
        return ret;
    }

    if ((ret = command_object->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode connect_app packet: amf0 write object failed. ret=%d", ret);
        return ret;
    }

    if (args && (ret = args->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode connect_app packet: amf0 write args failed. ret=%d", ret);
        return ret;
    }

    rs_trace("encode connect_app packet success");

    return ret;
}

ConnectAppResPacket::ConnectAppResPacket()
{
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = 1;
    props = AMF0Any::Object();
    info = AMF0Any::Object();
}

ConnectAppResPacket::~ConnectAppResPacket()
{
    rs_freep(props);
    rs_freep(info);
}

int ConnectAppResPacket::GetPreferCID()
{
    return RTMP_CID_PROTOCOL_CONTROL;
}

int ConnectAppResPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_COMMAND;
}

int ConnectAppResPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0ReadString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("decode connect_app response packet: amf0 read command failed. ret=%d", ret);
        return ret;
    }

    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_RESULT)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode connect_app response packet: amf0 read command failed. require=%s, actual=%s, ret=%d", RTMP_AMF0_COMMAND_RESULT, command_name.c_str(), ret);
        return ret;
    }

    if ((ret = AMF0ReadNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("decode connect_app response packet: amf0 read transaction failed. ret=%d", ret);
        return ret;
    }

    if (transaction_id != 1.0)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode connect_app response packet: amf0 read transaction failed. require=1.0, actual=%.1f, ret=%d", transaction_id, ret);
        return ret;
    }
    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode connect_app response packet: amf0 read properties failed. ret=%d", ret);
            return ret;
        }

        if (!p->IsObject())
        {
            rs_warn("decode connect_app response packet: ignore properties, marker=%#x", p->marker);
            rs_freep(p);
        }
        else
        {
            rs_freep(props);
            props = p->ToObject();
        }
    }
    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode connect_app response packet: amf0 read info failed. ret=%d", ret);
            return ret;
        }

        if (!p->IsObject())
        {
            rs_freep(p);
            ret = ERROR_PROTOCOL_AMF0_DECODE;
            rs_error("decode connect_app response packet: amf0 read info failed. type wrong, ret=%d", ret);
            return ret;
        }
        else
        {
            rs_freep(info);
            info = p->ToObject();
        }
    }
    rs_trace("decode connect_app response packet success");

    return ret;
}

int ConnectAppResPacket::GetSize()
{
    int size = 0;
    size += AMF0_LEN_STR(command_name);
    size += AMF0_LEN_NUMBER;
    size += AMF0_LEN_OBJECT(props);
    size += AMF0_LEN_OBJECT(info);
    return size;
}

int ConnectAppResPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0WriteString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("encode connect_app response packet: amf0 write command failed,ret=%d", ret);
        return ret;
    }

    if ((ret = AMF0WriteNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("encode connect_app response packet: amf0 write transaction_id failed. ret=%d", ret);
        return ret;
    }

    if ((ret = props->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode connect_app response packet: amf0 write props failed. ret=%d", ret);
        return ret;
    }

    if ((ret = info->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode connect_app response packet: amf0 write info failed. ret=%d", ret);
        return ret;
    }

    rs_trace("encode connect_app response packet success");

    return ret;
}

SetWindowAckSizePacket::SetWindowAckSizePacket()
{
    ackowledgement_window_size = 0;
}

SetWindowAckSizePacket::~SetWindowAckSizePacket()
{
}

int SetWindowAckSizePacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;
    if (!manager->Require(4))
    {
        ret = ERROR_RTMP_MESSAGE_DECODE;
        rs_error("decode set_ack_window_size packet failed. ret=%d", ret);
        return ret;
    }

    ackowledgement_window_size = manager->Read4Bytes();

    rs_trace("decode set_ack_window_size packet success");

    return ret;
}

int SetWindowAckSizePacket::GetPreferCID()
{
    return RTMP_CID_PROTOCOL_CONTROL;
}

int SetWindowAckSizePacket::GetMessageType()
{
    return RTMP_MSG_WINDOW_ACK_SIZE;
}

int SetWindowAckSizePacket::GetSize()
{
    return 4;
}

int SetWindowAckSizePacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(4))
    {
        ret = ERROR_RTMP_MESSAGE_ENCODE;
        rs_error("encode set_ack_window_size packet failed. ret=%d", ret);
        return ret;
    }

    manager->Write4Bytes(ackowledgement_window_size);

    rs_trace("encode set_ack_window_size packet success");

    return ret;
}

SetPeerBandwidthPacket::SetPeerBandwidthPacket()

{
    bandwidth = 0;
    type = (int8_t)PeerBandwidthType::DYNAMIC;
}

SetPeerBandwidthPacket::~SetPeerBandwidthPacket()
{
}

int SetPeerBandwidthPacket::GetPreferCID()
{
    return RTMP_CID_PROTOCOL_CONTROL;
}

int SetPeerBandwidthPacket::GetMessageType()
{
    return RTMP_MSG_SET_PEER_BANDWIDTH;
}

int SetPeerBandwidthPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(5))
    {
        ret = ERROR_RTMP_MESSAGE_DECODE;
        rs_error("decode set_peer_bw_packet failed. ret=%d", ret);
        return ret;
    }

    bandwidth = manager->Read4Bytes();
    type = manager->Read1Bytes();

    rs_trace("decode set_peer_bw_packet success");

    return ret;
}

int SetPeerBandwidthPacket::GetSize()
{
    return 5;
}

int SetPeerBandwidthPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(5))
    {
        ret = ERROR_RTMP_MESSAGE_ENCODE;
        rs_error("encode set_peer_bw_packet failed. ret=%d", ret);
        return ret;
    }

    manager->Write4Bytes(bandwidth);
    manager->Write1Bytes(type);

    rs_trace("encode set_peer_bw_packet success");

    return ret;
}

FMLEStartPacket::FMLEStartPacket()
{
    command_name = RTMP_AMF0_COMMAND_RELEASE_STREAM;
    transaction_id = 0;
    command_object = AMF0Any::Null();
    stream_name = "";
}

FMLEStartPacket::~FMLEStartPacket()
{
    rs_freep(command_object);
}

int FMLEStartPacket::GetPreferCID()
{
    return RTMP_CID_OVER_CONNECTION;
}

int FMLEStartPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_COMMAND;
}

int FMLEStartPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0ReadString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("decode FMLE_start packet: amf0 read command failed. ret=%d", ret);
        return ret;
    }

    if (command_name.empty() || (command_name != RTMP_AMF0_COMMAND_RELEASE_STREAM &&
                                 command_name != RTMP_AMF0_COMMAND_FC_PUBLISH &&
                                 command_name != RTMP_AMF0_COMMAND_UNPUBLISH))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode FMLE_start packet: amf0 read command failed. require=%s/%s/%s, actual=%s, ret=%d",
                 RTMP_AMF0_COMMAND_RELEASE_STREAM,
                 RTMP_AMF0_COMMAND_FC_PUBLISH,
                 RTMP_AMF0_COMMAND_UNPUBLISH,
                 (command_name.empty() ? "[EMPTY]" : command_name.c_str()),
                 ret);
        return ret;
    }

    if ((ret = AMF0ReadNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("decode FMLE_start packet: amf0 read transaction_id failed. ret=%d", ret);
        return ret;
    }
    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode FMLE_start packet: amf0 read object failed. ret=%d", ret);
            return ret;
        }
        else
        {
            rs_freep(command_object);
            command_object = p;
        }
    }
    if ((ret = AMF0ReadString(manager, stream_name)) != ERROR_SUCCESS)
    {
        rs_error("decode FMLE_start packet: amf0 read FMLE stream_name failed. ret=%d", ret);
        return ret;
    }

    rs_trace("decode FMLE_start packet success");

    return ret;
}

int FMLEStartPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0WriteString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("encode FMLE_start packet: amf0 write command failed. ret=%d", ret);
        return ret;
    }

    if ((ret = AMF0WriteNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("encode FMLE_start packet: amf0 write transaction_id failed. ret=%d", ret);
        return ret;
    }

    if ((ret = command_object->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode FMLE_start packet: amf0 write object failed. ret=%d", ret);
        return ret;
    }
    if ((ret = AMF0WriteString(manager, stream_name)) != ERROR_SUCCESS)
    {
        rs_error("encode FMLE_start packet: amf0 write stream_name failed. ret=%d", ret);
        return ret;
    }

    rs_trace("encode FMLE_start packet success");

    return ret;
}

int FMLEStartPacket::GetSize()
{
    int size = 0;
    size += AMF0_LEN_STR(command_name);
    size += AMF0_LEN_NUMBER;
    size += AMF0_LEN_ANY(command_object);
    size += AMF0_LEN_STR(stream_name);
    return size;
}

FMLEStartResPacket::FMLEStartResPacket(double trans_id)
{
    transaction_id = trans_id;
    command_name = RTMP_AMF0_COMMAND_RESULT;
    command_object = AMF0Any::Null();
    args = AMF0Any::Undefined();
}

FMLEStartResPacket::~FMLEStartResPacket()
{
    rs_freep(command_object);
    rs_freep(args);
}

int FMLEStartResPacket::GetPreferCID()
{
    return RTMP_CID_OVER_CONNECTION;
}

int FMLEStartResPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_COMMAND;
}

int FMLEStartResPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0ReadString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("decode FMLE_start response packet: amf0 read command failed. ret=%d", ret);
        return ret;
    }

    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_RESULT)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode FMLE_start response packet: amf0 read command failed. require=%s, actual=%s, ret=%d",
                 RTMP_AMF0_COMMAND_RESULT,
                 (command_name.empty() ? "[EMPTY]" : command_name.c_str()),
                 ret);
        return ret;
    }

    if ((ret = AMF0ReadNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("decode FMLE_start response packet: amf0 read transaction_id failed. ret=%d", ret);
        return ret;
    }
    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode FMLE_start response packet: amf0 read object failed. ret=%d", ret);
            return ret;
        }
        else
        {
            rs_freep(command_object);
            command_object = p;
        }
    }
    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode FMLE_start response packet: amf0 read args failed. ret=%d", ret);
            return ret;
        }
        else
        {
            rs_freep(args);
            args = p;
        }
    }

    rs_trace("decode FMLE_start response packet success");

    return ret;
}

int FMLEStartResPacket::GetSize()
{
    int size = 0;
    size += AMF0_LEN_STR(command_name);
    size += AMF0_LEN_NUMBER;
    size += AMF0_LEN_ANY(command_object);
    size += AMF0_LEN_ANY(args);
    return size;
}

int FMLEStartResPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0WriteString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("encode FMLE_start response packet: amf0 write command failed. ret=%d", ret);
        return ret;
    }

    if ((ret = AMF0WriteNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("encode FMLE_start response packet: amf0 write transactoin_id failed. ret=%d", ret);
        return ret;
    }

    if ((ret = command_object->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode FMLE_start response packet: amf0 write object failed. ret=%d", ret);
        return ret;
    }

    if ((ret = args->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode FMLE_start response packet: amf0 write args failed. ret=%d", ret);
        return ret;
    }

    rs_trace("encode FMLE_start response packet success");

    return ret;
}

CreateStreamPacket::CreateStreamPacket()
{
    command_name = RTMP_AMF0_COMMAND_CREATE_STREAM;
    transaction_id = 2;
    command_object = AMF0Any::Null();
}

CreateStreamPacket::~CreateStreamPacket()
{
    rs_freep(command_object);
}

int CreateStreamPacket::GetPreferCID()
{
    return RTMP_CID_OVER_CONNECTION;
}

int CreateStreamPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_COMMAND;
}

int CreateStreamPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0ReadString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("decode create_stream packet: amf0 read command failed. ret=%d", ret);
        return ret;
    }

    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_CREATE_STREAM)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode create_stream packet: amf0 read command failed. require=%s, actual=%s ret=%d",
                 RTMP_AMF0_COMMAND_CREATE_STREAM,
                 (command_name.empty() ? "[EMPTY]" : command_name.c_str()),
                 ret);
        return ret;
    }

    if ((ret = AMF0ReadNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("decode create_stream packet: amf0 read transaction_id failed. ret=%d", ret);
        return ret;
    }

    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode create_stream packet: amf0 read object failed. ret=%d", ret);
            return ret;
        }
        else
        {
            rs_freep(command_object);
            command_object = p;
        }
    }

    rs_trace("decode create_stream packet success");

    return ret;
}

int CreateStreamPacket::GetSize()
{
    int size = 0;
    size += AMF0_LEN_STR(command_name);
    size += AMF0_LEN_NUMBER;
    size += AMF0_LEN_ANY(command_object);
    return size;
}
int CreateStreamPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0WriteString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("encode create_stream packet: amf0 write command failed. ret=%d", ret);
        return ret;
    }

    if ((ret = AMF0WriteNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("encode create_stream packet: amf0 write transaction_id failed. ret=%d", ret);
        return ret;
    }

    if ((ret = command_object->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode create_stream packet: amf0 write object failed. ret=%d", ret);
        return ret;
    }

    rs_trace("encode create_stream packet success");

    return ret;
}

CreateStreamResPacket::CreateStreamResPacket(double trans_id, int sid)
{
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = trans_id;
    command_object = AMF0Any::Null();
    stream_id = sid;
}

CreateStreamResPacket::~CreateStreamResPacket()
{
    rs_freep(command_object);
}

int CreateStreamResPacket::GetPreferCID()
{
    return RTMP_CID_OVER_CONNECTION;
}
int CreateStreamResPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_COMMAND;
}
int CreateStreamResPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0ReadString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("decode create_stream response packet: amf0 read command failed. ret=%d", ret);
        return ret;
    }

    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_RESULT)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode create_stream response packet: amf0 read command failed. require=%s, actual=%s, ret=%d",
                 RTMP_AMF0_COMMAND_RESULT,
                 (command_name.empty() ? "[EMPTY]" : command_name.c_str()),
                 ret);
        return ret;
    }

    if ((ret = AMF0ReadNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("decode create_stream response packet: amf0 read command failed. ret=%d", ret);
        return ret;
    }

    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode create_stream response packet: amf0 read object failed. ret=%d", ret);
            return ret;
        }
        else
        {
            rs_freep(command_object);
            command_object = p;
        }
    }

    if ((ret = AMF0ReadNumber(manager, stream_id)) != ERROR_SUCCESS)
    {
        rs_error("decode create_stream response packet. amf0 read stream_id failed. ret=%d", ret);
        return ret;
    }

    rs_trace("decode create_stream response packet");

    return ret;
}

int CreateStreamResPacket::GetSize()
{
    int size = 0;
    size += AMF0_LEN_STR(command_name);
    size += AMF0_LEN_NUMBER;
    size += AMF0_LEN_ANY(command_object);
    size += AMF0_LEN_NUMBER;
    return size;
}

int CreateStreamResPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0WriteString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("encode create_stream response packet: amf0 write command failed. ret=%d", ret);
        return ret;
    }

    if ((ret = AMF0WriteNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("encode create_stream response packet: amf0 write transaction_id failed. ret=%d", ret);
        return ret;
    }

    if ((ret = command_object->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode create_stream response packet: amf0 write object failed. ret=%d", ret);
        return ret;
    }

    if ((ret = AMF0WriteNumber(manager, stream_id)) != ERROR_SUCCESS)
    {
        rs_error("encode create_stream response packet: amf0 write stream_id failed. ret=%d", ret);
        return ret;
    }

    rs_trace("encode create_stream response packet success");

    return ret;
}

PublishPacket::PublishPacket()
{
    command_name = RTMP_AMF0_COMMAND_PUBLISH;
    transaction_id = 0;
    command_object = AMF0Any::Null();
    stream_name = "";
    type = "live";
}

PublishPacket::~PublishPacket()
{
    rs_freep(command_object);
}

int PublishPacket::GetPreferCID()
{
    return RTMP_CID_OVER_CONNECTION;
}
int PublishPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_COMMAND;
}
int PublishPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0ReadString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("decode publish packet: amf0 read command failed. ret=%d", ret);
        return ret;
    }

    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PUBLISH)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode publish packet: amf0 read command failed. require=%s, actual=%s, ret=%d",
                 RTMP_AMF0_COMMAND_PUBLISH,
                 (command_name.empty() ? "[EMPTY]" : command_name.c_str()),
                 ret);
        return ret;
    }

    if ((ret = AMF0ReadNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("decode publish packet: amf0 read transaction_id failed. ret=%d", ret);
        return ret;
    }
    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode publish packet: amf0 read object failed. ret=%d", ret);
            return ret;
        }
        else
        {
            rs_freep(command_object);
            command_object = p;
        }
    }

    if ((ret = AMF0ReadString(manager, stream_name)) != ERROR_SUCCESS)
    {
        rs_error("decode publish packet: amf0 read stream_name failed. ret=%d", ret);
        return ret;
    }
    if ((ret = AMF0ReadString(manager, type)) != ERROR_SUCCESS)
    {
        rs_error("decode publish packet: amf0 read type failed. ret=%d", ret);
        return ret;
    }

    rs_trace("decode publish packet success");

    return ret;
}

int PublishPacket::GetSize()
{
    int size = 0;
    size += AMF0_LEN_STR(command_name);
    size += AMF0_LEN_NUMBER;
    size += AMF0_LEN_ANY(command_object);
    size += AMF0_LEN_STR(stream_name);
    size += AMF0_LEN_STR(type);
    return size;
}
int PublishPacket::EncodePacket(BufferManager *manager)
{
    //TODO implement encode
    int ret = ERROR_SUCCESS;
    return ret;
}

OnStatusCallPacket::OnStatusCallPacket()
{
    command_name = RTMP_AMF0_COMMAND_ON_STATUS;
    transaction_id = 0;
    args = AMF0Any::Null();
    data = AMF0Any::Object();
}
OnStatusCallPacket::~OnStatusCallPacket()
{
    rs_freep(args);
    rs_freep(data);
}
int OnStatusCallPacket::GetPreferCID()
{
    return RTMP_CID_OVER_CONNECTION;
}
int OnStatusCallPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_COMMAND;
}
int OnStatusCallPacket::Decode(BufferManager *manager)
{
    //TODO implement decode
    int ret = ERROR_SUCCESS;
    return ret;
}
int OnStatusCallPacket::GetSize()
{
    int size = 0;
    size += AMF0_LEN_STR(command_name);
    size += AMF0_LEN_NUMBER;
    size += AMF0_LEN_ANY(args);
    size += AMF0_LEN_OBJECT(data);
    return size;
}
int OnStatusCallPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0WriteString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("encode on_status_call packet: amf0 write command failed. ret=%d", ret);
        return ret;
    }

    if ((ret = AMF0WriteNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("encode on_status_call packet: amf0 write transaction_id failed. ret=%d", ret);
        return ret;
    }

    if ((ret = args->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode on_status_call packet: amf0 write args failed. ret=%d", ret);
        return ret;
    }

    if ((ret = data->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode on_status_call packet: amf0 write data failed. ret=%d", ret);
        return ret;
    }

    rs_trace("encode on_status_call packet success");

    return ret;
}

OnMetadataPacket::OnMetadataPacket()
{
    name = RTMP_AMF0_COMMAND_ON_METADATA;
    metadata = AMF0Any::Object();
}

OnMetadataPacket::~OnMetadataPacket()
{
    rs_freep(metadata);
}

int OnMetadataPacket::GetPreferCID()
{
    return RTMP_CID_OVER_CONNECTION2;
}

int OnMetadataPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_DATA;
}

int OnMetadataPacket::GetSize()
{
    return AMF0_LEN_STR(name) + AMF0_LEN_OBJECT(metadata);
}

int OnMetadataPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0WriteString(manager, name)) != ERROR_SUCCESS)
    {
        rs_error("encode on_metadata packet：amf0 write name failed. ret=%d", ret);
        return ret;
    }

    if ((ret = metadata->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode on_metadata packet：amf0 write metadata failed. ret=%d", ret);
        return ret;
    }

    rs_trace("encode on_metadata packet success");

    return ret;
}

int OnMetadataPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0ReadString(manager, name)) != ERROR_SUCCESS)
    {
        rs_error("decode on_metadata packet: amf0 read name failed. ret=%d", ret);
        return ret;
    }

    if (name == RTMP_AMF0_COMMAND_SET_DATAFRAME)
    {
        if ((ret = AMF0ReadString(manager, name)) != ERROR_SUCCESS)
        {
            rs_error("decode on_metadata packet: amf0 read name failed. ret=%d", ret);
            return ret;
        }
    }

    if (name.empty() || name != RTMP_AMF0_COMMAND_ON_METADATA)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode on_metadata packet: amf0 read name failed. require=%s, actual=%s, ret=%d",
                 RTMP_AMF0_COMMAND_ON_METADATA,
                 (name.empty() ? "[EMPTY]" : name.c_str()),
                 ret);
        return ret;
    }
    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode on_metadata packet: amf0 read metadata failed. ret=%d", ret);
            return ret;
        }

        if (p->IsObject())
        {
            rs_freep(metadata);
            metadata = p->ToObject();
        }
        else
        {
            rs_auto_free(AMF0Any, p);
            if (p->IsEcmaArray())
            {
                AMF0EcmaArray *arr = p->ToEcmaArray();
                for (int i = 0; i < arr->Count(); i++)
                {
                    metadata->Set(arr->KeyAt(i), arr->ValueAt(i)->Copy());
                }
            }
        }
    }

    rs_trace("decode on_metadata packet success");

    return ret;
}

PlayPacket::PlayPacket()
{
    command_name = RTMP_AMF0_COMMAND_PLAY;
    transaction_id = 0;
    command_obj = AMF0Any::Null();
    stream_name = "";
    start = -2;
    duration = -1;
    reset = true;
}

PlayPacket::~PlayPacket()
{
    rs_freep(command_obj);
}

int PlayPacket::GetPreferCID()
{
    return RTMP_CID_OVER_CONNECTION;
}

int PlayPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_COMMAND;
}

int PlayPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0ReadString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("decode play packet: amf0 read command failed. ret=%d", ret);
        return ret;
    }

    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PLAY)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("decode play packet: amf0 read command failed. require=%s, actual=%s, ret=%d",
                 RTMP_AMF0_COMMAND_PLAY,
                 (command_name.empty() ? "[EMPTY]" : command_name.c_str()), ret);
        return ret;
    }

    if ((ret = AMF0ReadNumber(manager, transaction_id)) != ERROR_SUCCESS)
    {
        rs_error("decode play packet: amf0 read transaction_id failed. ret=%d", ret);
        return ret;
    }
    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode play packet: amf0 read object failed. ret=%d", ret);
            return ret;
        }
        else
        {
            rs_freep(command_obj);
            command_obj = p;
        }
    }

    if ((ret = AMF0ReadString(manager, stream_name)) != ERROR_SUCCESS)
    {
        rs_error("decode play packet: amf0 read stream_name failed. ret=%d", ret);
        return ret;
    }

    if (!manager->Empty() && (ret = AMF0ReadNumber(manager, start)) != ERROR_SUCCESS)
    {
        rs_error("decode play packet: amf0 read start_pos failed. ret=%d", ret);
        return ret;
    }

    if (!manager->Empty() && (ret = AMF0ReadNumber(manager, duration)) != ERROR_SUCCESS)
    {
        rs_error("decode play packet: amf0 read duration failed. ret=%d", ret);
        return ret;
    }

    if (!manager->Empty())
    {
        AMF0Any *p = nullptr;
        if ((ret = AMF0ReadAny(manager, &p)) != ERROR_SUCCESS)
        {
            rs_freep(p);
            rs_error("decode play packet: amf0 read reset marker failed. ret=%d", ret);
            return ret;
        }

        rs_auto_freea(AMF0Any, p);

        if (p)
        {
            if (p->IsBoolean())
            {
                reset = p->ToBoolean();
            }
            else if (p->IsNumber())
            {
                reset = (p->ToNumber() != 0);
            }
            else
            {
                ret = ERROR_PROTOCOL_AMF0_DECODE;
                rs_error("decode play packet: amf0 read reset marker failed. invalid type=%#x, requires number or boolean, ret=%d",
                         p->marker,
                         ret);
                return ret;
            }
        }
    }

    rs_trace("decode play packet success");
    return ret;
}

int PlayPacket::GetSize()
{
    int size = 0;
    size += AMF0_LEN_STR(command_name);
    size += AMF0_LEN_NUMBER;
    size += AMF0_LEN_ANY(command_obj);
    size += AMF0_LEN_STR(stream_name);

    if (start != -2 || duration != -1 || !reset)
    {
        size += AMF0_LEN_NUMBER;
    }
    if (duration != -1 || !reset)
    {
        size += AMF0_LEN_NUMBER;
    }
    if (!reset)
    {
        size += AMF0_LEN_BOOLEAN;
    }

    return size;
}

int PlayPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;
    return ret;
}

UserControlPacket::UserControlPacket()
{
    event_type = 0;
    event_data = 0;
    extra_data = 0;
}

UserControlPacket::~UserControlPacket()
{
}

int UserControlPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(2))
    {
        ret = ERROR_RTMP_MESSAGE_DECODE;
        rs_error("decode user control packet: read event type failed. ret=%d", ret);
        return ret;
    }

    event_type = manager->Read2Bytes();

    if (event_type == (int16_t)UserEventType::FMS_EVENT0)
    {
        if (!manager->Require(1))
        {
            ret = ERROR_RTMP_MESSAGE_DECODE;
            rs_error("decode user control packet: read event_data failed. ret=%d", ret);
            return ret;
        }
        event_data = manager->Read1Bytes();
    }
    else
    {
        if (!manager->Require(4))
        {
            ret = ERROR_RTMP_MESSAGE_DECODE;
            rs_error("decode user control packet: read event_data failed. ret=%d", ret);
            return ret;
        }
        event_data = manager->Read4Bytes();
    }

    if (event_type == (int16_t)UserEventType::SET_BUFFER_LEN)
    {
        if (!manager->Require(4))
        {
            ret = ERROR_RTMP_MESSAGE_DECODE;
            rs_error("decode user control packet: read extra_data failed. ret=%d", ret);
            return ret;
        }
        extra_data = manager->Read4Bytes();
    }

    rs_trace("decode user control packet success. event_type=%d, event_data=%d, extra_data=%d", event_type, event_data, extra_data);

    return ret;
}

int UserControlPacket::GetPreferCID()
{
    return RTMP_CID_PROTOCOL_CONTROL;
}

int UserControlPacket::GetMessageType()
{
    return RTMP_MSG_USER_CONTROL_MESSAGE;
}

int UserControlPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(GetSize()))
    {
        ret = ERROR_RTMP_MESSAGE_ENCODE;
        rs_error("encode user control packet failed. ret=%d", ret);
        return ret;
    }

    manager->Write2Bytes(event_type);

    if (event_type == (int16_t)UserEventType::FMS_EVENT0)
    {
        manager->Write1Bytes(event_data);
    }
    else
    {
        manager->Write4Bytes(event_data);
    }

    if (event_type == (int16_t)UserEventType::SET_BUFFER_LEN)
    {
        manager->Write4Bytes(extra_data);
    }

    rs_trace("encode user control packet success");

    return ret;
}

int UserControlPacket::GetSize()
{
    int size = 0;

    size += 2;

    if (event_type == (int16_t)UserEventType::FMS_EVENT0)
    {
        size += 1;
    }
    else
    {
        size += 4;
    }

    if (event_type == (int16_t)UserEventType::SET_BUFFER_LEN)
    {
        size += 4;
    }

    return size;
}

OnStatusDataPacket::OnStatusDataPacket()
{
    command_name = RTMP_AMF0_COMMAND_ON_STATUS;
    data = AMF0Any::Object();
}

OnStatusDataPacket::~OnStatusDataPacket()
{
    rs_freep(data);
}

int OnStatusDataPacket::GetPreferCID()
{
    return RTMP_CID_OVER_STREAM;
}

int OnStatusDataPacket::GetMessageType()
{
    return RTMP_MSG_AMF0_DATA;
}

int OnStatusDataPacket::GetSize()
{
    return AMF0_LEN_STR(command_name) + AMF0_LEN_OBJECT(data);
}

int OnStatusDataPacket::Decode(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;
    return ret;
}

int OnStatusDataPacket::EncodePacket(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0WriteString(manager, command_name)) != ERROR_SUCCESS)
    {
        rs_error("encode on_status_data packet: amf0 write command failed. ret=%d", ret);
        return ret;
    }

    if ((ret = data->Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("encode  on_status_data packet: amf0 write data failed. ret=%d", ret);
        return ret;
    }
    return ret;
}
} // namespace rtmp