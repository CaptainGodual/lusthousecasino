// Pure economy rules. Storage and reel execution are injected by the transaction coordinator.
function createEconomyDomain(currentPlayerId, store, callNakama) {
var handlers = {};
var PROFILE_KEY = "SugarLustProfile";
var NAKAMA_URL_KEY = "SugarLustNakamaGatewayUrl";
var NAKAMA_HTTP_KEY = "SugarLustNakamaHttpKey";
var LEGACY_PLAYER_ID = "975A360332E4C54D";
var BONUS_MULTIPLIERS = [5,8,10,12,20,50];
var DAILY_MISSIONS = [
    { Id:"spin_10", Description:"Spin 10 times", Target:10, RewardCoins:25000 },
    { Id:"win_50000", Description:"Win 50,000 coins total", Target:50000, RewardCoins:50000 },
    { Id:"scatter_3", Description:"Trigger bonus 3 times", Target:3, RewardCoins:75000 },
    { Id:"bet_high", Description:"Play 5 high-bet spins", Target:5, RewardCoins:40000 },
    { Id:"level_up", Description:"Gain XP from spins", Target:1000, RewardCoins:60000 }
];

function ensureDailyMissions(profile) {
    var today = new Date().toISOString().slice(0, 10);
    var valid = profile.MissionsDate === today && Array.isArray(profile.Missions) && profile.Missions.length === DAILY_MISSIONS.length;
    if (valid) {
        for (var i = 0; i < DAILY_MISSIONS.length; ++i) {
            if (!profile.Missions[i] || profile.Missions[i].Id !== DAILY_MISSIONS[i].Id) { valid = false; break; }
        }
    }
    if (!valid) {
        profile.MissionsDate = today;
        profile.Missions = DAILY_MISSIONS.map(function (mission) {
            return { Id:mission.Id, Description:mission.Description, Target:mission.Target,
                Progress:0, RewardCoins:mission.RewardCoins, Claimed:false };
        });
        return true;
    }

    for (var m = 0; m < profile.Missions.length; ++m) {
        var current = profile.Missions[m];
        var definition = DAILY_MISSIONS[m];
        current.Description = definition.Description;
        current.Target = definition.Target;
        current.RewardCoins = definition.RewardCoins;
        current.Progress = Math.max(0, Math.min(definition.Target, Math.floor(Number(current.Progress) || 0)));
        current.Claimed = current.Claimed === true;
    }
    return false;
}

function addMissionProgress(profile, missionId, amount) {
    if (amount <= 0) return;
    for (var i = 0; i < profile.Missions.length; ++i) {
        var mission = profile.Missions[i];
        if (mission.Id === missionId && !mission.Claimed) {
            mission.Progress = Math.min(mission.Target, mission.Progress + amount);
            return;
        }
    }
}

// A previously settled PlayFab response needs no new debit, credit or RPC.
function replaySavedSpinResponse(response, expectedVersion) {
    if (response && (response.RulesVersion === 3 || (expectedVersion === 5 && response.RulesVersion === 4))) {
        var replay = JSON.parse(JSON.stringify(response));
        replay.RulesVersion = expectedVersion;
        replay.RecoveredLegacyReceipt = true;
        return replay;
    }
    return response;
}

function isRecoveredLegacyReceipt(outcome, expectedVersion) {
    return outcome && (outcome.RulesVersion === 3 || (expectedVersion === 5 && outcome.RulesVersion === 4)) &&
        outcome.ReceiptRuntimeRulesVersion === expectedVersion && outcome.RecoveredLegacyReceipt === true;
}

// Report contract failures without exposing gateway URLs, keys or player data.
function rejectSpinResult(game, outcome) {
    outcome = outcome || {};
    var shape = game === "Neon Nights" ? "Grid" : "Stops";
    var expected = shape === "Grid" ? 20 : 5;
    var issues = [];
    if (!Array.isArray(outcome[shape]) || outcome[shape].length !== expected)
        issues.push(shape + " length=" + (Array.isArray(outcome[shape]) ? outcome[shape].length : "missing") + " expected=" + expected);
    ["Win", "BaseWin", "ScatterCount", "FreeSpinCount"].forEach(function (key) {
        if (typeof outcome[key] !== "number") issues.push(key + "=" + typeof outcome[key]);
    });
    var expectedVersion = game === "Neon Nights" ? 5 : 4;
    if (outcome.RulesVersion !== expectedVersion) issues.push("RulesVersion=" + String(outcome.RulesVersion) + " expected=" + expectedVersion);
    if (!outcome.Jackpots) issues.push("Jackpots missing");
    throw new Error("Nakama " + game + " result rejected: " + issues.join("; ") +
        ". A saved older spin receipt can survive a runtime restart; do not delete receipts or reroll the request.");
}

function currentProfile() {
    if (!currentPlayerId) throw new Error("Authenticated player ID required.");
    var saved=store.read();
    if (saved) {
        if (typeof saved.Coins !== "number" || typeof saved.XP !== "number")
            throw new Error("Saved server profile is malformed; leaving it untouched.");
        return saved;
    }
    // Used by isolated rule tests; production migration is validated by the coordinator.
    return currentPlayerId === LEGACY_PLAYER_ID
        ? { Coins:10175950, XP:400, DailyBonusLastClaimDate:"2026-10-25", DailyBonusStreakDay:2, SchemaVersion:1, BonusState:null }
        : { Coins:10050000, XP:0, DailyBonusLastClaimDate:"", DailyBonusStreakDay:0, SchemaVersion:1, BonusState:null };
}
function saveProfile(profile) { store.write(profile); }

function publicProfile(profile, jackpots) {
    var pending = null;
    if (profile.BonusState) {
        var revealed = [];
        for (var i = 0; i < profile.BonusState.Revealed.length; ++i) {
            if (profile.BonusState.Revealed[i]) {
                revealed.push({ Card:i, Prize:profile.BonusState.Prizes[i] });
            }
        }
        pending = {
            ScatterCount:profile.BonusState.ScatterCount,
            PicksRemaining:profile.BonusState.PicksRemaining,
            Total:profile.BonusState.Total,
            Revealed:revealed
        };
    }
    var result = {
        Coins:profile.Coins,
        XP:profile.XP,
        DailyBonusLastClaimDate:profile.DailyBonusLastClaimDate || "",
        DailyBonusStreakDay:profile.DailyBonusStreakDay || 0,
        MissionsDate:profile.MissionsDate || "",
        Missions:(profile.Missions || []).map(function (mission) {
            return { Id:mission.Id, Description:mission.Description, Target:mission.Target,
                Progress:mission.Progress, RewardCoins:mission.RewardCoins, Claimed:mission.Claimed === true };
        }),
        PendingBonus:pending,
        MegaPot:profile.MegaPot || MegaPotRules.create(),
        NeonHeatBonus:publicNeonHeatBonus(profile.NeonHeatBonus),
        FreeSpins:{ SugarLust:freeSpinState(profile, "SugarLust"), NeonNights:freeSpinState(profile, "NeonNights") }
    };
    if (jackpots) result.Jackpots = jackpots;
    return result;
}

function publicNeonHeatBonus(state) {
    if (!state || state.Active !== true) return null;
    return {
        Active:true,
        Bet:state.Bet,
        TotalBet:state.TotalBet,
        RespinsRemaining:state.RespinsRemaining,
        Total:state.Total,
        Locked:(state.Locked || []).map(function (entry) {
            return { Index:entry.Index, Prize:entry.Prize };
        })
    };
}

function neonHeatPrize(totalBet) {
    var multipliers = [1,2,3,5,10,25];
    return totalBet * multipliers[Math.floor(Math.random() * multipliers.length)];
}


// Each game's feature keeps its own locked wager and remaining spins in the
// trusted player profile. The client cannot grant spins or choose a free wager.
function freeSpinState(profile, game) {
    if (!profile.FreeSpins) profile.FreeSpins = {};
    if (!profile.FreeSpins[game]) profile.FreeSpins[game] = {
        Remaining:0, BetPerLine:0, ActiveLines:0, TotalWin:0,
        TotalAwarded:0, LastAward:0, LastWasFreeSpin:false
    };
    return profile.FreeSpins[game];
}

function settleFreeSpin(profile, game, wasFree, bet, lines, win, symbolCount) {
    var state = freeSpinState(profile, game);
    if (!wasFree) {
        state.Remaining = 0; state.TotalWin = 0; state.TotalAwarded = 0;
    } else {
        state.Remaining -= 1;
        state.TotalWin += win;
    }
    var award = symbolCount >= 5 ? 20 : symbolCount === 4 ? 15 : symbolCount === 3 ? 10 : 0;
    // Retriggers extend the current feature; they do not start another full round.
    if (wasFree) award = symbolCount >= 3 ? 5 : 0;
    state.LastAward = Math.min(award, Math.max(0, 50 - state.TotalAwarded), Math.max(0, 100 - state.Remaining));
    state.Remaining += state.LastAward;
    state.TotalAwarded += state.LastAward;
    state.LastWasFreeSpin = wasFree;
    state.BetPerLine = bet;
    state.ActiveLines = lines;
    return state;
}

function requireInteger(value, min, max, label) {
    var number = Number(value);
    if (!isFinite(number) || Math.floor(number) !== number || number < min || number > max) {
        throw new Error("Invalid " + label + ".");
    }
    return number;
}

handlers.GetOrMigratePlayerProfile = function (args, context) {
    var profile = currentProfile();
    var missionsChanged = ensureDailyMissions(profile);
    var jackpots = callNakama({ Operation:"Get" }).Jackpots;
    saveProfile(profile);
    return publicProfile(profile, jackpots);
};

handlers.GetProgressiveJackpots = function (args, context) {
    return callNakama({ Operation:"Get" });
};

handlers.Spin = function (args, context) {
    args = args || {};
    var profile = currentProfile();
    ensureDailyMissions(profile);
    var spinId = String(args.SpinId || "");
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(spinId)) throw new Error("Invalid spin identifier.");
    if (profile.LastSpin && profile.LastSpin.SpinId === spinId && profile.LastSpin.Response) {
        return replaySavedSpinResponse(profile.LastSpin.Response, 4);
    }
    if (profile.BonusState) throw new Error("Finish the pending bonus before spinning again.");

    var freeState = freeSpinState(profile, "SugarLust");
    var wasFree = freeState.Remaining > 0;
    var betPerLine = wasFree ? freeState.BetPerLine : requireInteger(args.BetPerLine, 1, 10000, "bet");
    var activeLines = wasFree ? freeState.ActiveLines : requireInteger(args.ActiveLines, 1, 20, "active lines");
    var totalBet = betPerLine * activeLines;
    if (!wasFree && profile.Coins < totalBet) throw new Error("Not enough coins for that bet.");

    // Nakama atomically creates reel stops, evaluates payouts, contributes to
    // the shared pools, and records an idempotent receipt for this PlayFab spin.
    var outcome = callNakama({
        Operation:"Spin", PlayFabId:currentPlayerId, SpinId:spinId,
        BetPerLine:betPerLine, ActiveLines:activeLines, FreeSpin:wasFree
    });
    if (!outcome.Stops || outcome.Stops.length !== 5 || typeof outcome.Win !== "number" ||
        typeof outcome.BaseWin !== "number" || typeof outcome.ScatterCount !== "number" ||
        (outcome.RulesVersion !== 4 && !isRecoveredLegacyReceipt(outcome, 4)) || typeof outcome.FreeSpinCount !== "number" || !outcome.Jackpots) {
        rejectSpinResult("Sugar Lust", outcome);
    }
    var stops = outcome.Stops;
    var totalWin = outcome.Win;
    profile.Coins = profile.Coins - (wasFree ? 0 : totalBet) + totalWin;
    settleFreeSpin(profile, "SugarLust", wasFree, betPerLine, activeLines, totalWin, outcome.FreeSpinCount);
    var xpAwarded = Math.min(250, 10 + Math.floor(totalBet / 1000));
    profile.XP += xpAwarded;
    addMissionProgress(profile, "spin_10", 1);
    addMissionProgress(profile, "win_50000", totalWin);
    addMissionProgress(profile, "scatter_3", outcome.ScatterCount >= 3 ? 1 : 0);
    addMissionProgress(profile, "bet_high", !wasFree && totalBet >= 100000 ? 1 : 0);
    addMissionProgress(profile, "level_up", xpAwarded);

    if (outcome.ScatterCount >= 3) {
        var multipliers = BONUS_MULTIPLIERS.slice();
        for (var i = multipliers.length - 1; i > 0; --i) {
            var j = Math.floor(Math.random() * (i + 1));
            var swap = multipliers[i]; multipliers[i] = multipliers[j]; multipliers[j] = swap;
        }
        profile.BonusState = {
            ScatterCount:outcome.ScatterCount, FreeSpinFeature:wasFree,
            Prizes:multipliers.map(function (multiplier) { return multiplier * totalBet; }),
            Revealed:[false,false,false,false,false,false],
            PicksRemaining:3,
            Total:0
        };
    }

    var response = {
        Stops:stops, Win:totalWin, BaseWin:outcome.BaseWin, JackpotWin:outcome.JackpotWin,
        ScatterCount:outcome.ScatterCount, FreeSpinCount:outcome.FreeSpinCount, RulesVersion:4,
        RecoveredLegacyReceipt:isRecoveredLegacyReceipt(outcome, 4),
        Profile:publicProfile(profile, outcome.Jackpots)
    };
    // If the client retries after a network interruption, replay this result
    // instead of charging the wager or applying XP twice.
    profile.LastSpin = { SpinId:spinId, Response:response };
    saveProfile(profile);
    return response;
};

