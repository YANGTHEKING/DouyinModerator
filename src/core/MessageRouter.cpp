#include "MessageRouter.h"

MessageRouter::MessageRouter(QObject* parent) : QObject(parent) {}

QString MessageRouter::userName(const douyin::User& user) const {
    if (!user.nickname().empty()) return QString::fromStdString(user.nickname());
    return QString::number(user.id());
}

void MessageRouter::route(const douyin::Message& msg) {
    std::string method = msg.method();
    QByteArray payload(msg.payload().data(), msg.payload().size());

    if (method == "WebcastChatMessage") {
        douyin::ChatMessage cm;
        if (cm.ParseFromArray(payload.data(), payload.size())) {
            emit chatMessage(
                QString::number(cm.user().id()),
                userName(cm.user()),
                QString::fromStdString(cm.content()));
        }
    } else if (method == "WebcastMemberMessage") {
        douyin::MemberMessage mm;
        if (mm.ParseFromArray(payload.data(), payload.size())) {
            emit memberMessage(
                QString::number(mm.user().id()),
                userName(mm.user()));
        }
    } else if (method == "WebcastGiftMessage") {
        douyin::GiftMessage gm;
        if (gm.ParseFromArray(payload.data(), payload.size())) {
            QString giftName;
            if (gm.has_gift() && !gm.gift().name().empty()) {
                giftName = QString::fromStdString(gm.gift().name());
            } else if (!gm.giftname().empty()) {
                giftName = QString::fromStdString(gm.giftname());
            }
            int64_t diamonds = gm.has_gift() ? gm.gift().diamondcount() : gm.diamondcount();
            int64_t repeatCount = gm.repeatcount();
            emit giftMessage(
                QString::number(gm.user().id()),
                userName(gm.user()),
                giftName, diamonds,
                repeatCount > 0 ? repeatCount : 1);
        }
    } else if (method == "WebcastLikeMessage") {
        douyin::LikeMessage lm;
        if (lm.ParseFromArray(payload.data(), payload.size())) {
            emit likeMessage(
                QString::number(lm.user().id()),
                userName(lm.user()),
                lm.count());
        }
    } else if (method == "WebcastSocialMessage") {
        douyin::SocialMessage sm;
        if (sm.ParseFromArray(payload.data(), payload.size())) {
            emit socialMessage(
                QString::number(sm.user().id()),
                userName(sm.user()));
        }
    } else if (method == "WebcastFansclubMessage") {
        douyin::FansclubMessage fm;
        if (fm.ParseFromArray(payload.data(), payload.size())) {
            emit fansclubMessage(
                QString::number(fm.user().id()),
                userName(fm.user()),
                fm.level());
        }
    } else if (method == "WebcastControlMessage") {
        douyin::ControlMessage cm;
        if (cm.ParseFromArray(payload.data(), payload.size())) {
            emit controlMessage(cm.action());
        }
    } else {
        emit unknownMessage(QString::fromStdString(method));
    }
}
