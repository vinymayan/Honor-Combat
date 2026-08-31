#include "Configuration.h"

#include "Prisma.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace {
    constexpr const char* kModDirectory = "Data/Viny Mods/Honor Combat";
    constexpr const char* kUISettingsPath = "Data/Viny Mods/Honor Combat/UISettings.json";
    constexpr const char* kLanguagePath = "Data/Viny Mods/Honor Combat/Language.json";
    constexpr std::size_t kMaxAssetPathLength = 260;

    std::unordered_map<std::string, std::string> language;

    void ClampColor(std::array<float, 4>& color) {
        for (auto& value : color) {
            value = std::clamp(value, 0.0f, 1.0f);
        }
    }

    std::string NormalizeAssetPath(std::string path) {
        std::replace(path.begin(), path.end(), '\\', '/');
        if (path.size() > kMaxAssetPathLength || path.find("..") != std::string::npos ||
            path.find(':') != std::string::npos || path.starts_with('/') || path.starts_with("//")) {
            return {};
        }
        return path;
    }

    void ClampSettings(Settings::UISettings& ui) {
        ui.mergeWithPrevious[0] = false;
        if (ui.mergeDirection8With1) ui.mergeWithPrevious[7] = false;
        ui.positionXPercent = std::clamp(ui.positionXPercent, 0, 100);
        ui.positionYPercent = std::clamp(ui.positionYPercent, 0, 100);
        ui.attachOffsetXPixels = std::clamp(ui.attachOffsetXPixels, -1000, 1000);
        ui.attachOffsetYPixels = std::clamp(ui.attachOffsetYPixels, -1000, 1000);
        ui.scalePercent = std::clamp(ui.scalePercent, 25, 300);
        ui.diameterPixels = std::clamp(ui.diameterPixels, 96, 800);
        ui.innerDiameterPercent = std::clamp(ui.innerDiameterPercent, 10, 80);
        ui.centerPieceSizePercent = std::clamp(ui.centerPieceSizePercent, 10, 200);
        ui.segmentGapPixels = std::clamp(ui.segmentGapPixels, 0, 30);
        ui.rotationDegrees = std::clamp(ui.rotationDegrees, -180, 180);
        ui.opacityPercent = std::clamp(ui.opacityPercent, 10, 100);
        ui.centerMovementPixels = std::clamp(ui.centerMovementPixels, 0, 400);
        ui.centerMovementDurationMs = std::clamp(ui.centerMovementDurationMs, 0, 1000);
        ClampColor(ui.activeColor);
        ClampColor(ui.inactiveColor);
        ClampColor(ui.borderColor);
        ClampColor(ui.centerColor);
        ui.sharedSegmentImage = NormalizeAssetPath(std::move(ui.sharedSegmentImage));
        for (auto& path : ui.segmentImages) {
            path = NormalizeAssetPath(std::move(path));
        }
        ui.centerImage = NormalizeAssetPath(std::move(ui.centerImage));
    }

    bool RenderIntSliderWithInput(const char* label, int* value, int minimum, int maximum) {
        bool changed = false;
        ImGui::PushID(label);
        ImGui::SetNextItemWidth(200.0f);
        changed |= ImGui::SliderInt("##slider", value, minimum, maximum);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        changed |= ImGui::InputInt(label, value);
        const int clamped = std::clamp(*value, minimum, maximum);
        if (*value != clamped) {
            *value = clamped;
            changed = true;
        }
        ImGui::PopID();
        return changed;
    }

    bool ReadDocument(rapidjson::Document& document) {
        std::ifstream file(kUISettingsPath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        std::stringstream contents;
        contents << file.rdbuf();
        auto json = contents.str();
        if (json.size() >= 3 && static_cast<unsigned char>(json[0]) == 0xEF &&
            static_cast<unsigned char>(json[1]) == 0xBB && static_cast<unsigned char>(json[2]) == 0xBF) {
            json.erase(0, 3);
        }
        document.Parse(json.c_str());
        return !document.HasParseError() && document.IsObject();
    }

    void ReadColor(const rapidjson::Value& parent, const char* key, std::array<float, 4>& color) {
        if (!parent.HasMember(key) || !parent[key].IsArray() || parent[key].Size() < 3) {
            return;
        }
        const auto& array = parent[key];
        for (rapidjson::SizeType i = 0; i < 4 && i < array.Size(); ++i) {
            if (array[i].IsNumber()) {
                color[i] = array[i].GetFloat();
            }
        }
    }

    void WriteColor(
        rapidjson::Value& parent,
        const char* key,
        const std::array<float, 4>& color,
        rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value array(rapidjson::kArrayType);
        for (const float value : color) {
            array.PushBack(value, allocator);
        }
        parent.AddMember(rapidjson::Value(key, allocator).Move(), array, allocator);
    }
}