// Neon Nights is a separate 5x4 all-ways game. Its rules and symbol IDs are
// evaluated by the Neon rules; the account is committed by the outer transaction.
handlers.NeonNightsSpin = function (args, context) {
    args = args || {};
    var profile = currentProfile();
    ensureDailyMissions(profile);
    var spinId = String(args.SpinId || "");
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(spinId)) throw new Error("Invalid Neon Nights spin identifier.");
    if (profile.LastNeonSpin && profile.LastNeonSpin.SpinId === spinId && profile.LastNeonSpin.Response) {
        return replaySavedSpinResponse(profile.LastNeonSpin.Response, 5);
    }
    var freeState = freeSpinState(profile, "NeonNights");
    var wasFree = freeState.Remaining > 0;
    var bet = wasFree ? freeState.BetPerLine : requireInteger(args.Bet, 100, 10000, "Neon Nights bet");
    var totalBet = bet * 5;

    // Hold-and-spin respins are server-authoritative and free. The wager and
    // held prizes come from the saved trigger state, never from the client.
    if (profile.NeonHeatBonus && profile.NeonHeatBonus.Active === true) {
        var bonus = profile.NeonHeatBonus;
        var lockedLookup = {};
        var bonusGrid = new Array(20);
        for (var cellIndex = 0; cellIndex < 20; ++cellIndex) bonusGrid[cellIndex] = Math.floor(Math.random() * 11);
        for (var heldIndex = 0; heldIndex < bonus.Locked.length; ++heldIndex) {
            var held = bonus.Locked[heldIndex];
            lockedLookup[held.Index] = true;
            bonusGrid[held.Index] = 12;
        }

        var newSymbols = 0;
        for (var openCell = 0; openCell < 20; ++openCell) {
            if (lockedLookup[openCell] || Math.random() >= 0.08) continue;
            var prize = neonHeatPrize(bonus.TotalBet);
            bonus.Locked.push({ Index:openCell, Prize:prize });
            lockedLookup[openCell] = true;
            bonusGrid[openCell] = 12;
            bonus.Total += prize;
            ++newSymbols;
        }

        bonus.RespinsRemaining = newSymbols > 0 ? 3 : bonus.RespinsRemaining - 1;
        var completed = bonus.RespinsRemaining <= 0 || bonus.Locked.length >= 20;
        var bonusWin = 0;
        if (completed) {
            if (bonus.Locked.length >= 20) bonus.Total += bonus.TotalBet * 100;
            bonusWin = bonus.Total;
            profile.Coins += bonusWin;
            if (bonus.FreeSpinFeature) freeSpinState(profile, "NeonNights").TotalWin += bonusWin;
            profile.NeonHeatBonus = null;
        }

        var jackpotsDuringBonus = callNakama({ Operation:"Get" }).Jackpots;
        var bonusResponse = {
            Grid:bonusGrid, Win:bonusWin, BaseWin:0, JackpotWin:null,
            ScatterCount:newSymbols, FreeSpinCount:0, RulesVersion:5,
            Bonus:{ Active:!completed, Complete:completed, Triggered:false,
                RespinsRemaining:completed ? 0 : bonus.RespinsRemaining,
                Total:bonus.Total, Locked:bonus.Locked.map(function (entry) {
                    return { Index:entry.Index, Prize:entry.Prize };
                }) },
            Profile:publicProfile(profile, jackpotsDuringBonus)
        };
        profile.LastNeonSpin = { SpinId:spinId, Response:bonusResponse };
        saveProfile(profile);
        return bonusResponse;
    }

    if (!wasFree && profile.Coins < totalBet) throw new Error("Not enough coins for that Neon Nights bet.");

    var outcome = callNakama({
        Operation:"NeonSpin", PlayFabId:currentPlayerId, SpinId:spinId,
        Bet:bet, FreeSpin:wasFree
    });
    if (!outcome.Grid || outcome.Grid.length !== 20 || typeof outcome.Win !== "number" ||
        typeof outcome.BaseWin !== "number" || typeof outcome.ScatterCount !== "number" ||
        (outcome.RulesVersion !== 5 && !isRecoveredLegacyReceipt(outcome, 5)) || typeof outcome.FreeSpinCount !== "number" || !outcome.Jackpots) {
        rejectSpinResult("Neon Nights", outcome);
    }

    profile.Coins = profile.Coins - (wasFree ? 0 : totalBet) + outcome.Win;
    settleFreeSpin(profile, "NeonNights", wasFree, bet, 5, outcome.Win, outcome.FreeSpinCount);
    var xpAwarded = Math.min(250, 10 + Math.floor(totalBet / 1000));
    profile.XP += xpAwarded;
    addMissionProgress(profile, "spin_10", 1);
    addMissionProgress(profile, "win_50000", outcome.Win);
    addMissionProgress(profile, "bet_high", !wasFree && totalBet >= 100000 ? 1 : 0);
    addMissionProgress(profile, "scatter_3", outcome.ScatterCount >= 3 ? 1 : 0);

    var bonusResult = { Active:false, Complete:false, Triggered:false, RespinsRemaining:0, Total:0, Locked:[] };
    if (outcome.ScatterCount >= 3) {
        var locked = [];
        var bonusTotal = 0;
        for (var scatterIndex = 0; scatterIndex < outcome.Grid.length; ++scatterIndex) {
            if (outcome.Grid[scatterIndex] !== 12) continue;
            var scatterPrize = neonHeatPrize(totalBet);
            locked.push({ Index:scatterIndex, Prize:scatterPrize });
            bonusTotal += scatterPrize;
        }
        profile.NeonHeatBonus = {
            Active:true, Bet:bet, TotalBet:totalBet, RespinsRemaining:3, FreeSpinFeature:wasFree,
            Total:bonusTotal, Locked:locked
        };
        bonusResult = { Active:true, Complete:false, Triggered:true, RespinsRemaining:3,
            Total:bonusTotal, Locked:locked.map(function (entry) { return { Index:entry.Index, Prize:entry.Prize }; }) };
    }

    var response = {
        Grid:outcome.Grid, Win:outcome.Win, BaseWin:outcome.BaseWin,
        JackpotWin:outcome.JackpotWin, ScatterCount:outcome.ScatterCount,
        FreeSpinCount:outcome.FreeSpinCount, RulesVersion:5,
        RecoveredLegacyReceipt:isRecoveredLegacyReceipt(outcome, 5),
        Bonus:bonusResult,
        Profile:publicProfile(profile, outcome.Jackpots)
    };
    profile.LastNeonSpin = { SpinId:spinId, Response:response };
    saveProfile(profile);
    return response;
};

