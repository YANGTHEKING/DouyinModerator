#include "ProtobufDecoder.h"
#include <zlib.h>
#include <cstring>
#include <QDebug>

ProtobufDecoder::ProtobufDecoder(QObject* parent) : QObject(parent) {}

void ProtobufDecoder::processFrame(const QByteArray& data) {
    static int frameCount = 0;
    frameCount++;
    if (frameCount <= 5 || frameCount % 50 == 0) {
        qDebug() << "[Decoder] Frame #" << frameCount << "size:" << data.size();
    }

    douyin::PushFrame frame;
    if (!frame.ParseFromArray(data.data(), data.size())) {
        qWarning() << "[Decoder] Failed to parse PushFrame, size:" << data.size()
                    << "first bytes:" << data.left(8).toHex();
        return;
    }

    std::string payload = frame.payload();
    if (payload.empty()) {
        // 心跳帧或 ACK 帧，没有 payload
        return;
    }

    QByteArray decompressed = gzipDecompress(QByteArray::fromStdString(payload));
    if (decompressed.isEmpty()) {
        qWarning() << "[Decoder] GZip decompression failed, payload size:" << payload.size()
                    << "first bytes:" << QByteArray(payload.data(), qMin(8, (int)payload.size())).toHex();
        return;
    }

    qDebug() << "[Decoder] Decompressed" << payload.size() << "->" << decompressed.size() << "bytes";

    douyin::Response response;
    if (!response.ParseFromArray(decompressed.data(), decompressed.size())) {
        qWarning() << "[Decoder] Failed to parse Response, decompressed size:" << decompressed.size();
        return;
    }

    qDebug() << "[Decoder] Response messages:" << response.messageslist_size();
    for (int i = 0; i < response.messageslist_size(); ++i) {
        const auto& msg = response.messageslist(i);
        qDebug() << "[Decoder]   msg" << i << "method:" << QString::fromStdString(msg.method());
        emit messageDecoded(msg);
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

    // 自动检测 gzip 或 zlib 格式
    if (inflateInit2(&stream, MAX_WBITS + 32) != Z_OK) {
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
        if (ret == Z_STREAM_END) {
            output.append(buffer, stream.total_out - output.size());
            break;
        }
        if (ret == Z_BUF_ERROR) {
            if (stream.avail_in == 0) break;
            continue;
        }
        if (ret != Z_OK) break;
        if (output.size() < static_cast<int>(stream.total_out)) {
            output.append(buffer, stream.total_out - output.size());
        }
    } while (true);

    inflateEnd(&stream);
    return (ret == Z_STREAM_END || ret == Z_OK) ? output : QByteArray();
}
