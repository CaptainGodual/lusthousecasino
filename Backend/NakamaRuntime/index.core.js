// Sugar Lust authoritative spin and progressive jackpot RPC for Nakama's
// embedded JavaScript runtime (ES5-compatible; no Node.js APIs).

var SEEDS = { Mini:125689745, Minor:689745321, Major:1208745321, Grand:2809689745 };
var RATE_PERMILLE = { Mini:4, Minor:3, Major:2, Grand:1 };
var JACKPOT_TARGET_RATES = { Mini:1/7000, Minor:1/30000, Major:1/100000, Grand:1/300000 };
var COLLECTION = "sugar_lust";
var JACKPOT_KEY = "progressive_jackpots";
var RECEIPTS_KEY_PREFIX = "spin_receipts_";
var RECEIPT_LIMIT = 20;
var SYSTEM_USER_ID = "00000000-0000-0000-0000-000000000000";
var NEON_RECEIPTS_KEY_PREFIX = "neon_spin_receipts_";
var SLOT_RULES_VERSION = 4;
var NEON_RULES_VERSION = 5;
var NEON_BASE_STRIP = [0,1,2,3,4,5,6,7,8,9,10,0,1,2,3,4,5,6,7,8,9,10,0,1,2,3,4,5,6,7,11,11,12,0,8,9,10,5,6,7,13,0,1,13];
var NEON_REEL_STRIPS = [];
for (var neonReel = 0; neonReel < 5; ++neonReel) {
    var neonStrip = [];
    for (var neonIndex = 0; neonIndex < NEON_BASE_STRIP.length; ++neonIndex)
        neonStrip.push(NEON_BASE_STRIP[(neonIndex + neonReel * 7) % NEON_BASE_STRIP.length]);
    NEON_REEL_STRIPS.push(neonStrip);
}
// Symbol IDs are appended, preserving every existing feature ID.
var NEON_JACKPOT_POSITIONS = [[3,7,11,15],[0,4,8,12],[0,4,8,12],[0,4,15,26],[0,8,19,23]];
for (var nj = 0; nj < 5; ++nj)
    for (var nt = 0; nt < 4; ++nt) NEON_REEL_STRIPS[nj][NEON_JACKPOT_POSITIONS[nj][nt]] = 14 + nt;
// Neon Muse is appended as ID 18. Replace one ordinary Ace per strip;
// strip length and Scatter/Free Spins/jackpot positions remain unchanged.
for (var museReel = 0; museReel < 5; ++museReel) {
    var musePosition = NEON_REEL_STRIPS[museReel].indexOf(0);
    if (musePosition < 0) throw new Error("Neon Muse requires an ordinary symbol position.");
    NEON_REEL_STRIPS[museReel][musePosition] = 18;
}
var NEON_SYMBOL_MULTIPLIERS = [1,1,1,1,1,2,2,3,4,3,3,0,0,0,0,0,0,0,5];

function calculateNeonWin(grid, bet) {
    var totalBet = bet * 5;
    var total = 0;
    var scatters = 0, freeSpinSymbols = 0, jackpotCounts = [0,0,0,0];
    for (var i = 0; i < grid.length; ++i) {
        if (grid[i] === 12) ++scatters;
        if (grid[i] === 13) ++freeSpinSymbols;
        if (grid[i] >= 14 && grid[i] <= 17) ++jackpotCounts[grid[i] - 14];
    }

    // Ways are evaluated left-to-right. Each reel contributes every matching
    // symbol position; Wild substitutes for any paying symbol.
    var lengthMultipliers = [2,6,20];
    for (var symbol = 0; symbol < NEON_SYMBOL_MULTIPLIERS.length; ++symbol) {
        if (NEON_SYMBOL_MULTIPLIERS[symbol] <= 0) continue;
        var ways = 1;
        var length = 0;
        for (var reel = 0; reel < 5; ++reel) {
            var count = 0;
            for (var row = 0; row < 4; ++row) {
                var cell = grid[row * 5 + reel];
                if (cell === symbol || cell === 11) ++count;
            }
            if (count === 0) break;
            ways *= count;
            ++length;
        }
        if (length >= 3) {
            total += bet * lengthMultipliers[length - 3] * NEON_SYMBOL_MULTIPLIERS[symbol] * ways;
        }
    }
    if (scatters >= 3) {
        var scatterMultiplier = scatters === 3 ? 2 : scatters === 4 ? 10 : 50;
        total += totalBet * scatterMultiplier;
    }
    return { Win:total, ScatterCount:scatters, FreeSpinCount:freeSpinSymbols, JackpotCounts:jackpotCounts };
}