handlers.ClaimMissionReward = function (args, context) {
    args = args || {};
    var profile = currentProfile();
    var missionsReset = ensureDailyMissions(profile);
    if (missionsReset) {
        saveProfile(profile);
        return { Reward:0, AlreadyClaimed:false, MissionsReset:true, Profile:publicProfile(profile) };
    }
    var missionId = String(args.MissionId || "");
    var claimId = String(args.ClaimId || "");
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(claimId)) throw new Error("Invalid mission claim identifier.");

    if (profile.LastMissionClaim && profile.LastMissionClaim.ClaimId === claimId &&
        profile.LastMissionClaim.MissionId === missionId) {
        return profile.LastMissionClaim.Response;
    }

    var mission = null;
    for (var i = 0; i < profile.Missions.length; ++i) {
        if (profile.Missions[i].Id === missionId) { mission = profile.Missions[i]; break; }
    }
    if (!mission) throw new Error("Unknown mission.");
    if (mission.Claimed) {
        return { Reward:0, AlreadyClaimed:true, Profile:publicProfile(profile) };
    }
    if (mission.Progress < mission.Target) throw new Error("Mission is not complete yet.");

    mission.Claimed = true;
    profile.Coins += mission.RewardCoins;
    var response = { Reward:mission.RewardCoins, AlreadyClaimed:false, Profile:publicProfile(profile) };
    profile.LastMissionClaim = { ClaimId:claimId, MissionId:missionId, Response:response };
    saveProfile(profile);
    return response;
};