namespace ModMenu {
    void LoadLanguage() {
        language.clear();
        std::ifstream file(kLanguagePath, std::ios::binary);
        if (!file.is_open()) {
            return;
        }

        std::stringstream contents;
        contents << file.rdbuf();
        rapidjson::Document document;
        document.Parse(contents.str().c_str());
        if (document.HasParseError() || !document.IsObject()) {
            logger::warn("Honor Combat language file could not be parsed.");
            return;
        }

        for (auto member = document.MemberBegin(); member != document.MemberEnd(); ++member) {
            if (member->value.IsString()) {
                language[member->name.GetString()] = member->value.GetString();
            } else if (member->value.IsObject()) {
                const std::string category = member->name.GetString();
                for (auto nested = member->value.MemberBegin(); nested != member->value.MemberEnd(); ++nested) {
                    if (nested->value.IsString()) {
                        language[category + "." + nested->name.GetString()] = nested->value.GetString();
                    }
                }
            }
        }
    }

    const char* GetLoc(const std::string& key, const char* defaultValue) {
        const auto found = language.find(key);
        return found == language.end() ? defaultValue : found->second.c_str();
    }

    void LoadSettings() {
        rapidjson::Document document;
        if (!ReadDocument(document)) {
            ClampSettings(Settings::UI);
            return;
        }

        auto& ui = Settings::UI;
        const auto readBool = [&](const char* key, bool& value) {
            if (document.HasMember(key) && document[key].IsBool()) value = document[key].GetBool();
        };
        const auto readInt = [&](const char* key, int& value) {
            if (document.HasMember(key) && document[key].IsInt()) value = document[key].GetInt();
        };
        const auto readString = [&](const rapidjson::Value& parent, const char* key, std::string& value) {
            if (parent.HasMember(key) && parent[key].IsString()) value = parent[key].GetString();
        };

        readBool("enabled", ui.enabled);
        readBool("editMode", ui.editMode);
        readBool("blockReset", ui.blockReset);
        readBool("useDirectionalDMK", ui.useDirectionalDMK);
        readBool("requireTDMTargetLock", ui.requireTDMTargetLock);
        readBool("showCenter", ui.showCenter);
        readBool("moveCenterWithDirection", ui.moveCenterWithDirection);
        readBool("centerInThirdPerson", ui.centerInThirdPerson);
        readBool("scaleWithResolution", ui.scaleWithResolution);
        readBool("mergeDirection8With1", ui.mergeDirection8With1);
        readInt("positionXPercent", ui.positionXPercent);
        readInt("positionYPercent", ui.positionYPercent);
        readInt("attachOffsetXPixels", ui.attachOffsetXPixels);
        readInt("attachOffsetYPixels", ui.attachOffsetYPixels);
        readInt("scalePercent", ui.scalePercent);
        readInt("diameterPixels", ui.diameterPixels);
        readInt("innerDiameterPercent", ui.innerDiameterPercent);
        readInt("centerPieceSizePercent", ui.centerPieceSizePercent);
        readInt("segmentGapPixels", ui.segmentGapPixels);
        readInt("rotationDegrees", ui.rotationDegrees);
        readInt("opacityPercent", ui.opacityPercent);
        readInt("centerMovementPixels", ui.centerMovementPixels);
        readInt("centerMovementDurationMs", ui.centerMovementDurationMs);
        ReadColor(document, "activeColor", ui.activeColor);
        ReadColor(document, "inactiveColor", ui.inactiveColor);
        ReadColor(document, "borderColor", ui.borderColor);
        ReadColor(document, "centerColor", ui.centerColor);
        if (document.HasMember("mergeWithPrevious") && document["mergeWithPrevious"].IsArray()) {
            const auto& merges = document["mergeWithPrevious"];
            for (rapidjson::SizeType i = 1; i < merges.Size() && i < Settings::kDirectionCount; ++i) {
                if (merges[i].IsBool()) ui.mergeWithPrevious[i] = merges[i].GetBool();
            }
        }
        readString(document, "sharedSegmentImage", ui.sharedSegmentImage);
        readString(document, "centerImage", ui.centerImage);
        if (document.HasMember("segmentImages") && document["segmentImages"].IsArray()) {
            const auto& images = document["segmentImages"];
            for (rapidjson::SizeType i = 0; i < images.Size() && i < Settings::kDirectionCount; ++i) {
                if (images[i].IsString()) ui.segmentImages[i] = images[i].GetString();
            }
        }
        if (!document.HasMember("layoutVersion") || !document["layoutVersion"].IsInt() ||
            document["layoutVersion"].GetInt() < 2) {
            ui.positionXPercent = 50;
            ui.positionYPercent = 50;
        }
        ClampSettings(ui);
    }