var FIRST_HALF_STRIPS = [
    [0,10,9,11,3,12,12,2,13,13,4,8,3,7,0],
    [9,2,1,3,14,14,11,10,15,7,1,12,15,4,8],
    [9,3,2,4,2,1,10,11,7,2,16,16,1,2,8],
    [10,9,3,5,3,2,11,1,14,13,7,15,4,6,6],
    [4,10,9,4,5,7,4,3,6,13,1,15,8,16,7]
];
var REEL_STRIPS = FIRST_HALF_STRIPS.map(function (first) {
    var strip = first.slice();
    for (var i = 0; i < first.length; ++i) strip.push(first[(i + Math.floor(first.length / 2)) % first.length]);
    return strip;
});
var FREE_SPIN_POSITIONS = [5,4,10,8,9];
for (var fsReel = 0; fsReel < 5; ++fsReel) {
    var fsPosition = FREE_SPIN_POSITIONS[fsReel];
    REEL_STRIPS[fsReel][fsPosition] = 17;
    REEL_STRIPS[fsReel][15 + (fsPosition + 8) % 15] = 17;
}
// One Scatter per 30-stop reel. Remove the doubled copies; reel four
// previously had none, so give it one without changing Free Spins symbols.
for (var scatterReel = 0; scatterReel < 5; ++scatterReel) {
    var foundScatter = false;
    for (var scatterStop = 0; scatterStop < REEL_STRIPS[scatterReel].length; ++scatterStop) {
        if (REEL_STRIPS[scatterReel][scatterStop] !== 8) continue;
        if (foundScatter) REEL_STRIPS[scatterReel][scatterStop] = 5;
        else foundScatter = true;
    }
    if (!foundScatter) REEL_STRIPS[scatterReel][13] = 8;
}
var SUGAR_JACKPOT_POSITIONS = [[1,8,14,17],[0,7,10,18],[0,3,6,21],[0,3,19,22],[0,3,6,20]];
for (var sj = 0; sj < 5; ++sj)
    for (var st = 0; st < 4; ++st) REEL_STRIPS[sj][SUGAR_JACKPOT_POSITIONS[sj][st]] = 18 + st;
var PAYLINES = [
    [1,1,1,1,1], [0,0,0,0,0], [2,2,2,2,2], [0,1,2,1,0], [2,1,0,1,2],
    [0,0,1,2,2], [2,2,1,0,0], [1,0,0,0,1], [1,2,2,2,1], [0,1,1,1,0],
    [2,1,1,1,2], [0,0,1,1,1], [2,2,1,1,1], [1,1,1,0,0], [1,1,1,2,2],
    [0,1,0,1,0], [2,1,2,1,2], [0,2,1,0,2], [2,0,1,2,0], [1,0,1,0,1]
];
var PAYOUTS = [
    [5,20,100], [10,50,250], [15,75,400], [20,100,500], [25,125,750],
    [30,150,1000], [40,200,1500], [100,500,2500], [0,0,0], [45,225,1800], [15,75,400], [10,50,250],
    [2,10,50], [3,15,75], [4,20,100], [5,25,125], [6,30,150]
];

function parseJsonValue(value, description) {
    if (typeof value === "string") {
        try { return JSON.parse(value); }
        catch (error) { throw new Error("Malformed " + description + " in Nakama storage."); }
    }
    return value;
}

function seedJackpots() {
    return {
        Mini:SEEDS.Mini, Minor:SEEDS.Minor, Major:SEEDS.Major, Grand:SEEDS.Grand,
        Remainders:{ Mini:0, Minor:0, Major:0, Grand:0 }, SchemaVersion:2
    };
}

