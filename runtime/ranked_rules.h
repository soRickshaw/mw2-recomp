#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace mw2::ranked {

namespace rules {
inline constexpr int kPlayersPerTeam = 5;
inline constexpr int kRegulationRounds = 30;
inline constexpr int kRoundsPerHalf = 15;
inline constexpr int kRoundsToWin = 16;
inline constexpr int kOvertimeRounds = 6;
inline constexpr int kOvertimeRoundsPerHalf = 3;
inline constexpr int kOvertimeRoundsToWin = 4;
inline constexpr int kBuyPhaseSeconds = 15;
inline constexpr int kRoundSeconds = 120;
inline constexpr int kBombFuseSeconds = 40;
inline constexpr int kPlantMilliseconds = 3200;
inline constexpr int kDefuseMilliseconds = 10000;
inline constexpr int kDefuseKitMilliseconds = 5000;
inline constexpr int kRegulationStartMoney = 800;
inline constexpr int kOvertimeStartMoney = 10000;
inline constexpr int kMaximumMoney = 16000;
inline constexpr int kInitialLossCount = 1;
}

enum class Side : std::uint8_t { Attackers, Defenders };
enum class TeamId : std::uint8_t { A, B };
enum class WinReason : std::uint8_t { Elimination, RoundTimerExpired, BombDetonated, BombDefused };

enum class WeaponId : std::uint8_t {
    M9, USP45, DesertEagle, Magnum44, MiniUzi, P90, Vector, MP5K, UMP45,
    AK47, FAMAS, ACR, M16A4, RPD, AUGHBAR, Intervention, SPAS12, Ranger
};

enum class UtilityItem : std::uint8_t {
    Frag, Flash, Smoke, Kevlar, HelmetUpgrade, KevlarAndHelmet, DefuseKit
};

enum class TacticalGrenade : std::uint8_t { None, Flash, Smoke };
enum class KillRewardClass : std::uint8_t {
    Pistol, Rifle, Smg, P90, Lmg, Shotgun, Sniper, Grenade, Knife, NoReward
};

struct PlayerEconomy { int money = rules::kRegulationStartMoney; };

struct TeamEconomy {
    std::array<PlayerEconomy, rules::kPlayersPerTeam> players{};
    int loss_count = rules::kInitialLossCount;
};

struct PlayerLoadout {
    bool has_frag = false;
    TacticalGrenade tactical = TacticalGrenade::None;
    bool has_kevlar = false;
    bool has_helmet = false;
    bool has_defuse_kit = false;
};

struct TeamEconomyPair {
    TeamEconomy attackers;
    TeamEconomy defenders;
};

struct RoundResult {
    Side winning_side = Side::Attackers;
    WinReason reason = WinReason::Elimination;
    bool bomb_was_planted = false;
};

enum class MatchTransition : std::uint8_t {
    ContinueRegulation,
    RegulationHalfTime,
    OvertimeStarted,
    ContinueOvertime,
    OvertimeHalfTime,
    OvertimeRestarted,
    MatchFinished
};

struct MatchSnapshot {
    int team_a_rounds = 0;
    int team_b_rounds = 0;
    int regulation_rounds_played = 0;
    bool overtime = false;
    int overtime_period = 0;
    int overtime_rounds_in_period = 0;
    int overtime_team_a_rounds = 0;
    int overtime_team_b_rounds = 0;
    Side team_a_side = Side::Attackers;
    bool finished = false;
    std::optional<TeamId> winner;
};

// RecordRound must only be called after the game/server validates a round result.
class MatchProgress {
public:
    explicit MatchProgress(Side team_a_starting_side = Side::Attackers);
    MatchTransition RecordRound(TeamId winning_team);
    const MatchSnapshot& state() const noexcept { return state_; }
    Side sideOf(TeamId team) const noexcept;
private:
    MatchSnapshot state_{};
};

int LossBonusForCount(int loss_count) noexcept;
int KillReward(KillRewardClass weapon_class) noexcept;
int WeaponPrice(WeaponId weapon) noexcept;
bool WeaponAvailableTo(WeaponId weapon, Side side) noexcept;
int UtilityPrice(UtilityItem item) noexcept;

bool PurchaseWeapon(PlayerEconomy& player, WeaponId weapon, Side side, bool buy_phase_active);
bool PurchaseUtility(PlayerEconomy& player, PlayerLoadout& loadout, UtilityItem item,
                     Side side, bool buy_phase_active);

void AwardKill(PlayerEconomy& player, KillRewardClass weapon_class) noexcept;
void AwardBombPlant(PlayerEconomy& planter) noexcept;
void AwardBombDefuse(PlayerEconomy& defuser) noexcept;
void ResolveRoundEconomy(TeamEconomyPair& economy, const RoundResult& result);
void ResetRegulationHalf(TeamEconomyPair& economy) noexcept;
void ResetOvertimeHalf(TeamEconomyPair& economy) noexcept;

} // namespace mw2::ranked