handlers.PickBonusCard = function (args, context) {
    args = args || {};
    var profile = currentProfile();
    ensureDailyMissions(profile);
    var state = profile.BonusState;
    if (!state) throw new Error("There is no active bonus round.");
    var card = requireInteger(args.Card, 0, 5, "card");
    if (state.Revealed[card]) throw new Error("That bonus card was already selected.");

    var prize = state.Prizes[card];
    state.Revealed[card] = true;
    state.PicksRemaining -= 1;
    state.Total += prize;
    if (state.PicksRemaining === 0) {
        profile.Coins += state.Total;
        if (state.FreeSpinFeature) freeSpinState(profile, "SugarLust").TotalWin += state.Total;
        addMissionProgress(profile, "win_50000", state.Total);
        profile.BonusState = null;
    }
    saveProfile(profile);
    return { Card:card, Prize:prize, PicksRemaining:state.PicksRemaining, BonusTotal:state.Total, Profile:publicProfile(profile) };
};

handlers.ClaimDailyBonus = function (args, context) {
    var profile = currentProfile();
    ensureDailyMissions(profile);
    if (profile.BonusState) throw new Error("Finish the pending bonus before claiming the daily bonus.");

    var today = new Date().toISOString().slice(0, 10);
    if (profile.DailyBonusLastClaimDate === today) throw new Error("Today's daily bonus has already been claimed.");
    var yesterday = new Date(Date.now() - 86400000).toISOString().slice(0, 10);
    var streakDay = profile.DailyBonusLastClaimDate === yesterday
        ? (profile.DailyBonusStreakDay % 7) + 1
        : 1;
    var rewards = [50000,75000,100000,150000,200000,300000,500000];
    var reward = rewards[streakDay - 1];
    profile.Coins += reward;
    profile.DailyBonusLastClaimDate = today;
    profile.DailyBonusStreakDay = streakDay;
    saveProfile(profile);
    return { Reward:reward, StreakDay:streakDay, Profile:publicProfile(profile) };
};