function validateJackpots(state) {
    var tiers = ["Mini", "Minor", "Major", "Grand"];
    if (!state || !state.Remainders) throw new Error("Progressive jackpot data is malformed.");
    for (var i = 0; i < tiers.length; ++i) {
        var tier = tiers[i];
        if (typeof state[tier] !== "number" || !isFinite(state[tier]) ||
            Math.floor(state[tier]) !== state[tier] || state[tier] < SEEDS[tier] ||
            typeof state.Remainders[tier] !== "number" || !isFinite(state.Remainders[tier]) ||
            Math.floor(state.Remainders[tier]) !== state.Remainders[tier] ||
            state.Remainders[tier] < 0 || state.Remainders[tier] >= 1000) {
            throw new Error("Progressive jackpot data is malformed.");
        }
    }
    if (state.SchemaVersion !== 2) state.SchemaVersion = 2;
    return state;
}

function publicJackpots(state) {
    return { Mini:state.Mini, Minor:state.Minor, Major:state.Major, Grand:state.Grand };
}

function requireInteger(value, min, max, label) {
    var number = Number(value);
    if (!isFinite(number) || Math.floor(number) !== number || number < min || number > max) {
        throw new Error("Invalid " + label + ".");
    }
    return number;
}

function calculateWin(stops, betPerLine, activeLines) {
    if (!stops || stops.length !== 5) throw new Error("Invalid reel stops.");
    var window = [];
    for (var reel = 0; reel < 5; ++reel) {
        var stop = requireInteger(stops[reel], 0, REEL_STRIPS[reel].length - 1, "reel stop");
        window[reel] = [];
        for (var row = 0; row < 3; ++row) window[reel][row] = REEL_STRIPS[reel][(stop + row) % REEL_STRIPS[reel].length];
    }

    var total = 0;
    for (var lineIndex = 0; lineIndex < activeLines; ++lineIndex) {
        var line = PAYLINES[lineIndex];
        if (window[0][line[0]] === 8 || window[0][line[0]] >= 17) continue;
        var paying = window[0][line[0]];
        if (paying === 7) {
            for (var next = 1; next < 5; ++next) {
                var candidate = window[next][line[next]];
                if (candidate === 8 || candidate >= 17) break;
                if (candidate !== 7) { paying = candidate; break; }
            }
        }
        var matches = 0;
        for (var reelIndex = 0; reelIndex < 5; ++reelIndex) {
            var symbol = window[reelIndex][line[reelIndex]];
            if (symbol === 8 || symbol >= 17 || (symbol !== 7 && symbol !== paying)) break;
            ++matches;
        }
        if (matches >= 3) {
            var usedWild = false;
            for (var highlightReel = 0; highlightReel < matches; ++highlightReel) {
                usedWild = usedWild || window[highlightReel][line[highlightReel]] === 7;
            }
            total += PAYOUTS[paying][matches - 3] * betPerLine * (usedWild ? 2 : 1);
        }
    }

    var scatters = 0, freeSpinSymbols = 0, jackpotCounts = [0,0,0,0];
    for (var r = 0; r < 5; ++r) for (var y = 0; y < 3; ++y) {
        if (window[r][y] === 8) ++scatters;
        if (window[r][y] === 17) ++freeSpinSymbols;
        if (window[r][y] >= 18 && window[r][y] <= 21) ++jackpotCounts[window[r][y] - 18];
    }
    if (scatters >= 3) total += (scatters === 3 ? 5 : scatters === 4 ? 20 : 100) * betPerLine * activeLines;
    return { Win:total, ScatterCount:scatters, FreeSpinCount:freeSpinSymbols, JackpotCounts:jackpotCounts };
}

function applyContribution(state, totalBet) {
    var tiers = ["Mini", "Minor", "Major", "Grand"];
    for (var i = 0; i < tiers.length; ++i) {
        var tier = tiers[i];
        var milliCoins = totalBet * RATE_PERMILLE[tier] + state.Remainders[tier];
        var wholeCoins = Math.floor(milliCoins / 1000);
        state[tier] += wholeCoins;
        state.Remainders[tier] = milliCoins % 1000;
    }
}

