#pragma once

#include "SKSEMCP/SKSEMenuFramework.hpp"

#include <array>
#include <string>

namespace Settings {
    inline constexpr std::size_t kDirectionCount = 8;

    struct UISettings {
        bool enabled = true;
        bool editMode = false;
        bool blockReset = false;
        bool useDirectionalDMK = false;
        bool requireTDMTargetLock = false;
        bool showCenter = true;
        bool moveCenterWithDirection = false;
        bool centerInThirdPerson = false;
        bool scaleWithResolution = true;
        int positionXPercent = 50;
        int positionYPercent = 50;
        int attachOffsetXPixels = 0;
        int attachOffsetYPixels = 0;
        int scalePercent = 100;
        int diameterPixels = 260;
        int innerDiameterPercent = 36;
        int centerPieceSizePercent = 100;
        int segmentGapPixels = 4;
        int rotationDegrees = 0;
        int opacityPercent = 100;
        int centerMovementPixels = 20;
        int centerMovementDurationMs = 100;
        std::array<float, 4> activeColor{ 0.95f, 0.72f, 0.22f, 1.0f };
        std::array<float, 4> inactiveColor{ 0.08f, 0.09f, 0.10f, 0.72f };
        std::array<float, 4> borderColor{ 0.80f, 0.80f, 0.76f, 0.90f };
        std::array<float, 4> centerColor{ 0.04f, 0.04f, 0.05f, 0.86f };
        std::array<bool, kDirectionCount> mergeWithPrevious{};
        bool mergeDirection8With1 = false;
        std::string sharedSegmentImage;
        std::array<std::string, kDirectionCount> segmentImages{};
        std::string centerImage;
    };

    inline UISettings UI;
}

namespace ModMenu {
    void Register();
    void UIRender();
    void LoadSettings();
    void SaveSettings();
    void LoadLanguage();
    const char* GetLoc(const std::string& key, const char* defaultValue);
}
