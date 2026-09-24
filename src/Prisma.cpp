#include "Prisma.h"

#include "Configuration.h"
#include "PrismaUI_API.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {
    PRISMA_UI_API::IVPrismaUI1* prismaUI = nullptr;
    PrismaView view = 0;
    bool domReady = false;
    int currentDirection = 0;
    int currentRawDirection = 0;
    bool runtimeEligible = false;
    bool runtimePreviewEligible = false;
    bool runtimeAttached = false;
    float runtimeXPercent = 50.0f;
    float runtimeYPercent = 50.0f;
    float runtimeResolutionScale = 1.0f;
    std::vector<AttackWarningVisual> attackWarnings;

    void SyncVisibility();

    bool IsViewValid() {
        return prismaUI && view && prismaUI->IsValid(view);
    }

    void WriteColor(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::array<float, 4>& color) {
        writer.StartArray();
        for (const float component : color) {
            writer.Double(component);
        }
        writer.EndArray();
    }

    int ResolvePrincipalDirection(
        int direction,
        const std::array<bool, Settings::kDirectionCount>& mergeWithPrevious,
        bool mergeDirection8With1) {
        direction = std::clamp(direction, 0, 8);
        if (direction == 0) return 0;
        if (direction == 8 && mergeDirection8With1) return 1;
        std::size_t index = static_cast<std::size_t>(direction - 1);
        while (index > 0 && mergeWithPrevious[index]) --index;
        return static_cast<int>(index + 1);
    }

    int ResolvePlayerDirection(int direction) {
        return ResolvePrincipalDirection(
            direction, Settings::PlayerUI.mergeWithPrevious, Settings::PlayerUI.mergeDirection8With1);
    }

    std::string BuildSettingsPayload() {
        const auto& ui = Settings::PlayerUI;
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        writer.Key("enabled"); writer.Bool(ui.enabled);
        writer.Key("editMode"); writer.Bool(ui.editMode);
        writer.Key("showCenter"); writer.Bool(ui.showCenter);
        writer.Key("moveCenterWithDirection"); writer.Bool(ui.moveCenterWithDirection);
        writer.Key("centerInThirdPerson"); writer.Bool(ui.centerInThirdPerson);
        writer.Key("scaleWithResolution"); writer.Bool(ui.scaleWithResolution);
        writer.Key("positionXPercent"); writer.Int(ui.positionXPercent);
        writer.Key("positionYPercent"); writer.Int(ui.positionYPercent);
        writer.Key("attachOffsetXPixels"); writer.Int(ui.attachOffsetXPixels);
        writer.Key("attachOffsetYPixels"); writer.Int(ui.attachOffsetYPixels);
        writer.Key("scalePercent"); writer.Int(ui.scalePercent);
        writer.Key("diameterPixels"); writer.Int(ui.diameterPixels);
        writer.Key("innerDiameterPercent"); writer.Int(ui.innerDiameterPercent);
        writer.Key("centerPieceSizePercent"); writer.Int(ui.centerPieceSizePercent);
        writer.Key("segmentGapPixels"); writer.Int(ui.segmentGapPixels);
        writer.Key("rotationDegrees"); writer.Int(ui.rotationDegrees);
        writer.Key("opacityPercent"); writer.Int(ui.opacityPercent);
        writer.Key("centerMovementPixels"); writer.Int(ui.centerMovementPixels);
        writer.Key("centerMovementDurationMs"); writer.Int(ui.centerMovementDurationMs);
        writer.Key("activeColor"); WriteColor(writer, ui.activeColor);
        writer.Key("inactiveColor"); WriteColor(writer, ui.inactiveColor);
        writer.Key("borderColor"); WriteColor(writer, ui.borderColor);
        writer.Key("centerColor"); WriteColor(writer, ui.centerColor);
        writer.Key("mergeWithPrevious");
        writer.StartArray();
        for (const bool merge : ui.mergeWithPrevious) writer.Bool(merge);
        writer.EndArray();
        writer.Key("mergeDirection8With1"); writer.Bool(ui.mergeDirection8With1);
        writer.Key("sharedSegmentImage"); writer.String(ui.sharedSegmentImage.c_str());
        writer.Key("segmentImages");
        writer.StartArray();
        for (const auto& path : ui.segmentImages) writer.String(path.c_str());
        writer.EndArray();
        writer.Key("centerImage"); writer.String(ui.centerImage.c_str());
        const auto& npc = Settings::NPCUI;
        writer.Key("npc");
        writer.StartObject();
        writer.Key("enabled"); writer.Bool(npc.enabled);
        writer.Key("scalePercent"); writer.Int(npc.scalePercent);
        writer.Key("opacityPercent"); writer.Int(npc.opacityPercent);
        writer.Key("offsetXPixels"); writer.Int(npc.offsetXPixels);
        writer.Key("offsetYPixels"); writer.Int(npc.offsetYPixels);
        writer.Key("rotationDegrees"); writer.Int(npc.rotationDegrees);
        writer.Key("diameterPixels"); writer.Int(npc.diameterPixels);
        writer.Key("innerDiameterPercent"); writer.Int(npc.innerDiameterPercent);
        writer.Key("segmentGapPixels"); writer.Int(npc.segmentGapPixels);
        writer.Key("activeColor"); WriteColor(writer, npc.activeColor);
        writer.Key("inactiveColor"); WriteColor(writer, npc.inactiveColor);
        writer.Key("borderColor"); WriteColor(writer, npc.borderColor);
        writer.Key("mergeWithPrevious");
        writer.StartArray();
        for (const bool merge : npc.mergeWithPrevious) writer.Bool(merge);
        writer.EndArray();
        writer.Key("mergeDirection8With1"); writer.Bool(npc.mergeDirection8With1);
        writer.Key("sharedSegmentImage"); writer.String(npc.sharedSegmentImage.c_str());
        writer.Key("segmentImages");
        writer.StartArray();
        for (const auto& path : npc.segmentImages) writer.String(path.c_str());
        writer.EndArray();
        writer.EndObject();
        writer.EndObject();
        return buffer.GetString();
    }

    void SendSettings() {
        if (!domReady || !IsViewValid()) return;
        static std::string payload;
        payload = BuildSettingsPayload();
        prismaUI->InteropCall(view, "updateHonorCombatSettings", payload.c_str());
    }

    void SendDirection() {
        if (!domReady || !IsViewValid()) return;
        static std::string payload;
        payload = std::to_string(currentDirection);
        prismaUI->InteropCall(view, "updateHonorCombatDirection", payload.c_str());
    }

    void SendRuntimeState() {
        if (!domReady || !IsViewValid()) return;
        char payload[128]{};
        std::snprintf(
            payload,
            sizeof(payload),
            "%d|%d|%.3f|%.3f|%.3f|%d",
            runtimeEligible ? 1 : 0,
            runtimePreviewEligible ? 1 : 0,
            runtimeXPercent,
            runtimeYPercent,
            runtimeResolutionScale,
            runtimeAttached ? 1 : 0);
        prismaUI->InteropCall(view, "updateHonorCombatRuntime", payload);
    }

    void SendAttackWarnings() {
        if (!domReady || !IsViewValid()) return;
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartArray();
        for (const auto& warning : attackWarnings) {
            writer.StartObject();
            writer.Key("id"); writer.Uint(warning.id);
            writer.Key("direction"); writer.Int(warning.direction);
            writer.Key("xPercent"); writer.Double(warning.xPercent);
            writer.Key("yPercent"); writer.Double(warning.yPercent);
            writer.Key("resolutionScale"); writer.Double(warning.resolutionScale);
            writer.Key("distanceScale"); writer.Double(warning.distanceScale);
            writer.EndObject();
        }
        writer.EndArray();
        prismaUI->InteropCall(view, "updateHonorCombatAttackWarnings", buffer.GetString());
    }

    void SendAllState() {
        SendSettings();
        SendDirection();
        SendRuntimeState();
        SendAttackWarnings();
    }

    bool EnsureView() {
        if (!prismaUI) return false;
        if (IsViewValid()) return true;

        view = 0;
        domReady = false;
#ifdef DEV_SERVER
        constexpr const char* path = "http://localhost:5173";
#else
        constexpr const char* path = PRODUCT_NAME "/index.html";
#endif
        view = prismaUI->CreateView(path, [](PrismaView readyView) {
            if (!prismaUI || !view || readyView != view) return;
            domReady = true;
            SendAllState();
            SyncVisibility();
        });
        if (!view) {
            logger::error("Honor Combat could not create its PrismaUI view.");
            return false;
        }
        prismaUI->RegisterJSListener(view, "hideHonorCombat", [](const char*) { Prisma::Hide(); });
        return true;
    }

    void SyncVisibility() {
        const bool shouldShow =
            (Settings::PlayerUI.enabled && (runtimeEligible || (Settings::PlayerUI.editMode && runtimePreviewEligible))) ||
            (Settings::NPCUI.enabled && !attackWarnings.empty());
        const bool viewValid = IsViewValid();
        const bool hidden = viewValid ? prismaUI->IsHidden(view) : true;
        if (!shouldShow) {
            if (viewValid && !hidden) {
                prismaUI->Hide(view);
            }
            return;
        }
        if (!EnsureView() || !domReady) return;
        if (prismaUI->IsHidden(view)) {
            prismaUI->Show(view);
        }
    }
}