// Each reel independently selects its visible jackpot tier. A win is awarded
// only from 3+ actual matching symbols, with no extra mystery-jackpot roll.
function awardSymbolJackpot(state, counts) {
    var tiers = ["Mini", "Minor", "Major", "Grand"];
    for (var i = 3; i >= 0; --i) {
        if (counts[i] < 3) continue;
        var tier = tiers[i], prize = state[tier];
        state[tier] = SEEDS[tier];
        state.Remainders[tier] = 0;
        return { Tier:tier, Prize:prize, Count:counts[i] };
    }
    return null;
}

function jackpotColumnChance(rate) {
    var low = 0, high = 1;
    for (var i = 0; i < 70; ++i) {
        var q = (low + high) / 2;
        var chance = 10*q*q*q*(1-q)*(1-q) + 5*q*q*q*q*(1-q) + q*q*q*q*q;
        if (chance < rate) low = q; else high = q;
    }
    return (low + high) / 2;
}

function jackpotStopWeights(strips, rows, scatter, freeSpin, firstJackpot) {
    var tiers = ["Mini", "Minor", "Major", "Grand"];
    var tierChances = tiers.map(function(tier) { return jackpotColumnChance(JACKPOT_TARGET_RATES[tier]); });
    var jackpotMass = tierChances.reduce(function(a,b) { return a+b; }, 0);
    return strips.map(function(strip) {
        var types = [], protectedStops = 0, ordinaryStops = 0;
        var tierStops = [0,0,0,0];
        for (var stop = 0; stop < strip.length; ++stop) {
            var protectedFeature = false, tier = -1;
            for (var row = 0; row < rows; ++row) {
                var symbol = strip[(stop + row) % strip.length];
                if (symbol === scatter || symbol === freeSpin) protectedFeature = true;
                if (symbol >= firstJackpot && symbol < firstJackpot + 4) {
                    if (tier !== -1) throw new Error("Overlapping jackpot windows.");
                    tier = symbol - firstJackpot;
                }
            }
            if (protectedFeature && tier !== -1) throw new Error("Jackpot symbol overlaps a Scatter/Free Spins window.");
            var type = protectedFeature ? -2 : tier;
            types.push(type);
            if (type === -2) ++protectedStops;
            else if (type === -1) ++ordinaryStops;
            else ++tierStops[type];
        }
        var ordinaryMass = 1 - protectedStops / strip.length - jackpotMass;
        if (ordinaryMass <= 0 || ordinaryStops === 0) throw new Error("Invalid jackpot weighting.");
        // Retain the exact probabilities of every Scatter/Free Spins window.
        // Jackpot probability comes from ordinary windows only.
        return types.map(function(type) {
            if (type === -2) return 1 / strip.length;
            if (type === -1) return ordinaryMass / ordinaryStops;
            return tierChances[type] / tierStops[type];
        });
    });
}
var SUGAR_STOP_WEIGHTS = jackpotStopWeights(REEL_STRIPS, 3, 8, 17, 18);
var NEON_STOP_WEIGHTS = jackpotStopWeights(NEON_REEL_STRIPS, 4, 12, 13, 14);

function chooseWeightedStop(weights) {
    var roll = Math.random();
    for (var stop = 0; stop < weights.length; ++stop) {
        roll -= weights[stop];
        if (roll < 0) return stop;
    }
    return weights.length - 1;
}

// Only this current runtime can attest that a version-3 result was replayed
// from storage. Never reroll it, recalculate its payout, or rewrite its receipt.
function serializeSpinReceipt(result, expectedVersion) {
    if (result && (result.RulesVersion === 3 || (expectedVersion === 5 && result.RulesVersion === 4))) {
        var replay = JSON.parse(JSON.stringify(result));
        replay.ReceiptRuntimeRulesVersion = expectedVersion;
        replay.RecoveredLegacyReceipt = true;
        return JSON.stringify(replay);
    }
    return JSON.stringify(result);
}

