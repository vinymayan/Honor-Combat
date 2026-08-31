#include "Events.h"

#include "Configuration.h"
#include "Prisma.h"
#include "TrueDirectionalMovementAPI.h"
#include "DelayedDispatcher.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {
    inline std::array blockedMenus = {
        RE::DialogueMenu::MENU_NAME,    RE::JournalMenu::MENU_NAME,    RE::MapMenu::MENU_NAME,
        RE::StatsMenu::MENU_NAME,       RE::ContainerMenu::MENU_NAME,  RE::InventoryMenu::MENU_NAME,
        RE::TweenMenu::MENU_NAME,       RE::TrainingMenu::MENU_NAME,   RE::TutorialMenu::MENU_NAME,
        RE::LockpickingMenu::MENU_NAME, RE::SleepWaitMenu::MENU_NAME,  RE::LevelUpMenu::MENU_NAME,
        RE::Console::MENU_NAME,         RE::BookMenu::MENU_NAME,       RE::CreditsMenu::MENU_NAME,
        RE::LoadingMenu::MENU_NAME,     RE::MessageBoxMenu::MENU_NAME, RE::MainMenu::MENU_NAME,
        RE::RaceSexMenu::MENU_NAME,     RE::FavoritesMenu::MENU_NAME
    };
    std::unordered_set<std::string> openBlockingMenus;
    TDM_API::IVTDM1* tdmAPI = nullptr;

    bool IsBlockingMenuName(std::string_view name) {
        return std::ranges::any_of(blockedMenus, [name](const auto& blocked) {
            return name == std::string_view(blocked);
        });
    }

    bool IsAnyBlockingMenuOpen() {
        auto* ui = RE::UI::GetSingleton();
        if (!ui) return true;
        return ui->GameIsPaused() || !openBlockingMenus.empty();
    }

    bool IsPreviewBlockingMenuOpen() {
        return openBlockingMenus.contains(std::string(RE::MainMenu::MENU_NAME)) ||
               openBlockingMenus.contains(std::string(RE::LoadingMenu::MENU_NAME)) ||
               openBlockingMenus.contains(std::string(RE::CreditsMenu::MENU_NAME));
    }

    void RefreshBlockingMenus() {
        openBlockingMenus.clear();
        auto* ui = RE::UI::GetSingleton();
        if (!ui) return;
        for (const auto menu : blockedMenus) {
            if (ui->IsMenuOpen(menu)) openBlockingMenus.emplace(menu);
        }
    }

    float GetPlayerHealth(RE::PlayerCharacter* player) {
        const auto* actorValueOwner = player ? player->AsActorValueOwner() : nullptr;
        return actorValueOwner ? actorValueOwner->GetActorValue(RE::ActorValue::kHealth) : 0.0f;
    }

    bool IsPlayerAlive(RE::PlayerCharacter* player) {
        const float health = GetPlayerHealth(player);
        return player && std::isfinite(health) && health > 0.0f;
    }

    bool IsPlayerStructurallyEligible(RE::PlayerCharacter* player) {
        return player && IsPlayerAlive(player);
    }

    bool IsRuntimeInteractionEligible() {
        return !IsAnyBlockingMenuOpen();
    }

    bool HasLockedTDMTarget() {
        if (!tdmAPI || !tdmAPI->GetTargetLockState()) return false;
        const auto target = tdmAPI->GetCurrentTarget().get();
        return target != nullptr;
    }

    RE::NiAVObject* GetTorsoNode(RE::PlayerCharacter* player) {
        auto* root = player ? player->Get3D(false) : nullptr;
        if (!root) return nullptr;
        constexpr std::array torsoNames = {
            "NPC Spine1 [Spn1]",
            "NPC Spine2 [Spn2]",
            "NPC Spine [Spn0]",
            "NPC COM [COM ]",
            "Bip01 Spine1",
            "Bip01 Spine2"
        };
        for (const auto* name : torsoNames) {
            if (auto* node = root->GetObjectByName(RE::BSFixedString(name))) return node;
        }
        return root;
    }

    bool ProjectToScreenPercent(const RE::NiPoint3& worldPosition, float& xPercent, float& yPercent) {
        auto* camera = RE::Main::WorldRootCamera();
        if (!camera) return false;

        float normalizedX = 0.0f;
        float normalizedY = 0.0f;
        float depth = 0.0f;
        if (!camera->WorldPtToScreenPt3(worldPosition, normalizedX, normalizedY, depth, 1e-5f)) return false;

        xPercent = std::clamp(normalizedX * 100.0f, 0.0f, 100.0f);
        yPercent = std::clamp((1.0f - normalizedY) * 100.0f, 0.0f, 100.0f);
        return true;
    }

    float GetResolutionScale() {
        if (!Settings::UI.scaleWithResolution) return 1.0f;
        const auto screen = RE::BSGraphics::Renderer::GetScreenSize();
        if (screen.height == 0) return 1.0f;
        return std::clamp(static_cast<float>(screen.height) / 1080.0f, 0.5f, 4.0f);
    }
}

