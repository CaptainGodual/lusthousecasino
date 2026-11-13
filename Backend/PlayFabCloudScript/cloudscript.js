// PlayFab authenticates callers. Nakama atomically owns coins, features and settlement.
var PROFILE_KEY = "SugarLustProfile";
var NAKAMA_URL_KEY = "SugarLustNakamaGatewayUrl";
var NAKAMA_HTTP_KEY = "SugarLustNakamaHttpKey";
function getTitleInternalValue(data, key) {
    var value = data && data[key];
    if (value && typeof value === "object" && value.Value !== undefined) value = value.Value;
    return typeof value === "string" ? value : "";
}

function callNakama(request, vending) {
    var settings = server.GetTitleInternalData({ Keys:[NAKAMA_URL_KEY, NAKAMA_HTTP_KEY, "SugarLustEconomyMigrationReady"] });
    var baseUrl = getTitleInternalValue(settings && settings.Data, NAKAMA_URL_KEY).replace(/\/+$/, "");
    var httpKey = getTitleInternalValue(settings && settings.Data, NAKAMA_HTTP_KEY);
    if (!baseUrl || !/^https:\/\//i.test(baseUrl)) {
        throw new Error("Nakama HTTPS gateway is not configured. Set " + NAKAMA_URL_KEY + " to an HTTPS URL in PlayFab Title Internal Data.");
    }
    if (!httpKey) throw new Error("Nakama server-to-server key is not configured in PlayFab Title Internal Data.");

    if (request.Operation === "Economy") request.MigrationReady =
        getTitleInternalValue(settings && settings.Data, "SugarLustEconomyMigrationReady") === "transactional-v1";
    var url = baseUrl + "/v2/rpc/sugarlust_progressive?http_key=" + encodeURIComponent(httpKey) + "&unwrap";
    var responseText = http.request(url, "post", JSON.stringify(request), "application/json", {}, false);
    var response;
    try { response = typeof responseText === "string" ? JSON.parse(responseText) : responseText; }
    catch (error) { throw new Error("Nakama returned an unreadable progressive response."); }
    if (!response || response.message || (!vending && (response.error || !response.Jackpots))) {
        throw new Error("Nakama rejected the progressive request: " + JSON.stringify(response));
    }
    return response;
}

function callLustVend(action,args) {
    if(!currentPlayerId) throw new Error("Player login required.");
    args=args || {};
    // Identity, amount and price are never copied from client arguments.
    return callNakama({Operation:"LustVend",Action:action,PlayFabId:currentPlayerId,
        RequestId:args.requestId,Sku:args.sku,Version:args.version,Receipt:args.receipt},true);
}
handlers.LustVendState=function(args,context){return callLustVend("Daily",{});};
handlers.LustVendPurchase=function(args,context){return callLustVend("Purchase",args);};
handlers.LustVendCollect=function(args,context){return callLustVend("Collect",args);};
handlers.LustVendTutorial=function(args,context){
    if(!currentPlayerId)throw new Error("Player login required.");
    return callNakama({Operation:"LustVend",Action:"Tutorial",PlayFabId:currentPlayerId,Step:requireInteger(args && args.step,1,3,"tutorial step")},true);
};

function hasLegacyProfile() {
    var read=server.GetUserInternalData({PlayFabId:currentPlayerId,Keys:[PROFILE_KEY]});
    return !!(read && read.Data && read.Data[PROFILE_KEY] && read.Data[PROFILE_KEY].Value);
}

function requireInteger(value, min, max, label) {
    var number = Number(value);
    if (!isFinite(number) || Math.floor(number) !== number || number < min || number > max) {
        throw new Error("Invalid " + label + ".");
    }
    return number;
}


function callEconomy(action, args) {
    if (!currentPlayerId) throw new Error("Player login required.");
    args = args || {};
    var allowed = {};
    var fields = {
        Spin:["SpinId","BetPerLine","ActiveLines"], NeonNightsSpin:["SpinId","Bet"],MegaLustPotSpin:["SpinId","Bet","ExpectedRevision"],MegaLustPotState:[],
        ClaimMissionReward:["ClaimId","MissionId"], PickBonusCard:["Card"],
        ClaimDailyBonus:[], GetOrMigratePlayerProfile:[], GetHotStreakState:[],
        ClaimHotStreakReward:["ClaimId","EventId","TierId"], EquipCasinoCosmetic:["EquipId","RewardId","Slot"]
    }[action];
    for (var i = 0; i < fields.length; ++i) if (args[fields[i]] !== undefined) allowed[fields[i]] = args[fields[i]];
    // Only the existence flag crosses this bridge. Legacy balances must have
    // been frozen by the private migration tool, never supplied by a player call.
    var response = callNakama({Operation:"Economy", SettlementVersion:1,
        PlayFabId:currentPlayerId, Action:action, Args:allowed, HasLegacyProfile:hasLegacyProfile()}, true);
    if (!response || response.SettlementVersion !== 1 || !response.Result)
        throw new Error("Transactional economy backend is not deployed. No legacy settlement fallback is allowed.");
    return response.Result;
}
["GetOrMigratePlayerProfile","Spin","NeonNightsSpin","MegaLustPotSpin","MegaLustPotState","ClaimMissionReward","PickBonusCard","ClaimDailyBonus","GetHotStreakState","ClaimHotStreakReward","EquipCasinoCosmetic"].forEach(function(action) {
    handlers[action] = function(args, context) { return callEconomy(action,args); };
});
handlers.GetProgressiveJackpots = function(args, context) { return callNakama({Operation:"Get"}); };