function rpcNeonSpin(ctx, nk, request) {
    var playerId = String(request.PlayFabId || "");
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(playerId)) throw new Error("Invalid player identifier.");
    var spinId = String(request.SpinId || "");
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(spinId)) throw new Error("Invalid Neon Nights spin identifier.");
    var bet = requireInteger(request.Bet, 100, 10000, "Neon Nights bet");
    var totalBet = bet * 5;
    var jackpotRead = { collection:COLLECTION, key:JACKPOT_KEY, userId:SYSTEM_USER_ID };
    var receiptKey = NEON_RECEIPTS_KEY_PREFIX + playerId;
    var receiptRead = { collection:COLLECTION, key:receiptKey, userId:SYSTEM_USER_ID };
    var result = null;

    nk.storageWriteRetry([jackpotRead, receiptRead].concat(vendActivityReads(playerId,"neon",spinId)), function (objects) {
        var jackpotObject = vendFind(objects,JACKPOT_KEY);
        var receiptObject = vendFind(objects,receiptKey);
        var receipts = receiptObject ? parseJsonValue(receiptObject.value, "Neon Nights spin receipt") : { Recent:[] };
        if (!receipts || !Array.isArray(receipts.Recent)) throw new Error("Neon Nights receipt data is malformed.");
        for (var i = 0; i < receipts.Recent.length; ++i) {
            if (receipts.Recent[i].SpinId === spinId) {
                result = receipts.Recent[i].Result;
                return [];
            }
        }

        var jackpots = jackpotObject
            ? validateJackpots(parseJsonValue(jackpotObject.value, "jackpot"))
            : seedJackpots();
        var stops = NEON_STOP_WEIGHTS.map(function (weights) { return chooseWeightedStop(weights); });
        var grid = [];
        for (var row = 0; row < 4; ++row) {
            for (var reel = 0; reel < 5; ++reel) {
                var strip = NEON_REEL_STRIPS[reel];
                grid.push(strip[(stops[reel] + row) % strip.length]);
            }
        }
        var outcome = calculateNeonWin(grid, bet);
        if (request.FreeSpin !== true) applyContribution(jackpots, totalBet);
        var jackpotWin = awardSymbolJackpot(jackpots, outcome.JackpotCounts);
        result = {
            Grid:grid,
            Win:outcome.Win + (jackpotWin ? jackpotWin.Prize : 0),
            BaseWin:outcome.Win,
            JackpotWin:jackpotWin,
            ScatterCount:outcome.ScatterCount,
            FreeSpinCount:outcome.FreeSpinCount, RulesVersion:NEON_RULES_VERSION,
            Jackpots:publicJackpots(jackpots)
        };
        receipts.Recent.push({ SpinId:spinId, Result:result });
        while (receipts.Recent.length > RECEIPT_LIMIT) receipts.Recent.shift();
        return vendActivityWrites(objects,playerId,"neon",spinId).concat([
            { collection:COLLECTION, key:JACKPOT_KEY, userId:SYSTEM_USER_ID, value:jackpots,
                version:jackpotObject ? jackpotObject.version : "*", permissionRead:0, permissionWrite:0 },
            { collection:COLLECTION, key:receiptKey, userId:SYSTEM_USER_ID, value:receipts,
                version:receiptObject ? receiptObject.version : "*", permissionRead:0, permissionWrite:0 }
        ]);
    }, 10);
    if (!result) throw new Error("The Neon Nights spin did not produce a result.");
    return serializeSpinReceipt(result, NEON_RULES_VERSION);
}

