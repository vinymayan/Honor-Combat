#pragma once

class Prisma {
public:
    static void Install();
    static void Preload();
    static void Show();
    static void Hide();
    static bool IsHidden();
    static bool IsReady();
    static void ApplyUISettings();
    static void UpdateDirection(int direction);
    static void UpdateRuntimeState(
        bool eligible,
        bool previewEligible,
        float xPercent,
        float yPercent,
        float resolutionScale,
        bool attached);
    static void Reset();
};