handlers.MegaLustPotState=function(args) {
    var profile=currentProfile();
    if(!profile.MegaPot)profile.MegaPot=MegaPotRules.create();
    MegaPotRules.validate(profile.MegaPot);profile.MegaPot.Revision++;saveProfile(profile);
    return {RulesVersion:1,Synchronized:true,State:profile.MegaPot,Profile:publicProfile(profile)};
};
handlers.MegaLustPotSpin=function(args) {
    args=args || {};var profile=currentProfile();ensureDailyMissions(profile);
    var state=profile.MegaPot || MegaPotRules.create();MegaPotRules.validate(state);
    if(typeof args.SpinId!=="string" || !/^[A-Za-z0-9_-]{1,64}$/.test(args.SpinId))throw new Error("Invalid Mega Pot spin identifier.");
    if(args.ExpectedRevision!==state.Revision)throw new Error("Mega Pot state changed. Refresh before spinning.");
    if(typeof args.Bet!=="number" || Math.floor(args.Bet)!==args.Bet || args.Bet<1 || args.Bet>100)throw new Error("Invalid Mega Pot bet.");
    var paid=state.Phase!=="Hold" && state.Free===0;
    var locked=state.Phase==="Hold" || state.Free>0 || state.Royal>0;
    var bet=locked?state.Bet:args.Bet;
    if(paid && profile.Coins<bet*20)throw new Error("Not enough coins for that Mega Pot bet.");
    var outcome=MegaPotRules.step(state,args.Bet,Math.random);
    profile.Coins+=outcome.Win-outcome.Debit;
    if(outcome.Debit>0){profile.XP+=Math.max(10,Math.floor(outcome.Debit/100));addMissionProgress(profile,"spin_10",1);if(outcome.Debit>=1000)addMissionProgress(profile,"bet_high",1);}
    addMissionProgress(profile,"win_50000",outcome.Win);
    if(!isFinite(profile.XP)||Math.floor(profile.XP)!==profile.XP||profile.XP>9007199254740991)throw new Error("Mega Pot XP exceeds safe integer range.");
    profile.MegaPot=state;saveProfile(profile);
    return {RulesVersion:1,SpinId:args.SpinId,Kind:outcome.Kind,Debit:outcome.Debit,Win:outcome.Win,Triggered:outcome.Triggered,Lines:outcome.Lines,State:state,Profile:publicProfile(profile)};
};

handlers.__snapshot = function(jackpots) { return publicProfile(currentProfile(),jackpots); };
return handlers;
}

