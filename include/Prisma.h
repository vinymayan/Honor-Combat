#pragma once

#include <cstdint>
#include <vector>

struct AttackWarningVisual {
    std::uint32_t id{ 0 };
    int direction{ 0 };
    float xPercent{ 50.0f };
    float yPercent{ 50.0f };
    float resolutionScale{ 1.0f };
    float distanceScale{ 1.0f };
};

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
    static void UpdateAttackWarnings(const std::vector<AttackWarningVisual>& warnings);
    static void Reset();
};
