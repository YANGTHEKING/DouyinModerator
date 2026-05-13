#pragma once
#include <QObject>
#include <QMap>
#include <QList>

class EmojiResolver : public QObject {
    Q_OBJECT
public:
    explicit EmojiResolver(QObject* parent = nullptr);

    struct EmojiInfo {
        QString shortcode;   // e.g. "[微笑]"
        QString name;        // e.g. "微笑"
        QString douyinId;    // Douyin's internal emoji ID
    };

    QString resolve(const QString& text) const;
    QList<EmojiInfo> allEmojis() const;
    static QList<EmojiInfo> defaultEmojis();

private:
    void loadEmojiDatabase();
    QMap<QString, EmojiInfo> m_emojiMap;
};
