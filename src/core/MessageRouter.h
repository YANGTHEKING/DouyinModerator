#pragma once
#include <QObject>
#include "douyin.pb.h"

class MessageRouter : public QObject {
    Q_OBJECT
public:
    explicit MessageRouter(QObject* parent = nullptr);

    void route(const douyin::Message& msg);

signals:
    void chatMessage(const QString& userId, const QString& userName, const QString& content);
    void memberMessage(const QString& userId, const QString& userName);
    void giftMessage(const QString& userId, const QString& userName,
                     const QString& giftName, int64_t diamondCount, int64_t count);
    void likeMessage(const QString& userId, const QString& userName, int64_t count);
    void socialMessage(const QString& userId, const QString& userName);
    void fansclubMessage(const QString& userId, const QString& userName, int32_t level);
    void controlMessage(int32_t action);
    void unknownMessage(const QString& method);

private:
    QString userName(const douyin::User& user) const;
};
