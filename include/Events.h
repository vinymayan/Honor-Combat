#pragma once

class HonorCombatEventHandler final :
    public RE::BSTEventSink<SKSE::ModCallbackEvent>,
    public RE::BSTEventSink<RE::BSAnimationGraphEvent>,
    public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    static HonorCombatEventHandler* GetSingleton();

    void RegisterDMKListener();
    void InitializeTDMAPI();
    void RegisterMenuListener();
    void RegisterPlayerAnimationSink();
    void UpdateFrame();
    void Reset();

    RE::BSEventNotifyControl ProcessEvent(
        const SKSE::ModCallbackEvent* event,
        RE::BSTEventSource<SKSE::ModCallbackEvent>*) override;
    RE::BSEventNotifyControl ProcessEvent(
        const RE::BSAnimationGraphEvent* event,
        RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;
    RE::BSEventNotifyControl ProcessEvent(
        const RE::MenuOpenCloseEvent* event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

private:
    int direction_ = 0;
    bool dmkRegistered_ = false;
    bool menuRegistered_ = false;
    bool playerAnimationRegistered_ = false;
};

class PC3DLoadEventHandler final : public RE::BSTEventSink<RE::TESObjectLoadedEvent> {
public:
    static PC3DLoadEventHandler* GetSingleton();

    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESObjectLoadedEvent* event,
        RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;
};