function rpcProgressive(ctx, logger, nk, payload) {
    if (ctx.userId) throw new Error("This RPC is reserved for server-to-server calls.");

    var request = JSON.parse(payload || "{}");
    var operation = request.Operation;
    if (operation === "LustVend") return rpcLustVend(nk, request);
    var jackpotRead = { collection:COLLECTION, key:JACKPOT_KEY, userId:SYSTEM_USER_ID };

    if (operation === "NeonSpin") return rpcNeonSpin(ctx, nk, request);

    if (operation === "Get") {
        var existing = nk.storageRead([jackpotRead]);
        if (existing.length > 0) {
            return JSON.stringify({ Jackpots:publicJackpots(validateJackpots(parseJsonValue(existing[0].value, "jackpot"))) });
        }

        var seeded = seedJackpots();
        nk.storageWriteRetry([jackpotRead], function (objects) {
            if (objects.length > 0) {
                seeded = validateJackpots(parseJsonValue(objects[0].value, "jackpot"));
                return [];
            }
            return [{ collection:COLLECTION, key:JACKPOT_KEY, userId:SYSTEM_USER_ID, value:seeded,
                version:"*", permissionRead:0, permissionWrite:0 }];
        }, 10);
        return JSON.stringify({ Jackpots:publicJackpots(seeded) });
    }

    if (operation !== "Spin") throw new Error("Unknown progressive jackpot operation.");
    var playerId = String(request.PlayFabId || "");
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(playerId)) throw new Error("Invalid player identifier.");
    var spinId = String(request.SpinId || "");
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(spinId)) throw new Error("Invalid spin identifier.");
    var betPerLine = requireInteger(request.BetPerLine, 1, 10000, "bet per line");
    var activeLines = requireInteger(request.ActiveLines, 1, 20, "active lines");
    var totalBet = betPerLine * activeLines;
    var receiptKey = RECEIPTS_KEY_PREFIX + playerId;
    var receiptRead = { collection:COLLECTION, key:receiptKey, userId:SYSTEM_USER_ID };
    var result = null;

    nk.storageWriteRetry([jackpotRead, receiptRead].concat(vendActivityReads(playerId,"sugar",spinId)), function (objects) {
        var jackpotObject = vendFind(objects,JACKPOT_KEY);
        var receiptObject = vendFind(objects,receiptKey);
        var receipts = receiptObject ? parseJsonValue(receiptObject.value, "spin receipt") : { Recent:[] };
        if (!receipts || !Array.isArray(receipts.Recent)) throw new Error("Spin receipt data is malformed.");

        for (var i = 0; i < receipts.Recent.length; ++i) {
            if (receipts.Recent[i].SpinId === spinId) {
                result = receipts.Recent[i].Result;
                return [];
            }
        }

        var jackpots = jackpotObject
            ? validateJackpots(parseJsonValue(jackpotObject.value, "jackpot"))
            : seedJackpots();
        var stops = SUGAR_STOP_WEIGHTS.map(function (weights) { return chooseWeightedStop(weights); });
        var outcome = calculateWin(stops, betPerLine, activeLines);
        if (request.FreeSpin !== true) applyContribution(jackpots, totalBet);
        var jackpotWin = awardSymbolJackpot(jackpots, outcome.JackpotCounts);
        result = {
            Stops:stops,
            Win:outcome.Win + (jackpotWin ? jackpotWin.Prize : 0),
            BaseWin:outcome.Win,
            JackpotWin:jackpotWin,
            ScatterCount:outcome.ScatterCount,
            FreeSpinCount:outcome.FreeSpinCount, RulesVersion:SLOT_RULES_VERSION,
            Jackpots:publicJackpots(jackpots)
        };

        receipts.Recent.push({ SpinId:spinId, Result:result });
        while (receipts.Recent.length > RECEIPT_LIMIT) receipts.Recent.shift();

        return vendActivityWrites(objects,playerId,"sugar",spinId).concat([
            { collection:COLLECTION, key:JACKPOT_KEY, userId:SYSTEM_USER_ID, value:jackpots,
                version:jackpotObject ? jackpotObject.version : "*", permissionRead:0, permissionWrite:0 },
            { collection:COLLECTION, key:receiptKey, userId:SYSTEM_USER_ID, value:receipts,
                version:receiptObject ? receiptObject.version : "*", permissionRead:0, permissionWrite:0 }
        ]);
    }, 10);

    if (!result) throw new Error("The progressive spin did not produce a result.");
    return serializeSpinReceipt(result, SLOT_RULES_VERSION);
}

