#include "EmojiResolver.h"
#include <QRegularExpression>

EmojiResolver::EmojiResolver(QObject* parent) : QObject(parent) {
    loadEmojiDatabase();
}

void EmojiResolver::loadEmojiDatabase() {
    for (const auto& info : defaultEmojis()) {
        m_emojiMap[info.shortcode] = info;
    }
}

QString EmojiResolver::resolve(const QString& text) const {
    QString result = text;
    QRegularExpression re("\\[([^\\]]+)\\]");
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        QString full = m.captured(0);
        QString name = m.captured(1);
        QString shortcode = "[" + name + "]";
        if (m_emojiMap.contains(shortcode)) {
            // Keep the shortcode format for Douyin API - it accepts [emoji_name]
            // The actual resolution happens server-side
        }
    }
    return result;
}

QList<EmojiResolver::EmojiInfo> EmojiResolver::allEmojis() const {
    return m_emojiMap.values();
}

QList<EmojiResolver::EmojiInfo> EmojiResolver::defaultEmojis() {
    return {
        {"[微笑]", "微笑", "SMILE"},
        {"[色]", "色", "SEXY"},
        {"[发呆]", "发呆", "DAZE"},
        {"[流泪]", "流泪", "CRY"},
        {"[害羞]", "害羞", "SHY"},
        {"[闭嘴]", "闭嘴", "SHUTUP"},
        {"[睡]", "睡", "SLEEP"},
        {"[大哭]", "大哭", "SOB"},
        {"[尴尬]", "尴尬", "EMBARRASS"},
        {"[发怒]", "发怒", "ANGER"},
        {"[调皮]", "调皮", "NAUGHTY"},
        {"[呲牙]", "呲牙", "GRIN"},
        {"[惊讶]", "惊讶", "SURPRISE"},
        {"[难过]", "难过", "SAD"},
        {"[酷]", "酷", "COOL"},
        {"[抓狂]", "抓狂", "CRAZY"},
        {"[吐]", "吐", "PUKE"},
        {"[偷笑]", "偷笑", "GIGGLE"},
        {"[愉快]", "愉快", "HAPPY"},
        {"[白眼]", "白眼", "ROLLEYES"},
        {"[傲慢]", "傲慢", "PROUD"},
        {"[饥饿]", "饥饿", "HUNGRY"},
        {"[困]", "困", "SLEEPY"},
        {"[惊恐]", "惊恐", "TERROR"},
        {"[流汗]", "流汗", "SWEAT"},
        {"[憨笑]", "憨笑", "SIMPER"},
        {"[悠闲]", "悠闲", "LEISURE"},
        {"[奋斗]", "奋斗", "STRIVE"},
        {"[咒骂]", "咒骂", "CURSE"},
        {"[疑问]", "疑问", "QUESTION"},
        {"[嘘]", "嘘", "HUSH"},
        {"[晕]", "晕", "DIZZY"},
        {"[疯了]", "疯了", "MAD"},
        {"[衰]", "衰", "UNLUCKY"},
        {"[骷髅]", "骷髅", "SKULL"},
        {"[敲打]", "敲打", "HIT"},
        {"[再见]", "再见", "BYE"},
        {"[擦汗]", "擦汗", "WIPE"},
        {"[抠鼻]", "抠鼻", "PICKNOSE"},
        {"[鼓掌]", "鼓掌", "CLAP"},
        {"[坏笑]", "坏笑", "EVIL"},
        {"[右哼哼]", "右哼哼", "RIGHTHENG"},
        {"[鄙视]", "鄙视", "DESPISE"},
        {"[委屈]", "委屈", "WRONGED"},
        {"[快哭了]", "快哭了", "CRYALITTLE"},
        {"[阴险]", "阴险", "CUNNING"},
        {"[亲亲]", "亲亲", "KISS"},
        {"[可怜]", "可怜", "POOR"},
        {"[666]", "666", "666"},
        {"[比心]", "比心", "BIXIN"},
        {"[玫瑰]", "玫瑰", "ROSE"},
        {"[礼物]", "礼物", "GIFT"},
        {"[红包]", "红包", "REDBAG"},
        {"[干杯]", "干杯", "CHEERS"},
        {"[加油]", "加油", "JIAYOU"},
        {"[棒]", "棒", "GOOD"},
        {"[耶]", "耶", "YEAH"},
    };
}
