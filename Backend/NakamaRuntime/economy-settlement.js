// Transaction coordinator. All helpers below run synchronously inside a single
// storageWriteRetry callback. No HTTP, PlayFab writes or nested database commits.
var ECONOMY_COLLECTION = "slot_economy";
var ECONOMY_ACTIONS = ["GetOrMigratePlayerProfile","Spin","NeonNightsSpin","MegaLustPotSpin","MegaLustPotState","ClaimMissionReward","PickBonusCard","ClaimDailyBonus"];
function economyClone(value) { return JSON.parse(JSON.stringify(value)); }
function economyRead(collection, key) { return {collection:collection,key:key,userId:SYSTEM_USER_ID}; }
function economyObjectKey(row) { return row.collection + "/" + row.key; }
function economyWrite(read, value, row) {
    return {collection:read.collection,key:read.key,userId:SYSTEM_USER_ID,value:value,
        version:row ? row.version : "*",permissionRead:0,permissionWrite:0};
}
function economyFingerprint(action, args) {
    var fields = {Spin:["SpinId","BetPerLine","ActiveLines"],NeonNightsSpin:["SpinId","Bet"],MegaLustPotSpin:["SpinId","Bet","ExpectedRevision"],MegaLustPotState:[],
        ClaimMissionReward:["ClaimId","MissionId"],PickBonusCard:["Card"],
        ClaimDailyBonus:[],GetOrMigratePlayerProfile:[]}[action];
    var value = {Action:action};
    for (var i=0;i<fields.length;++i) {
        var field=fields[i];
        if (args[field] !== undefined) {
            if (typeof args[field] !== "number" && typeof args[field] !== "string") throw new Error("Invalid economy argument.");
            value[field]=args[field];
        }
    }
    return JSON.stringify(value);
}
function rpcGateway(ctx, logger, nk, payload) {
    if (ctx.userId) throw new Error("This RPC is reserved for server-to-server calls.");
    var request=JSON.parse(payload || "{}");
    if (request.Operation === "StageMigration") return rpcStageEconomyMigration(ctx,nk,request);
    if (request.Operation === "Economy") return rpcEconomy(ctx,logger,nk,request);
    // Old CloudScript cannot mutate separate reel/pool transactions during cutover.
    if (request.Operation !== "Get" && request.Operation !== "LustVend")
        throw new Error("Legacy spin RPC disabled. Deploy the transactional PlayFab bridge.");
    return rpcProgressive(ctx,logger,nk,payload);
}
function rpcEconomy(ctx, logger, nk, request) {
    if (ctx.userId) throw new Error("This RPC is reserved for server-to-server calls.");
    if (request.SettlementVersion !== 1) throw new Error("Unsupported settlement contract.");
    var player=String(request.PlayFabId || ""), action=request.Action, args=request.Args || {};
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(player)) throw new Error("Invalid player identifier.");
    if (ECONOMY_ACTIONS.indexOf(action)<0) throw new Error("Unknown economy action.");
    var fingerprint=economyFingerprint(action,args);
    var mega=action === "MegaLustPotSpin";
    var spin=action === "Spin" || action === "NeonNightsSpin" || mega;
    var id=spin ? args.SpinId : action === "ClaimMissionReward" ? args.ClaimId : "";
    if ((spin || action === "ClaimMissionReward") && (typeof id !== "string" || !/^[A-Za-z0-9_-]{1,64}$/.test(id)))
        throw new Error("Invalid settlement identifier.");
    var accountRead=economyRead(ECONOMY_COLLECTION,"account_"+player);
    var receiptRead=id ? economyRead(ECONOMY_COLLECTION,"receipt_"+nk.sha256Hash(player+"|"+action+"|"+id)) : null;
    var importRead=economyRead(ECONOMY_COLLECTION,"legacy_"+player);
    var reads=[accountRead,importRead,economyRead(COLLECTION,JACKPOT_KEY),
        economyRead(COLLECTION,RECEIPTS_KEY_PREFIX+player),economyRead(COLLECTION,NEON_RECEIPTS_KEY_PREFIX+player)];
    if (receiptRead) reads.push(receiptRead);
    if (spin && !mega) reads=reads.concat(vendActivityReads(player,action === "Spin" ? "sugar" : "neon",id));
    var result;
    nk.storageWriteRetry(reads,function(objects) {
        // A retry must start from fresh values, never a mutated failed attempt.
        var originals={}, staged={}, snapshot={};
        for(var i=0;i<objects.length;++i) {
            originals[economyObjectKey(objects[i])]=objects[i];
            snapshot[economyObjectKey(objects[i])]=economyClone(objects[i]);
        }
        function row(read) { return originals[economyObjectKey(read)]; }
        var accountRow=row(accountRead);
        var account=accountRow ? economyClone(parseJsonValue(accountRow.value,"economy account")) : null;
        if (!account) {
            if (request.MigrationReady !== true) throw new Error("Economy migration is in maintenance; imports are not enabled.");
            var importRow=row(importRead);
            if (!importRow && request.HasLegacyProfile !== false)
                throw new Error("Existing player requires a frozen legacy import. Run the private migration helper; do not reset progress.");
            var seed=importRow ? parseJsonValue(importRow.value,"frozen legacy import").Profile :
                {Coins:10050000,XP:0,DailyBonusLastClaimDate:"",DailyBonusStreakDay:0,SchemaVersion:1,BonusState:null};
            validateEconomySeed(seed);
            account={SchemaVersion:1,Revision:0,Profile:economyClone(seed),
                LegacySettled:{Spin:seed.LastSpin || null,NeonNightsSpin:seed.LastNeonSpin || null}};
        }
        if (account.SchemaVersion!==1 || !account.Profile || typeof account.Profile.Coins!=="number")
            throw new Error("Malformed transactional account; leaving it untouched.");
        var profile=account.Profile;
        // The existing reel helpers operate on this in-memory overlay. Their
        // pool, recent-receipt and gem writes join the final conditional batch.
        var overlay={
            storageRead:function(wanted) {
                var found=[];
                for(var j=0;j<wanted.length;++j) {
                    var key=economyObjectKey(wanted[j]);
                    var known=false;
                    for(var r=0;r<reads.length;++r) if(economyObjectKey(reads[r])===key) known=true;
                    if(!known) throw new Error("Undeclared transactional read.");
                    if(snapshot[key]) found.push(economyClone(snapshot[key]));
                }
                return found;
            },
            storageWriteRetry:function(wanted, update) {
                var writes=update(this.storageRead(wanted));
                for(var j=0;j<writes.length;++j) {
                    var key=economyObjectKey(writes[j]);
                    staged[key]=economyClone(writes[j]);
                    snapshot[key]=economyClone(writes[j]);
                }
            }
        };
        var domain=createEconomyDomain(player,{
            read:function() {return economyClone(profile);},
            write:function(value) {profile=economyClone(value);account.Profile=profile;}
        },function(reelRequest) {return JSON.parse(rpcProgressive({},logger,overlay,JSON.stringify(reelRequest)));});
        var jackpotRow=row(economyRead(COLLECTION,JACKPOT_KEY));
        var jackpots=jackpotRow ? publicJackpots(validateJackpots(parseJsonValue(jackpotRow.value,"jackpots"))) : publicJackpots(seedJackpots());
        var receiptRow=receiptRead ? row(receiptRead) : null;
        if(receiptRow) {
            var settled=parseJsonValue(receiptRow.value,"settled receipt");
            if(settled.Fingerprint!==fingerprint) throw new Error("Settlement identifier conflicts with original request.");
            result=economyClone(settled.Response);
            if(result.Profile) result.Profile=domain.__snapshot(jackpots);
            return []; // No second payment, feature consumption, XP or gem grant.
        }
        var legacy=spin && account.LegacySettled && account.LegacySettled[action];
        if(legacy && legacy.SpinId===id && legacy.Response) {
            // The last PlayFab response is proof of prior settlement. Import it,
            // but never debit or credit it again, regardless of old wager args.
            result=economyClone(legacy.Response);
            var expectedVersion=action === "Spin" ? 4 : 5;
            if(result.RulesVersion===3 || (expectedVersion===5 && result.RulesVersion===4)) {
                result.RulesVersion=expectedVersion;result.RecoveredLegacyReceipt=true;
            }
            result.Profile=domain.__snapshot(jackpots);
        } else {
            if(spin && !mega) {
                var recentRow=row(economyRead(COLLECTION,(action === "Spin" ? RECEIPTS_KEY_PREFIX : NEON_RECEIPTS_KEY_PREFIX)+player));
                var recent=recentRow ? parseJsonValue(recentRow.value,"legacy receipt").Recent : [];
                for(var old=0;old<recent.length;++old) if(recent[old].SpinId===id)
                    throw new Error("Legacy spin needs settlement reconciliation; no reroll or automatic payment allowed.");
                var activity=vendActivityReads(player,action === "Spin" ? "sugar" : "neon",id)[1];
                if(row(activity)) throw new Error("Legacy spin needs settlement reconciliation; no reroll or automatic payment allowed.");
            }
            result=domain[action](args,{});
        }
        if (!isFinite(profile.Coins) || profile.Coins<0 || Math.floor(profile.Coins)!==profile.Coins || profile.Coins>9007199254740991)
            throw new Error("Coin settlement exceeds safe integer range.");
        // Even read/migration and claim operations share the account CAS row.
        // This serializes every coin/feature mutation across games and devices.
        account.Profile=profile;account.Revision+=1;
        staged[economyObjectKey(accountRead)]=economyWrite(accountRead,account,accountRow);
        if(receiptRead) staged[economyObjectKey(receiptRead)]=economyWrite(receiptRead,{
            Fingerprint:fingerprint,Response:result,AccountRevision:account.Revision,
            Mode:mega ? result.Kind : spin ? (result.Bonus && !result.Bonus.Triggered && (result.Bonus.Active || result.Bonus.Complete) ? "NeonHeat" :
                result.Profile.FreeSpins[action === "Spin" ? "SugarLust" : "NeonNights"].LastWasFreeSpin ? "FreeSpin" : "PaidSpin") : action
        },null);
        var writes=[];
        for(var key in staged) if(Object.prototype.hasOwnProperty.call(staged,key)) writes.push(staged[key]);
        return writes;
    },10);
    return JSON.stringify({SettlementVersion:1,Result:result});
}