void Prisma::Install() {
    prismaUI = reinterpret_cast<PRISMA_UI_API::IVPrismaUI1*>(PRISMA_UI_API::RequestPluginAPI());
    if (!prismaUI) {
        logger::error("PrismaUI.dll was not found; Honor Combat HUD is unavailable.");
    } else {
        logger::info("Honor Combat linked to PrismaUI!");
    }
}

void Prisma::Preload() {
    if (!EnsureView()) return;
    prismaUI->Hide(view);
    SendAllState();
    SyncVisibility();
}

void Prisma::Show() {
    if (EnsureView()) prismaUI->Show(view);
}

void Prisma::Hide() {
    if (IsViewValid()) prismaUI->Hide(view);
}

bool Prisma::IsHidden() {
    return !IsViewValid() || prismaUI->IsHidden(view);
}

bool Prisma::IsReady() {
    return domReady && IsViewValid();
}

void Prisma::ApplyUISettings() {
    if (!prismaUI) return;
    if (Settings::PlayerUI.enabled && (runtimeEligible || (Settings::PlayerUI.editMode && runtimePreviewEligible))) EnsureView();
    SendSettings();
    const int remappedDirection = ResolvePlayerDirection(currentRawDirection);
    if (currentDirection != remappedDirection) {
        currentDirection = remappedDirection;
        SendDirection();
    }
    SyncVisibility();
}

