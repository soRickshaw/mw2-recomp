#include "ranked_rules.h"

#include <algorithm>

namespace mw2::ranked {
namespace {
constexpr int kRoundWinElimination = 3250;
constexpr int kRoundWinBomb = 3500;
constexpr int kBombPlantedLossAward = 800;
constexpr int kBombPlantPersonalAward = 300;
constexpr int kBombDefusePersonalAward = 300;

void AddMoney(PlayerEconomy& player, int amount) noexcept {
    player.money = std::clamp(player.money + amount, 0, rules::kMaximumMoney);
}

int WinAward(const RoundResult& result) noexcept {
    return result.reason == WinReason::BombDetonated || result.reason == WinReason::BombDefused
        ? kRoundWinBomb : kRoundWinElimination;
}

void ResetTeam(TeamEconomy& team, int cash, int loss_count) noexcept {
    for (auto& player : team.players) player.money = cash;
    team.loss_count = loss_count;
}
} // namespace

MatchProgress::MatchProgress(Side team_a_starting_side) {
    state_.team_a_side = team_a_starting_side;
}

Side MatchProgress::sideOf(TeamId team) const noexcept {
    if (team == TeamId::A) return state_.team_a_side;
    return state_.team_a_side == Side::Attackers ? Side::Defenders : Side::Attackers;
}

MatchTransition MatchProgress::RecordRound(TeamId winning_team) {
    if (state_.finished) return MatchTransition::MatchFinished;

    if (!state_.overtime) {
        if (winning_team == TeamId::A) ++state_.team_a_rounds;
        else ++state_.team_b_rounds;
        ++state_.regulation_rounds_played;

        if (state_.team_a_rounds >= rules::kRoundsToWin || state_.team_b_rounds >= rules::kRoundsToWin) {
            state_.finished = true;
            state_.winner = state_.team_a_rounds > state_.team_b_rounds ? TeamId::A : TeamId::B;
            return MatchTransition::MatchFinished;
        }

        if (state_.regulation_rounds_played == rules::kRoundsPerHalf) {
            state_.team_a_side = state_.team_a_side == Side::Attackers ? Side::Defenders : Side::Attackers;
            return MatchTransition::RegulationHalfTime;
        }

        if (state_.regulation_rounds_played == rules::kRegulationRounds) {
            state_.overtime = true;
            state_.overtime_period = 1;
            state_.overtime_rounds_in_period = 0;
            state_.overtime_team_a_rounds = 0;
            state_.overtime_team_b_rounds = 0;
            return MatchTransition::OvertimeStarted;
        }
        return MatchTransition::ContinueRegulation;
    }

    if (winning_team == TeamId::A) {
        ++state_.team_a_rounds;
        ++state_.overtime_team_a_rounds;
    } else {
        ++state_.team_b_rounds;
        ++state_.overtime_team_b_rounds;
    }
    ++state_.overtime_rounds_in_period;

    if (state_.overtime_team_a_rounds >= rules::kOvertimeRoundsToWin ||
        state_.overtime_team_b_rounds >= rules::kOvertimeRoundsToWin) {
        state_.finished = true;
        state_.winner = state_.overtime_team_a_rounds > state_.overtime_team_b_rounds ? TeamId::A : TeamId::B;
        return MatchTransition::MatchFinished;
    }

    if (state_.overtime_rounds_in_period == rules::kOvertimeRoundsPerHalf) {
        state_.team_a_side = state_.team_a_side == Side::Attackers ? Side::Defenders : Side::Attackers;
        return MatchTransition::OvertimeHalfTime;
    }

    if (state_.overtime_rounds_in_period == rules::kOvertimeRounds) {
        ++state_.overtime_period;
        state_.overtime_rounds_in_period = 0;
        state_.overtime_team_a_rounds = 0;
        state_.overtime_team_b_rounds = 0;
        return MatchTransition::OvertimeRestarted;
    }
    return MatchTransition::ContinueOvertime;
}

int LossBonusForCount(int loss_count) noexcept {
    return 1400 + std::clamp(loss_count, 0, 4) * 500;
}

int KillReward(KillRewardClass weapon_class) noexcept {
    switch (weapon_class) {
    case KillRewardClass::Pistol: return 300;
    case KillRewardClass::Rifle: return 300;
    case KillRewardClass::Smg: return 600;
    case KillRewardClass::P90: return 300;
    case KillRewardClass::Lmg: return 300;
    case KillRewardClass::Shotgun: return 900;
    case KillRewardClass::Sniper: return 100;
    case KillRewardClass::Grenade: return 300;
    case KillRewardClass::Knife: return 1500;
    case KillRewardClass::NoReward: return 0;
    }
    return 0;
}

int WeaponPrice(WeaponId weapon) noexcept {
    switch (weapon) {
    case WeaponId::M9:
    case WeaponId::USP45: return 0;
    case WeaponId::DesertEagle: return 700;
    case WeaponId::Magnum44: return 600;
    case WeaponId::MiniUzi: return 1050;
    case WeaponId::P90: return 2350;
    case WeaponId::Vector: return 1250;
    case WeaponId::MP5K: return 1500;
    case WeaponId::UMP45: return 2350;
    case WeaponId::AK47: return 2700;
    case WeaponId::FAMAS: return 2050;
    case WeaponId::ACR: return 3100;
    case WeaponId::M16A4: return 2050;
    case WeaponId::RPD: return 5200;
    case WeaponId::AUGHBAR: return 3300;
    case WeaponId::Intervention: return 4750;
    case WeaponId::SPAS12: return 1050;
    case WeaponId::Ranger: return 1100;
    }
    return -1;
}

bool WeaponAvailableTo(WeaponId weapon, Side side) noexcept {
    switch (weapon) {
    case WeaponId::M9: return side == Side::Attackers;
    case WeaponId::USP45: return side == Side::Defenders;
    case WeaponId::AK47:
    case WeaponId::FAMAS:
    case WeaponId::RPD: return side == Side::Attackers;
    case WeaponId::ACR:
    case WeaponId::M16A4:
    case WeaponId::AUGHBAR: return side == Side::Defenders;
    case WeaponId::DesertEagle:
    case WeaponId::Magnum44:
    case WeaponId::MiniUzi:
    case WeaponId::P90:
    case WeaponId::Vector:
    case WeaponId::MP5K:
    case WeaponId::UMP45:
    case WeaponId::Intervention:
    case WeaponId::SPAS12:
    case WeaponId::Ranger: return true;
    }
    return false;
}

int UtilityPrice(UtilityItem item) noexcept {
    switch (item) {
    case UtilityItem::Frag: return 300;
    case UtilityItem::Flash: return 200;
    case UtilityItem::Smoke: return 300;
    case UtilityItem::Kevlar: return 650;
    case UtilityItem::HelmetUpgrade: return 350;
    case UtilityItem::KevlarAndHelmet: return 1000;
    case UtilityItem::DefuseKit: return 400;
    }
    return -1;
}

bool PurchaseWeapon(PlayerEconomy& player, WeaponId weapon, Side side, bool buy_phase_active) {
    if (!buy_phase_active || !WeaponAvailableTo(weapon, side)) return false;
    const int price = WeaponPrice(weapon);
    if (price <= 0 || player.money < price) return false;
    player.money -= price;
    return true;
}

bool PurchaseUtility(PlayerEconomy& player, PlayerLoadout& loadout, UtilityItem item,
                     Side side, bool buy_phase_active) {
    if (!buy_phase_active) return false;
    if (item == UtilityItem::DefuseKit && side != Side::Defenders) return false;

    switch (item) {
    case UtilityItem::Frag:
        if (loadout.has_frag) return false;
        break;
    case UtilityItem::Flash:
    case UtilityItem::Smoke:
        if (loadout.tactical != TacticalGrenade::None) return false;
        break;
    case UtilityItem::Kevlar:
        if (loadout.has_kevlar) return false;
        break;
    case UtilityItem::HelmetUpgrade:
        if (!loadout.has_kevlar || loadout.has_helmet) return false;
        break;
    case UtilityItem::KevlarAndHelmet:
        if (loadout.has_kevlar || loadout.has_helmet) return false;
        break;
    case UtilityItem::DefuseKit:
        if (loadout.has_defuse_kit) return false;
        break;
    }

    const int price = UtilityPrice(item);
    if (price < 0 || player.money < price) return false;
    player.money -= price;

    switch (item) {
    case UtilityItem::Frag: loadout.has_frag = true; break;
    case UtilityItem::Flash: loadout.tactical = TacticalGrenade::Flash; break;
    case UtilityItem::Smoke: loadout.tactical = TacticalGrenade::Smoke; break;
    case UtilityItem::Kevlar: loadout.has_kevlar = true; break;
    case UtilityItem::HelmetUpgrade: loadout.has_helmet = true; break;
    case UtilityItem::KevlarAndHelmet:
        loadout.has_kevlar = true;
        loadout.has_helmet = true;
        break;
    case UtilityItem::DefuseKit: loadout.has_defuse_kit = true; break;
    }
    return true;
}

void AwardKill(PlayerEconomy& player, KillRewardClass weapon_class) noexcept {
    AddMoney(player, KillReward(weapon_class));
}
void AwardBombPlant(PlayerEconomy& planter) noexcept { AddMoney(planter, kBombPlantPersonalAward); }
void AwardBombDefuse(PlayerEconomy& defuser) noexcept { AddMoney(defuser, kBombDefusePersonalAward); }

void ResolveRoundEconomy(TeamEconomyPair& economy, const RoundResult& result) {
    TeamEconomy& winners = result.winning_side == Side::Attackers ? economy.attackers : economy.defenders;
    TeamEconomy& losers = result.winning_side == Side::Attackers ? economy.defenders : economy.attackers;
    const int win_award = WinAward(result);
    const int loss_award = LossBonusForCount(losers.loss_count);
    const bool attackers_lost_after_plant = result.winning_side == Side::Defenders && result.bomb_was_planted;

    for (auto& player : winners.players) AddMoney(player, win_award);
    for (auto& player : losers.players) {
        AddMoney(player, loss_award);
        if (attackers_lost_after_plant && &losers == &economy.attackers)
            AddMoney(player, kBombPlantedLossAward);
    }
    winners.loss_count = std::max(0, winners.loss_count - 1);
    losers.loss_count = std::min(4, losers.loss_count + 1);
}

void ResetRegulationHalf(TeamEconomyPair& economy) noexcept {
    ResetTeam(economy.attackers, rules::kRegulationStartMoney, rules::kInitialLossCount);
    ResetTeam(economy.defenders, rules::kRegulationStartMoney, rules::kInitialLossCount);
}
void ResetOvertimeHalf(TeamEconomyPair& economy) noexcept {
    ResetTeam(economy.attackers, rules::kOvertimeStartMoney, rules::kInitialLossCount);
    ResetTeam(economy.defenders, rules::kOvertimeStartMoney, rules::kInitialLossCount);
}

} // namespace mw2::ranked
