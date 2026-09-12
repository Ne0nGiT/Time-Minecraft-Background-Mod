#include <Geode/Bindings.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/loader/Mod.hpp>

#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>

using namespace geode::prelude;

namespace {

// Tra ve gio hien tai theo may (0-23), local time.
int getCurrentHour() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return local.tm_hour;
}

// Tra ve chuoi "HH:MM:SS" cua thoi gian hien tai.
std::string getTimeString() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);
    return std::string(buf);
}

// Tao sprite background full-man-hinh cho 1 gio cu the, doc file
// bg_<hour>.png tu thu muc resources cua mod (Mod::get()->getResourcesDir()).
// File nay PHAI ton tai trong resources/backgrounds/, neu khong se log
// warning va tra ve nullptr (khong crash game).
CCSprite* createHourBackground(int hour) {
    auto fileName = "bg_" + std::to_string(hour) + ".png";
    auto path = (Mod::get()->getResourcesDir() / fileName).string();

    auto bg = CCSprite::create(path.c_str());
    if (!bg) {
        log::warn("Time Theme: khong tim thay background '{}'", fileName);
        return nullptr;
    }

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    bg->setPosition(winSize.width / 2.f, winSize.height / 2.f);
    bg->setScaleX(winSize.width / bg->getContentSize().width);
    bg->setScaleY(winSize.height / bg->getContentSize().height);
    bg->setID("time-theme-background");
    return bg;
}

// Quet toan bo resources/icons/ , voi moi cap file .plist + .png cung ten,
// nap vao CCSpriteFrameCache dung chung cua game. Neu cac frame name trong
// .plist trung voi frame name goc cua GD (vd "player_01_001.png",
// "bird_01_001.png", ...) thi hinh custom se tu dong thay the icon goc
// o moi noi game su dung frame do - day chinh la co che cac mod
// "custom icon / texture pack" dang dung.
void loadCustomIconPacks() {
    std::error_code ec;
    auto iconsDir = Mod::get()->getResourcesDir() / "icons";
    if (!std::filesystem::exists(iconsDir, ec)) return;

    for (auto const& entry : std::filesystem::directory_iterator(iconsDir, ec)) {
        if (entry.path().extension() != ".plist") continue;

        auto png = entry.path();
        png.replace_extension(".png");
        if (!std::filesystem::exists(png, ec)) continue;

        CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(
            entry.path().string().c_str(),
            png.string().c_str()
        );
        log::info("Time Theme: da nap icon pack '{}'", entry.path().filename().string());
    }
}

} // namespace

// $execute chay 1 lan duy nhat, ngay khi mod duoc load - truoc khi bat ky
// layer nao cua GD duoc tao. Dung de nap cac custom icon pack tu dau.
$execute {
    loadCustomIconPacks();
}

// ============================================================
// MAIN MENU: background theo gio + dong ho
// ============================================================
class $modify(TimeThemeMenuLayer, MenuLayer) {
    struct Fields {
        CCSprite* bgSprite = nullptr;
        CCLabelBMFont* clockLabel = nullptr;
        int lastHour = -1;
    };

    bool init() {
        if (!MenuLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        int hour = getCurrentHour();

        // 1) Background tu dong doi theo gio thuc
        m_fields->bgSprite = createHourBackground(hour);
        if (m_fields->bgSprite) {
            this->addChild(m_fields->bgSprite, -100);
        }
        m_fields->lastHour = hour;

        // 2) Dong ho thoi gian thuc, goc tren-phai man hinh
        m_fields->clockLabel = CCLabelBMFont::create(getTimeString().c_str(), "bigFont.fnt");
        m_fields->clockLabel->setScale(0.4f);
        m_fields->clockLabel->setAnchorPoint({1.f, 1.f});
        m_fields->clockLabel->setPosition(winSize.width - 8.f, winSize.height - 8.f);
        this->addChild(m_fields->clockLabel, 100);

        // Cap nhat moi giay: doi text dong ho + kiem tra doi gio -> doi bg
        this->schedule(schedule_selector(TimeThemeMenuLayer::onSecondTick), 1.0f);

        return true;
    }

    void onSecondTick(float) {
        if (m_fields->clockLabel) {
            m_fields->clockLabel->setString(getTimeString().c_str());
        }

        int hour = getCurrentHour();
        if (hour != m_fields->lastHour) {
            if (m_fields->bgSprite) {
                m_fields->bgSprite->removeFromParent();
                m_fields->bgSprite = nullptr;
            }
            m_fields->bgSprite = createHourBackground(hour);
            if (m_fields->bgSprite) {
                this->addChild(m_fields->bgSprite, -100);
            }
            m_fields->lastHour = hour;
        }
    }
};

// ============================================================
// TRONG MAN CHOI (PlayLayer): background theo gio (mo hon, khong
// che gameplay) + dong ho, khong co nut Discord de tranh vuong
// ============================================================
class $modify(TimeThemePlayLayer, PlayLayer) {
    struct Fields {
        CCSprite* bgSprite = nullptr;
        CCLabelBMFont* clockLabel = nullptr;
        int lastHour = -1;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        int hour = getCurrentHour();

        m_fields->bgSprite = createHourBackground(hour);
        if (m_fields->bgSprite) {
            m_fields->bgSprite->setOpacity(120); // mo bot de khong lan gameplay
            this->addChild(m_fields->bgSprite, -100);
        }
        m_fields->lastHour = hour;

        m_fields->clockLabel = CCLabelBMFont::create(getTimeString().c_str(), "bigFont.fnt");
        m_fields->clockLabel->setScale(0.4f);
        m_fields->clockLabel->setAnchorPoint({1.f, 1.f});
        m_fields->clockLabel->setPosition(winSize.width - 8.f, winSize.height - 8.f);
        this->addChild(m_fields->clockLabel, 1000);

        this->schedule(schedule_selector(TimeThemePlayLayer::onSecondTick), 1.0f);

        return true;
    }

    void onSecondTick(float) {
        if (m_fields->clockLabel) {
            m_fields->clockLabel->setString(getTimeString().c_str());
        }

        int hour = getCurrentHour();
        if (hour != m_fields->lastHour) {
            if (m_fields->bgSprite) {
                m_fields->bgSprite->removeFromParent();
                m_fields->bgSprite = nullptr;
            }
            m_fields->bgSprite = createHourBackground(hour);
            if (m_fields->bgSprite) {
                m_fields->bgSprite->setOpacity(120);
                this->addChild(m_fields->bgSprite, -100);
            }
            m_fields->lastHour = hour;
        }
    }
};