// Private HTTP-key RPC only. There is deliberately no player-facing CloudScript
// handler for this operation. Create-only staging prevents later legacy writes
// (including calls to historical CloudScript revisions) changing the import.
function validateEconomySeed(seed) {
    if (!seed || typeof seed.Coins !== "number" || !isFinite(seed.Coins) || seed.Coins<0 ||
        Math.floor(seed.Coins)!==seed.Coins || seed.Coins>9007199254740991 ||
        typeof seed.XP !== "number" || !isFinite(seed.XP) || seed.XP<0 ||
        Math.floor(seed.XP)!==seed.XP || seed.XP>9007199254740991)
        throw new Error("Invalid frozen legacy profile.");
}
function rpcStageEconomyMigration(ctx,nk,request) {
    if (ctx.userId) throw new Error("This RPC is reserved for server-to-server calls.");
    if (request.SettlementVersion!==1) throw new Error("Unsupported settlement contract.");
    var player=String(request.PlayFabId || "");
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(player)) throw new Error("Invalid player identifier.");
    validateEconomySeed(request.Profile);
    var accountRead=economyRead(ECONOMY_COLLECTION,"account_"+player);
    var importRead=economyRead(ECONOMY_COLLECTION,"legacy_"+player);
    var status;
    nk.storageWriteRetry([accountRead,importRead],function(objects) {
        var account=vendFind(objects,accountRead.key), existing=vendFind(objects,importRead.key);
        if(account){status="already_migrated";return [];}
        if(existing) {
            if(JSON.stringify(parseJsonValue(existing.value,"frozen import").Profile)!==JSON.stringify(request.Profile))
                throw new Error("Frozen legacy import differs. No overwrite permitted; reconcile offline.");
            status="already_staged";return [];
        }
        status="staged";
        return [economyWrite(importRead,{Profile:economyClone(request.Profile),StagedAt:new Date().toISOString()},null)];
    },10);
    return JSON.stringify({SettlementVersion:1,Status:status});
}
