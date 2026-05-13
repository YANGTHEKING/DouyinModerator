#include "ProtobufDecoder.h"
#include <zlib.h>
#include <cstring>
#include <QDebug>

ProtobufDecoder::ProtobufDecoder(QObject* parent) : QObject(parent) {}

void ProtobufDecoder::processFrame(const QByteArray& data) {
    douyin::PushFrame frame;
    if (!frame.ParseFromArray(data.data(), data.size())) {
        qWarning() << "Failed to parse PushFrame";
        return;
    }

    QByteArray decompressed = gzipDecompress(QByteArray::fromStdString(frame.payload()));
    if (decompressed.isEmpty()) {
        qWarning() << "GZip decompression failed";
        return;
    }

    douyin::Response response;
    if (!response.ParseFromArray(decompressed.data(), decompressed.size())) {
        qWarning() << "Failed to parse Response";
        return;
    }

    for (int i = 0; i < response.messageslist_size(); ++i) {
        emit messageDecoded(response.messageslist(i));
    }

    if (response.needack()) {
        douyin::PushFrame ack;
        ack.set_logid(frame.logid());
        ack.set_payloadtype("ack");
        ack.set_responseidstr(response.internalext());

        QByteArray ackData(ack.ByteSizeLong(), 0);
        (void)ack.SerializeToArray(ackData.data(), ackData.size());
        emit ackFrameReady(ackData);
    }
}

QByteArray ProtobufDecoder::gzipDecompress(const QByteArray& compressed) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) {
        return {};
    }

    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
    stream.avail_in = compressed.size();

    QByteArray output;
    char buffer[32768];

    int ret;
    do {
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        stream.avail_out = sizeof(buffer);
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_BUF_ERROR) break;
        if (output.size() < static_cast<int>(stream.total_out)) {
            output.append(buffer, stream.total_out - output.size());
        }
    } while (ret == Z_OK);

    inflateEnd(&stream);
    return (ret == Z_STREAM_END || ret == Z_OK) ? output : QByteArray();
}