HonorCombatEventHandler* HonorCombatEventHandler::GetSingleton() {
    static HonorCombatEventHandler singleton;
    return &singleton;
}

void HonorCombatEventHandler::RegisterDMKListener() {
    if (dmkRegistered_) return;
    if (auto* source = SKSE::GetModCallbackEventSource()) {
        source->AddEventSink(this);
        dmkRegistered_ = true;
        logger::info("Honor Combat is listening for DMKUpdate events.");
    } else {
        logger::error("Honor Combat could not obtain the SKSE mod callback event source.");
    }
}

void HonorCombatEventHandler::InitializeTDMAPI() {
    tdmAPI = reinterpret_cast<TDM_API::IVTDM1*>(
        TDM_API::RequestPluginAPI(TDM_API::InterfaceVersion::V1));
    if (tdmAPI) {
        logger::info("Honor Combat connected to the True Directional Movement API.");
    } else {
        logger::warn("Honor Combat could not obtain the True Directional Movement API; target-lock-only HUD mode will remain hidden.");
    }
}

void HonorCombatEventHandler::RegisterMenuListener() {
    if (menuRegistered_) return;
    if (auto* ui = RE::UI::GetSingleton()) {
        ui->AddEventSink<RE::MenuOpenCloseEvent>(this);
        menuRegistered_ = true;
        RefreshBlockingMenus();
    }
}

void HonorCombatEventHandler::RegisterPlayerAnimationSink() {
    playerAnimationRegistered_ = false;
    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
        RE::BSTSmartPointer<RE::BSAnimationGraphManager> graphManager;
        player->GetAnimationGraphManager(graphManager);
        if (!graphManager) return;
        player->RemoveAnimationGraphEventSink(this);
        playerAnimationRegistered_ = player->AddAnimationGraphEventSink(this);
    }
}

void HonorCombatEventHandler::UpdateFrame() {
    if (!playerAnimationRegistered_) RegisterPlayerAnimationSink();
    auto* player = RE::PlayerCharacter::GetSingleton();
    const float resolutionScale = GetResolutionScale();
    const bool structurallyEligible = IsPlayerStructurallyEligible(player);
    const bool previewBlocked = IsPreviewBlockingMenuOpen();
    const std::string_view frameGate = !Settings::UI.enabled ? "disabled" :
                                       !structurallyEligible ? "player-not-ready" :
                                       previewBlocked ? "main-loading-or-credits" : "passed";
    if (frameGate != "passed") {
        Prisma::UpdateRuntimeState(false, false, 50.0f, 50.0f, resolutionScale, false);
        return;
    }
    const auto* actorState = player->AsActorState();
    const bool weaponDrawn = actorState && actorState->IsWeaponDrawn();
    const bool targetLockEligible = !Settings::UI.requireTDMTargetLock || HasLockedTDMTarget();
    const bool interactionEligible = IsRuntimeInteractionEligible();
    const bool runtimeEligible = weaponDrawn && targetLockEligible && interactionEligible;
    const auto* playerCamera = RE::PlayerCamera::GetSingleton();
    const bool firstPerson = !playerCamera || playerCamera->IsInFirstPerson();
    if (firstPerson || Settings::UI.centerInThirdPerson) {
        Prisma::UpdateRuntimeState(runtimeEligible, true, 50.0f, 50.0f, resolutionScale, false);
        return;
    }

    const auto* torso = GetTorsoNode(player);
    float xPercent = 50.0f;
    float yPercent = 50.0f;
    if (!torso || !ProjectToScreenPercent(torso->world.translate, xPercent, yPercent)) {
        static bool projectionFailureLogged = false;
        if (!projectionFailureLogged) {
            logger::warn("Honor Combat torso projection failed; runtime hidden and edit preview moved to screen center.");
            projectionFailureLogged = true;
        }
        Prisma::UpdateRuntimeState(false, true, 50.0f, 50.0f, resolutionScale, false);
        return;
    }

    const auto screen = RE::BSGraphics::Renderer::GetScreenSize();
    if (screen.width > 0 && screen.height > 0) {
        xPercent += static_cast<float>(Settings::UI.attachOffsetXPixels) * resolutionScale * 100.0f /
                    static_cast<float>(screen.width);
        yPercent -= static_cast<float>(Settings::UI.attachOffsetYPixels) * resolutionScale * 100.0f /
                    static_cast<float>(screen.height);
    }
    Prisma::UpdateRuntimeState(
        runtimeEligible,
        true,
        std::clamp(xPercent, 0.0f, 100.0f),
        std::clamp(yPercent, 0.0f, 100.0f),
        resolutionScale,
        true);
}

