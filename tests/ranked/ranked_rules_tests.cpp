#include "ranked_rules.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace mw2::ranked;

namespace {
int failures = 0;
void Check(bool condition, const std::string& name) {
    if (condition) std::cout << "PASS: " << name << '\n';
    else {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}
}

int main() {
    Check(rules::kPlayersPerTeam == 5, "5v5 teams");
    Check(rules::kRoundSeconds == 120 && rules::kBombFuseSeconds == 40, "2-minute round and 40-second bomb");
    Check(rules::kBuyPhaseSeconds == 15 && rules::kPlantMilliseconds == 3200, "buy and plant timings");
    Check(rules::kDefuseMilliseconds == 10000 && rules::kDefuseKitMilliseconds == 5000, "defuse timings");

    Check(LossBonusForCount(0) == 1400, "loss bonus at counter 0");
    Check(LossBonusForCount(1) == 1900, "loss bonus at counter 1");
    Check(LossBonusForCount(2) == 2400, "loss bonus at counter 2");
    Check(LossBonusForCount(3) == 2900, "loss bonus at counter 3");
    Check(LossBonusForCount(4) == 3400 && LossBonusForCount(9) == 3400, "loss bonus cap");

    Check(KillReward(KillRewardClass::Smg) == 600, "SMG kill reward");
    Check(KillReward(KillRewardClass::P90) == 300, "P90 exception");
    Check(KillReward(KillRewardClass::Shotgun) == 900, "shotgun kill reward");
    Check(KillReward(KillRewardClass::Sniper) == 100, "Intervention/AWP kill reward");

    Check(WeaponPrice(WeaponId::AK47) == 2700, "AK-47 price");
    Check(WeaponPrice(WeaponId::ACR) == 3100, "ACR/M4 price equivalent");
    Check(WeaponPrice(WeaponId::UMP45) == 2350, "UMP45 premium price exception");
    Check(WeaponAvailableTo(WeaponId::AK47, Side::Attackers), "AK available to attackers");
    Check(!WeaponAvailableTo(WeaponId::AK47, Side::Defenders), "AK unavailable to defenders");
    Check(WeaponAvailableTo(WeaponId::Intervention, Side::Attackers) &&
          WeaponAvailableTo(WeaponId::Intervention, Side::Defenders), "Intervention shared by both sides");
    Check(WeaponAvailableTo(WeaponId::P90, Side::Attackers) &&
          WeaponAvailableTo(WeaponId::P90, Side::Defenders), "P90 shared by both sides");

    PlayerEconomy buyer{5000};
    Check(!PurchaseWeapon(buyer, WeaponId::AK47, Side::Attackers, false) && buyer.money == 5000,
          "cannot buy outside buy phase");
    Check(PurchaseWeapon(buyer, WeaponId::AK47, Side::Attackers, true) && buyer.money == 2300,
          "valid weapon purchase deducts money");
    Check(!PurchaseWeapon(buyer, WeaponId::ACR, Side::Attackers, true) && buyer.money == 2300,
          "cannot purchase opposing faction rifle");

    PlayerEconomy utility_buyer{3000};
    PlayerLoadout loadout;
    Check(PurchaseUtility(utility_buyer, loadout, UtilityItem::Frag, Side::Attackers, true), "buy frag grenade");
    Check(!PurchaseUtility(utility_buyer, loadout, UtilityItem::Frag, Side::Attackers, true), "one frag maximum");
    Check(PurchaseUtility(utility_buyer, loadout, UtilityItem::Flash, Side::Attackers, true), "buy one tactical grenade");
    Check(!PurchaseUtility(utility_buyer, loadout, UtilityItem::Smoke, Side::Attackers, true),
          "cannot carry both flash and smoke");
    Check(!PurchaseUtility(utility_buyer, loadout, UtilityItem::DefuseKit, Side::Attackers, true),
          "attackers cannot buy defuse kit");
    Check(PurchaseUtility(utility_buyer, loadout, UtilityItem::DefuseKit, Side::Defenders, true),
          "defenders can buy defuse kit");

    TeamEconomyPair economy;
    ResolveRoundEconomy(economy, {Side::Attackers, WinReason::Elimination, false});
    Check(economy.defenders.players[0].money == 2700, "opening-round loss pays $1,900");
    Check(economy.attackers.players[0].money == 4050, "elimination win pays $3,250");
    Check(economy.defenders.loss_count == 2 && economy.attackers.loss_count == 0, "loss counters update");

    TeamEconomyPair planted_loss;
    AwardBombPlant(planted_loss.attackers.players[0]);
    ResolveRoundEconomy(planted_loss, {Side::Defenders, WinReason::BombDefused, true});
    Check(planted_loss.attackers.players[0].money == 3800, "plant-loss team award applied");
    AwardBombDefuse(planted_loss.defenders.players[0]);
    Check(planted_loss.defenders.players[0].money == 4600, "defuse personal award applied");

    ResetRegulationHalf(economy);
    Check(economy.attackers.players[0].money == 800 && economy.defenders.players[0].money == 800,
          "regulation half reset");
    ResetOvertimeHalf(economy);
    Check(economy.attackers.players[0].money == 10000 && economy.defenders.players[0].money == 10000,
          "overtime half reset");

    MatchProgress match(Side::Attackers);
    MatchTransition transition = MatchTransition::ContinueRegulation;
    for (int i = 0; i < 15; ++i) transition = match.RecordRound(TeamId::A);
    Check(transition == MatchTransition::RegulationHalfTime, "halftime after 15 rounds");
    Check(match.sideOf(TeamId::A) == Side::Defenders, "team A swaps sides");
    for (int i = 0; i < 15; ++i) transition = match.RecordRound(TeamId::B);
    Check(transition == MatchTransition::OvertimeStarted, "15-15 starts overtime");
    for (int i = 0; i < 3; ++i) transition = match.RecordRound(TeamId::A);
    Check(transition == MatchTransition::OvertimeHalfTime, "overtime halftime after three");
    for (int i = 0; i < 3; ++i) transition = match.RecordRound(TeamId::B);
    Check(transition == MatchTransition::OvertimeRestarted, "3-3 repeats overtime");
    for (int i = 0; i < 4; ++i) transition = match.RecordRound(TeamId::A);
    Check(transition == MatchTransition::MatchFinished && match.state().winner == TeamId::A,
          "first team to four wins overtime");

    MatchProgress regulation_win;
    for (int i = 0; i < 16; ++i) transition = regulation_win.RecordRound(TeamId::B);
    Check(transition == MatchTransition::MatchFinished && regulation_win.state().winner == TeamId::B,
          "first team to 16 wins regulation");

    if (failures) {
        std::cerr << failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "All ranked rules tests passed.\n";
    return EXIT_SUCCESS;
}
