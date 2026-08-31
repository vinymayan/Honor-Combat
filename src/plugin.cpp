#include "Plugin.h"

#include "Configuration.h"
#include "Events.h"
#include "Hooks.h"
#include "Prisma.h"

namespace {
    void OnMessage(SKSE::MessagingInterface::Message* message) {
        switch (message->type) {
        case SKSE::MessagingInterface::kPostLoad:
            Prisma::Install();
            ModMenu::Register();
            HonorCombatEventHandler::GetSingleton()->InitializeTDMAPI();
            break;
        case SKSE::MessagingInterface::kDataLoaded:
            Prisma::Preload();
            HonorCombatEventHandler::GetSingleton()->RegisterMenuListener();
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(PC3DLoadEventHandler::GetSingleton());
            break;
        case SKSE::MessagingInterface::kPreLoadGame:
            Prisma::Reset();
            break;
        case SKSE::MessagingInterface::kNewGame:
        case SKSE::MessagingInterface::kPostLoadGame:
            HonorCombatEventHandler::GetSingleton()->Reset();
            break;
        default:
            break;
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    HonorCombatEventHandler::GetSingleton()->RegisterDMKListener();
    Hooks::Install();
    logger::info("Honor Combat loaded.");
    return true;
}
