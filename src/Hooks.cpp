#include "Hooks.h"

#include "Events.h"

namespace {
    struct PlayerCharacterUpdateHook {
        static void Update(RE::PlayerCharacter* player, float delta) {
            original(player, delta);
            HonorCombatEventHandler::GetSingleton()->UpdateFrame();
        }

        static void Install() {
            REL::Relocation<std::uintptr_t> vtable(RE::VTABLE_PlayerCharacter[0]);
            original = vtable.write_vfunc(0xAD, Update);
        }

        static inline REL::Relocation<decltype(&Update)> original;
    };
}

void Hooks::Install() {
    PlayerCharacterUpdateHook::Install();
    logger::info("Honor Combat player update hook installed.");
}
