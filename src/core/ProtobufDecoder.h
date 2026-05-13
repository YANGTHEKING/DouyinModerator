#pragma once
#include <QObject>
#include <QByteArray>
#include "douyin.pb.h"

class ProtobufDecoder : public QObject {
    Q_OBJECT
public:
    explicit ProtobufDecoder(QObject* parent = nullptr);

    void processFrame(const QByteArray& data);

signals:
    void messageDecoded(const douyin::Message& msg);
    void ackFrameReady(const QByteArray& frame);

private:
    QByteArray gzipDecompress(const QByteArray& data);
};