// LUSTVEND uses the same trusted PlayFab bridge and RPC key as the slots.
// A conditional multi-object write commits wallet/ownership, receipt and ledger
// together. No historical receipts are trimmed and no client can write these rows.
var VEND_CATALOG = [
    {sku:"vid-chip",name:"Vid Chip",price:60,sprite:0},
    {sku:"holo-disk",name:"Holo Disk",price:90,sprite:1},
    {sku:"pinup-pack",name:"Pinup Pack",price:120,sprite:3},
    {sku:"avatar-item",name:"Avatar Item",price:180,sprite:4},
    {sku:"theme-pack",name:"Theme Pack",price:300,sprite:5},
    {sku:"special-item",name:"Special Item",price:600,sprite:10}
];
var VEND_VERSION = "lustvend-1";
var VEND_TARGETS=[10,25,50];
function vendFind(objects,key){for(var i=0;i<objects.length;++i)if(objects[i].key===key)return objects[i];return null;}
function vendWeek(day){var d=new Date(day+"T00:00:00Z");d.setUTCDate(d.getUTCDate()-((d.getUTCDay()+6)%7));return d.toISOString().slice(0,10);}
function vendEarningState(state,day){
    if(!state.earning || state.earning.day!==day)state.earning={day:day,spins:0,missions:[],set:false};
    var week=vendWeek(day);
    if(!state.weekly || state.weekly.week!==week)state.weekly={week:week,days:[],paid:false};
    if(typeof state.tutorialStep!=="number")state.tutorialStep=0;
}
function vendWrite(key,value,row){return {collection:"lustvend",key:key,userId:SYSTEM_USER_ID,value:value,version:row?row.version:"*",permissionRead:0,permissionWrite:0};}
function vendActivityReads(player,game,id){return [
    {collection:"lustvend",key:"account_"+player,userId:SYSTEM_USER_ID},
    {collection:"lustvend",key:"played_"+player+"_"+game+"_"+id,userId:SYSTEM_USER_ID}
];}
function vendActivityWrites(objects,player,game,id){
    var marker="played_"+player+"_"+game+"_"+id;
    if(vendFind(objects,marker))return [];
    var row=vendFind(objects,"account_"+player);
    var state=row?parseJsonValue(row.value,"gem account"):{balance:0,owned:[],pending:[]};
    var day=new Date().toISOString().slice(0,10);vendEarningState(state,day);
    state.earning.spins=Math.min(50,state.earning.spins+1);
    var writes=[];
    function grant(amount,source){state.balance+=amount;writes.push(vendWrite("ledger_"+player+"_"+source,{player:player,delta:amount,reason:source,source:source,at:day},null));}
    for(var m=0;m<VEND_TARGETS.length;++m){
        if(state.earning.spins>=VEND_TARGETS[m] && state.earning.missions.indexOf(VEND_TARGETS[m])<0){
            state.earning.missions.push(VEND_TARGETS[m]);grant(5,"mission_"+day+"_"+VEND_TARGETS[m]);
        }
    }
    if(state.earning.missions.length===3 && !state.earning.set){
        state.earning.set=true;grant(5,"set_"+day);
        if(state.weekly.days.indexOf(day)<0)state.weekly.days.push(day);
        if(state.weekly.days.length>=5 && !state.weekly.paid){state.weekly.paid=true;grant(30,"weekly_"+state.weekly.week);}
    }
    return writes.concat([vendWrite("account_"+player,state,row),vendWrite(marker,{day:day,game:game,id:id},null)]);
}
function rpcLustVend(nk, request) {
    var player=String(request.PlayFabId || "");
    if(!/^[A-Za-z0-9_-]{1,64}$/.test(player)) throw new Error("Invalid player identifier.");
    var action=request.Action;
    if(["Get","Daily","Purchase","Collect","Tutorial"].indexOf(action)<0) throw new Error("Invalid vending action.");
    var id=String(action==="Collect"?request.Receipt || "":request.RequestId || "");
    if((action==="Purchase" || action==="Collect") && !/^[A-Za-z0-9-]{16,80}$/.test(id))
        return JSON.stringify({error:"invalid_request_id"});
    var day=new Date().toISOString().slice(0,10);
    var collection="lustvend";
    var stateKey="account_"+player;
    var receiptKey="receipt_"+player+"_"+id;
    var ledgerKey="ledger_"+player+"_"+(action==="Daily"?"daily_"+day:id);
    var reads=[{collection:collection,key:stateKey,userId:SYSTEM_USER_ID}];
    if(action==="Purchase" || action==="Collect") reads.push({collection:collection,key:receiptKey,userId:SYSTEM_USER_ID});
    if(action==="Purchase" || action==="Daily") reads.push({collection:collection,key:ledgerKey,userId:SYSTEM_USER_ID});
    if(action==="Tutorial")reads.push({collection:collection,key:"ledger_"+player+"_tutorial_v1",userId:SYSTEM_USER_ID});
    var result;
    nk.storageWriteRetry(reads,function(objects){
        var rows={};
        for(var i=0;i<objects.length;++i) rows[objects[i].key]=objects[i];
        var stateRow=rows[stateKey];
        var state=stateRow?parseJsonValue(stateRow.value,"vending account"):{balance:0,owned:[],pending:[]};
        if(!state || typeof state.balance!=="number" || state.balance<0 || !Array.isArray(state.owned) || !Array.isArray(state.pending)) throw new Error("Invalid vending account state.");
        vendEarningState(state,day);
        function write(key,value){return {collection:collection,key:key,userId:SYSTEM_USER_ID,value:value,
            version:rows[key]?rows[key].version:"*",permissionRead:0,permissionWrite:0};}
        function snapshot(){return {player:player,balance:state.balance,owned:state.owned,pending:state.pending,catalog:VEND_CATALOG,version:VEND_VERSION,
            earning:{day:day,spins:state.earning.spins,targets:VEND_TARGETS,missionGems:5,setComplete:state.earning.set,weeklyDays:state.weekly.days.length,weeklyTarget:5,weeklyPaid:state.weekly.paid,tutorialStep:state.tutorialStep}};}
        if(action==="Get"){result=snapshot();return [];}
        if(action==="Tutorial"){
            var step=request.Step;
            if(typeof step!=="number" || step!==Math.floor(step) || step<1 || step>3){result={error:"invalid_tutorial_step"};return [];}
            if(step<=state.tutorialStep){result=snapshot();return [];}
            if(step!==state.tutorialStep+1){result={error:"tutorial_step_out_of_order"};return [];}
            state.tutorialStep=step;
            var tutorialWrites=[];
            if(step===3 && !rows["ledger_"+player+"_tutorial_v1"]){state.balance+=30;tutorialWrites.push(write("ledger_"+player+"_tutorial_v1",{player:player,delta:30,reason:"tutorial_complete",source:"tutorial:v1",at:day}));}
            result=snapshot();return tutorialWrites.concat([write(stateKey,state)]);
        }
        if(action==="Daily"){
            if(rows[ledgerKey]){result=snapshot();return [];}
            state.balance+=10;result=snapshot();
            return [write(stateKey,state),write(ledgerKey,{player:player,delta:10,reason:"daily_login",source:"daily:"+day,at:day})];
        }
        var existing=rows[receiptKey]?parseJsonValue(rows[receiptKey].value,"vending receipt"):null;
        if(action==="Collect"){
            if(!existing){result={error:"receipt_not_found"};return [];}
            if(existing.collected){result={receipt:id,collected:true,sku:existing.sku};return [];}
            existing.collected=true;
            state.pending=state.pending.filter(function(r){return r.receipt!==id;});
            result={receipt:id,collected:true,sku:existing.sku};
            return [write(stateKey,state),write(receiptKey,existing)];
        }
        if(existing){
            result=existing.sku===request.Sku && existing.version===request.Version?existing:{error:"idempotency_conflict"};
            return [];
        }
        if(request.Version!==VEND_VERSION){result={error:"catalog_changed"};return [];}
        var product=null;
        for(var c=0;c<VEND_CATALOG.length;++c) if(VEND_CATALOG[c].sku===request.Sku) product=VEND_CATALOG[c];
        if(!product){result={error:"unknown_product"};return [];}
        if(state.owned.indexOf(product.sku)>=0){result={error:"already_owned"};return [];}
        if(state.balance<product.price){result={error:"insufficient_gems"};return [];}
        // Receipt equals the per-account request UUID, so collect can directly
        // address the same durable receipt row without a second lookup index.
        state.balance-=product.price;state.owned.push(product.sku);
        state.pending.push({receipt:id,sku:product.sku});
        result={request:id,receipt:id,sku:product.sku,version:VEND_VERSION,price:product.price,balance:state.balance,collected:false};
        return [write(stateKey,state),write(receiptKey,result),write(ledgerKey,{player:player,delta:-product.price,reason:"vending_purchase",source:id,at:new Date().toISOString()})];
    },10);
    return JSON.stringify(result);
}

function InitModule(ctx, logger, nk, initializer) {
    initializer.registerRpc("sugarlust_progressive", rpcGateway);
    logger.info("Sugar Lust progressive jackpot RPC registered. Transactional settlement v1 enabled; legacy spin RPC disabled.");
}