void HonorCombatEventHandler::Reset() {
    direction_ = 0;
    Prisma::Reset();
    RegisterPlayerAnimationSink();
    RefreshBlockingMenus();
    UpdateFrame();
}

RE::BSEventNotifyControl HonorCombatEventHandler::ProcessEvent(
    const SKSE::ModCallbackEvent* event,
    RE::BSTEventSource<SKSE::ModCallbackEvent>*) {
    if (!event || std::string_view(event->eventName.c_str()) != "DMKUpdate") {
        return RE::BSEventNotifyControl::kContinue;
    }

    const std::string_view expectedSource = Settings::UI.useDirectionalDMK ? "Direcional" : "Camera";
    if (std::string_view(event->strArg.c_str()) != expectedSource) {
        return RE::BSEventNotifyControl::kContinue;
    }

    const int direction = std::clamp(static_cast<int>(std::lround(event->numArg)), 0, 8);
    if (Settings::UI.useDirectionalDMK) {
        if (auto* player = RE::PlayerCharacter::GetSingleton()) {
            player->SetGraphVariableInt("DirecionalCycleMoveset", direction);
        }
    }
    if (direction != direction_) {
        direction_ = direction;
        Prisma::UpdateDirection(direction_);
    }
    UpdateFrame();
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl HonorCombatEventHandler::ProcessEvent(
    const RE::BSAnimationGraphEvent* event,
    RE::BSTEventSource<RE::BSAnimationGraphEvent>*) {
    if (event && event->holder && event->holder->IsPlayerRef()) {
        if (Settings::UI.blockReset && std::string_view(event->tag.c_str()) == "SBF_BlockStart") {
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (player) {
                player->SetGraphVariableInt(
                    Settings::UI.useDirectionalDMK ? "DirecionalCycleMoveset" : "CameraMovementCMF",
                    0);
            }
            direction_ = 0;
            Prisma::UpdateDirection(0);
        }
        UpdateFrame();
    }
    return RE::BSEventNotifyControl::kContinue;
}

PC3DLoadEventHandler* PC3DLoadEventHandler::GetSingleton() {
    static PC3DLoadEventHandler singleton;
    return &singleton;
}

RE::BSEventNotifyControl PC3DLoadEventHandler::ProcessEvent(
    const RE::TESObjectLoadedEvent* event,
    RE::BSTEventSource<RE::TESObjectLoadedEvent>*) {
    if (!event || !event->loaded) return RE::BSEventNotifyControl::kContinue;

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || event->formID != player->GetFormID()) return RE::BSEventNotifyControl::kContinue;

    SKSE::GetTaskInterface()->AddTask([] {
        HonorCombatEventHandler::GetSingleton()->RegisterPlayerAnimationSink();
    });
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl HonorCombatEventHandler::ProcessEvent(
    const RE::MenuOpenCloseEvent* event,
    RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
    if (!event) return RE::BSEventNotifyControl::kContinue;
    const std::string menuName = event->menuName.c_str();
    const bool blocking = IsBlockingMenuName(menuName);
    if (blocking && event->opening) openBlockingMenus.insert(menuName);
    if (blocking && !event->opening) openBlockingMenus.erase(menuName);

    if (!event->opening) {
        SKSE::GetTaskInterface()->AddTask([] { HonorCombatEventHandler::GetSingleton()->UpdateFrame(); });
    } else {
        UpdateFrame();
    }
    return RE::BSEventNotifyControl::kContinue;
}