    void SaveSettings() {
        ClampSettings(Settings::UI);
        std::filesystem::create_directories(kModDirectory);

        rapidjson::Document document;
        document.SetObject();
        auto& allocator = document.GetAllocator();
        const auto& ui = Settings::UI;
        document.AddMember("layoutVersion", 2, allocator);
        document.AddMember("enabled", ui.enabled, allocator);
        document.AddMember("editMode", ui.editMode, allocator);
        document.AddMember("blockReset", ui.blockReset, allocator);
        document.AddMember("useDirectionalDMK", ui.useDirectionalDMK, allocator);
        document.AddMember("requireTDMTargetLock", ui.requireTDMTargetLock, allocator);
        document.AddMember("showCenter", ui.showCenter, allocator);
        document.AddMember("moveCenterWithDirection", ui.moveCenterWithDirection, allocator);
        document.AddMember("centerInThirdPerson", ui.centerInThirdPerson, allocator);
        document.AddMember("scaleWithResolution", ui.scaleWithResolution, allocator);
        document.AddMember("mergeDirection8With1", ui.mergeDirection8With1, allocator);
        document.AddMember("positionXPercent", ui.positionXPercent, allocator);
        document.AddMember("positionYPercent", ui.positionYPercent, allocator);
        document.AddMember("attachOffsetXPixels", ui.attachOffsetXPixels, allocator);
        document.AddMember("attachOffsetYPixels", ui.attachOffsetYPixels, allocator);
        document.AddMember("scalePercent", ui.scalePercent, allocator);
        document.AddMember("diameterPixels", ui.diameterPixels, allocator);
        document.AddMember("innerDiameterPercent", ui.innerDiameterPercent, allocator);
        document.AddMember("centerPieceSizePercent", ui.centerPieceSizePercent, allocator);
        document.AddMember("segmentGapPixels", ui.segmentGapPixels, allocator);
        document.AddMember("rotationDegrees", ui.rotationDegrees, allocator);
        document.AddMember("opacityPercent", ui.opacityPercent, allocator);
        document.AddMember("centerMovementPixels", ui.centerMovementPixels, allocator);
        document.AddMember("centerMovementDurationMs", ui.centerMovementDurationMs, allocator);
        WriteColor(document, "activeColor", ui.activeColor, allocator);
        WriteColor(document, "inactiveColor", ui.inactiveColor, allocator);
        WriteColor(document, "borderColor", ui.borderColor, allocator);
        WriteColor(document, "centerColor", ui.centerColor, allocator);
        rapidjson::Value merges(rapidjson::kArrayType);
        for (const bool merge : ui.mergeWithPrevious) merges.PushBack(merge, allocator);
        document.AddMember("mergeWithPrevious", merges, allocator);
        document.AddMember("sharedSegmentImage", rapidjson::Value(ui.sharedSegmentImage.c_str(), allocator).Move(), allocator);
        rapidjson::Value images(rapidjson::kArrayType);
        for (const auto& image : ui.segmentImages) {
            images.PushBack(rapidjson::Value(image.c_str(), allocator).Move(), allocator);
        }
        document.AddMember("segmentImages", images, allocator);
        document.AddMember("centerImage", rapidjson::Value(ui.centerImage.c_str(), allocator).Move(), allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);
        std::ofstream file(kUISettingsPath, std::ios::binary);
        if (file.is_open()) {
            file << buffer.GetString();
        }
    }