void Prisma::UpdateDirection(int direction) {
    direction = std::clamp(direction, 0, 8);
    currentRawDirection = direction;
    const int remappedDirection = ResolvePlayerDirection(direction);
    if (currentDirection != remappedDirection) {
        currentDirection = remappedDirection;
        SendDirection();
    }
    SyncVisibility();
}

void Prisma::UpdateRuntimeState(
    bool eligible,
    bool previewEligible,
    float xPercent,
    float yPercent,
    float resolutionScale,
    bool attached) {
    xPercent = std::clamp(xPercent, 0.0f, 100.0f);
    yPercent = std::clamp(yPercent, 0.0f, 100.0f);
    resolutionScale = std::clamp(resolutionScale, 0.5f, 4.0f);
    const bool changed = runtimeEligible != eligible || runtimePreviewEligible != previewEligible ||
                         runtimeAttached != attached ||
                         std::abs(runtimeXPercent - xPercent) >= 0.025f ||
                         std::abs(runtimeYPercent - yPercent) >= 0.025f ||
                         std::abs(runtimeResolutionScale - resolutionScale) >= 0.005f;
    runtimeEligible = eligible;
    runtimePreviewEligible = previewEligible;
    runtimeAttached = attached;
    runtimeXPercent = xPercent;
    runtimeYPercent = yPercent;
    runtimeResolutionScale = resolutionScale;
    if (changed) SendRuntimeState();
    SyncVisibility();
}

void Prisma::UpdateAttackWarnings(const std::vector<AttackWarningVisual>& warnings) {
    auto remappedWarnings = warnings;
    for (auto& warning : remappedWarnings) {
        warning.direction = ResolvePrincipalDirection(
            warning.direction,
            Settings::NPCUI.mergeWithPrevious,
            Settings::NPCUI.mergeDirection8With1);
    }
    const auto nearlyEqual = [](float left, float right, float threshold) {
        return std::abs(left - right) < threshold;
    };
    const bool changed = attackWarnings.size() != remappedWarnings.size() ||
                         !std::equal(
                             attackWarnings.begin(), attackWarnings.end(), remappedWarnings.begin(), remappedWarnings.end(),
                             [&](const AttackWarningVisual& left, const AttackWarningVisual& right) {
                                 return left.id == right.id && left.direction == right.direction &&
                                        nearlyEqual(left.xPercent, right.xPercent, 0.025f) &&
                                        nearlyEqual(left.yPercent, right.yPercent, 0.025f) &&
                                        nearlyEqual(left.resolutionScale, right.resolutionScale, 0.005f) &&
                                        nearlyEqual(left.distanceScale, right.distanceScale, 0.005f);
                             });
    if (!changed) return;
    attackWarnings = std::move(remappedWarnings);
    SendAttackWarnings();
    SyncVisibility();
}

void Prisma::Reset() {
    currentRawDirection = 0;
    currentDirection = 0;
    runtimeEligible = false;
    runtimePreviewEligible = false;
    runtimeAttached = false;
    runtimeXPercent = 50.0f;
    runtimeYPercent = 50.0f;
    runtimeResolutionScale = 1.0f;
    attackWarnings.clear();
    SendDirection();
    SendRuntimeState();
    SendAttackWarnings();
    SyncVisibility();
}