    void UIRender() {
        auto& ui = Settings::UI;
        bool changed = false;

        if (ImGui::CollapsingHeader(GetLoc("menu.general", "General"), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            changed |= ImGui::Checkbox(GetLoc("menu.enabled", "Enable Honor Combat HUD"), &ui.enabled);
            changed |= ImGui::Checkbox(GetLoc("menu.edit_mode", "UI edit mode (keep HUD visible)"), &ui.editMode);
            changed |= ImGui::Checkbox(
                GetLoc("menu.block_reset", "Block reset (SBF_BlockStart selects center)"),
                &ui.blockReset);
            changed |= ImGui::Checkbox(
                GetLoc("menu.use_directional_dmk", "Use DMK directional movement instead of camera"),
                &ui.useDirectionalDMK);
            changed |= ImGui::Checkbox(
                GetLoc("menu.require_tdm_target_lock", "Show HUD only while a TDM target is locked"),
                &ui.requireTDMTargetLock);
            changed |= ImGui::Checkbox(GetLoc("menu.show_center", "Show center piece"), &ui.showCenter);
            changed |= ImGui::Checkbox(
                GetLoc("menu.move_center", "Move center piece with direction"),
                &ui.moveCenterWithDirection);
            changed |= ImGui::Checkbox(GetLoc("menu.center_third_person", "Use center-screen position in third person"), &ui.centerInThirdPerson);
            changed |= ImGui::Checkbox(GetLoc("menu.resolution_scale", "Scale dynamically with screen resolution"), &ui.scaleWithResolution);
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader(GetLoc("menu.direction_groups", "Direction groups"), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            ImGui::TextWrapped("%s", GetLoc(
                "menu.direction_groups_help",
                "Merge adjacent directions. Each group uses the image/SVG of its first direction."));
            for (std::size_t index = 1; index < Settings::kDirectionCount; ++index) {
                const std::string label = std::format(
                    "{} {} {}##direction_merge_{}",
                    GetLoc("menu.merge_direction", "Merge direction"),
                    index + 1,
                    GetLoc("menu.with_previous", "with previous"),
                    index + 1);
                if (ImGui::Checkbox(label.c_str(), &ui.mergeWithPrevious[index])) {
                    if (index == 7 && ui.mergeWithPrevious[index]) ui.mergeDirection8With1 = false;
                    changed = true;
                }
            }
            if (ImGui::Checkbox(
                    GetLoc(
                        "menu.merge_direction_8_with_1",
                        "Merge direction 8 with direction 1 (uses direction 1 image)"),
                    &ui.mergeDirection8With1)) {
                if (ui.mergeDirection8With1) ui.mergeWithPrevious[7] = false;
                changed = true;
            }

            std::string summary;
            std::size_t groupStart = 0;
            const std::size_t linearDirectionCount = ui.mergeDirection8With1 ? 7 : Settings::kDirectionCount;
            for (std::size_t index = 1; index <= linearDirectionCount; ++index) {
                if (index == linearDirectionCount || !ui.mergeWithPrevious[index]) {
                    if (!summary.empty()) summary += " | ";
                    if (groupStart == 0 && ui.mergeDirection8With1) summary += "8+";
                    summary += std::to_string(groupStart + 1);
                    if (index > groupStart + 1) summary += "-" + std::to_string(index);
                    groupStart = index;
                }
            }
            ImGui::Text("%s: %s", GetLoc("menu.direction_groups_summary", "Groups"), summary.c_str());
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader(GetLoc("menu.layout", "Position and size"), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.center_position", "First-person / center-screen position"));
            changed |= RenderIntSliderWithInput(GetLoc("menu.position_x", "Center horizontal position (%)"), &ui.positionXPercent, 0, 100);
            changed |= RenderIntSliderWithInput(GetLoc("menu.position_y", "Center vertical position (%)"), &ui.positionYPercent, 0, 100);
            ImGui::Separator();
            ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.torso_attachment", "Third-person torso attachment"));
            changed |= RenderIntSliderWithInput(GetLoc("menu.attach_offset_x", "Torso horizontal offset (pixels)"), &ui.attachOffsetXPixels, -1000, 1000);
            changed |= RenderIntSliderWithInput(GetLoc("menu.attach_offset_y", "Torso vertical offset (negative moves down)"), &ui.attachOffsetYPixels, -1000, 1000);
            changed |= RenderIntSliderWithInput(GetLoc("menu.scale", "HUD scale (%)"), &ui.scalePercent, 25, 300);
            changed |= RenderIntSliderWithInput(GetLoc("menu.diameter", "Base diameter (pixels)"), &ui.diameterPixels, 96, 800);
            changed |= RenderIntSliderWithInput(GetLoc("menu.inner_diameter", "Center opening (%)"), &ui.innerDiameterPercent, 10, 80);
            changed |= RenderIntSliderWithInput(
                GetLoc("menu.center_piece_size", "Center piece size (%)"),
                &ui.centerPieceSizePercent,
                10,
                200);
            changed |= RenderIntSliderWithInput(GetLoc("menu.segment_gap", "Segment gap (pixels)"), &ui.segmentGapPixels, 0, 30);
            changed |= RenderIntSliderWithInput(GetLoc("menu.rotation", "Ring rotation (degrees)"), &ui.rotationDegrees, -180, 180);
            changed |= RenderIntSliderWithInput(GetLoc("menu.opacity", "HUD opacity (%)"), &ui.opacityPercent, 10, 100);
            ImGui::Separator();
            ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.center_movement", "Center movement"));
            changed |= RenderIntSliderWithInput(
                GetLoc("menu.center_movement_pixels", "Maximum center movement (pixels)"),
                &ui.centerMovementPixels,
                0,
                400);
            changed |= RenderIntSliderWithInput(
                GetLoc("menu.center_movement_duration", "Center movement duration (ms, 0 = instant)"),
                &ui.centerMovementDurationMs,
                0,
                1000);
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader(GetLoc("menu.colors", "Colors"), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            constexpr ImGuiColorEditFlags colorFlags =
                ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf;
            changed |= ImGui::ColorEdit4(GetLoc("menu.active_color", "Active direction"), ui.activeColor.data(), colorFlags);
            changed |= ImGui::ColorEdit4(GetLoc("menu.inactive_color", "Inactive directions"), ui.inactiveColor.data(), colorFlags);
            changed |= ImGui::ColorEdit4(GetLoc("menu.border_color", "Segment border"), ui.borderColor.data(), colorFlags);
            changed |= ImGui::ColorEdit4(GetLoc("menu.center_color", "Center piece"), ui.centerColor.data(), colorFlags);
            ImGui::Unindent();
        }

        if (ImGui::Button(GetLoc("menu.reset", "Reset UI settings"))) {
            ui = Settings::UISettings{};
            changed = true;
        }

        ClampSettings(ui);
        if (changed) {
            SaveSettings();
            Prisma::ApplyUISettings();
        }
    }

    void Register() {
        LoadLanguage();
        LoadSettings();
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::warn("SKSE Menu Framework not found; Honor Combat will use saved/default UI settings.");
            return;
        }
        SKSEMenuFramework::SetSection("Honor Combat");
        SKSEMenuFramework::AddSectionItem(GetLoc("menu.ui_settings", "UI Settings"), UIRender);
    }
}
