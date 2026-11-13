// Pure ES5 rules. Time, configuration and settled outcomes come from the server.
var CasinoCosmeticRewards = (function () {
    var catalog = [
        {RewardId:"pepper_internal_mild_badge_v1",Name:"Pepper's Mild title",RewardKind:"Presentation",EquipSlot:"EventBadge",Ready:true,PresentationId:"pepper_mild_v1"},
        {RewardId:"pepper_internal_hot_v1",Name:"Hot outfit and jewelry",RewardKind:"Wearable",EquipSlot:"Outfit",Ready:false},
        {RewardId:"pepper_internal_extra_hot_v1",Name:"Extra Hot accents",RewardKind:"Wearable",EquipSlot:"Effect",Ready:false},
        {RewardId:"pepper_internal_inferno_v1",Name:"Inferno Pepper skin",RewardKind:"Wearable",EquipSlot:"Avatar",Ready:false}
    ];
    function find(id) { for(var i=0;i<catalog.length;++i) if(catalog[i].RewardId===id)return catalog[i]; return null; }
    function account(value) {
        var state=value ? JSON.parse(JSON.stringify(value)) : {balance:0,owned:[],pending:[]};
        if(typeof state.balance!=="number" || !isFinite(state.balance) || state.balance<0 || !Array.isArray(state.owned) || !Array.isArray(state.pending))
            throw new Error("Invalid cosmetic ownership account.");
        if(!state.cosmetics)state.cosmetics={};
        if(!state.equipped)state.equipped={};
        return state;
    }
    function grant(state,id,eventId,tierId) {
        var reward=find(id);
        if(!reward || !reward.Ready)throw new Error("This reward is a preview until its content is ready.");
        var already=state.owned.indexOf(id)>=0;
        if(!already)state.owned.push(id);
        state.cosmetics[id]={RewardId:id,RewardKind:reward.RewardKind,EquipSlot:reward.EquipSlot,EventId:eventId,TierId:tierId};
        return already;
    }
    function equip(state,id,slot) {
        var reward=find(id);
        if(!reward || !reward.Ready || reward.EquipSlot!==slot || state.owned.indexOf(id)<0 || !state.cosmetics[id])
            throw new Error("That reward is unowned or incompatible with this equipment slot.");
        state.equipped[slot]=id;
    }
    return {find:find,account:account,grant:grant,equip:equip};
}());

var HotStreakRules = (function () {
    function copy(value){return JSON.parse(JSON.stringify(value));}
    function integer(value,min){return typeof value==="number" && isFinite(value) && Math.floor(value)===value && value>=min && value<=9007199254740991;}
    function identifier(value){return typeof value==="string" && /^[A-Za-z0-9_-]{1,64}$/.test(value);}
    function timestamp(value){return typeof value==="string" && /Z$/.test(value) && isFinite(Date.parse(value));}
    function validateDefinition(definition) {
        if(!definition || !identifier(definition.EventId) || !integer(definition.RulesRevision,1) ||
            !timestamp(definition.StartsAt) || !timestamp(definition.EndsAt) || !timestamp(definition.ClaimUntil) ||
            !(Date.parse(definition.StartsAt)<Date.parse(definition.EndsAt) && Date.parse(definition.EndsAt)<Date.parse(definition.ClaimUntil)) ||
            typeof definition.EarningEnabled!=="boolean" || typeof definition.ClaimsEnabled!=="boolean" ||
            definition.TriggerWins!==3 || definition.WinPolicy!=="BaseWinPositive" || definition.HeatPolicy!=="OnePerWinFromTrigger" ||
            !integer(definition.ClientPresentationVersion,1) || definition.RepeatOwnershipPolicy!=="KeepExisting" || !Array.isArray(definition.EligibleGames) || !definition.EligibleGames.length ||
            definition.EligibleGames.length>2 || !Array.isArray(definition.Tiers) || definition.Tiers.length!==4)
            throw new Error("Invalid Hot Streak definition.");
        var games={},tiers={},rewards={},last=0;
        for(var g=0;g<definition.EligibleGames.length;++g){
            var game=definition.EligibleGames[g];
            if((game!=="SugarLust" && game!=="NeonNights") || games[game])throw new Error("Invalid Hot Streak eligible games.");
            games[game]=true;
        }
        var names=["Mild","Hot","ExtraHot","Inferno"];
        for(var i=0;i<definition.Tiers.length;++i){
            var tier=definition.Tiers[i], reward=CasinoCosmeticRewards.find(tier.RewardId);
            if(tier.TierId!==names[i] || tiers[tier.TierId] || !integer(tier.HeatRequired,1) || tier.HeatRequired<=last ||
                !reward || rewards[tier.RewardId] || typeof tier.ClaimEnabled!=="boolean" || (tier.ClaimEnabled && !reward.Ready))
                throw new Error("Invalid Hot Streak reward tier.");
            tiers[tier.TierId]=true;rewards[tier.RewardId]=true;last=tier.HeatRequired;
        }
        return definition;
    }
    function validateConfig(config) {
        if(!config || config.SchemaVersion!==1 || !identifier(config.ActiveEventId) || !Array.isArray(config.Events) || !config.Events.length || config.Events.length>24)
            throw new Error("Invalid Hot Streak configuration.");
        var ids={};
        for(var i=0;i<config.Events.length;++i){validateDefinition(config.Events[i]);if(ids[config.Events[i].EventId])throw new Error("Duplicate Hot Streak event.");ids[config.Events[i].EventId]=true;}
        if(!ids[config.ActiveEventId])throw new Error("Missing active Hot Streak definition.");
        return config;
    }
    function definition(config,id){if(!config)return null;for(var i=0;i<config.Events.length;++i)if(config.Events[i].EventId===id)return config.Events[i];return null;}
    function frozenDefinition(value){var result=copy(value);delete result.EarningEnabled;delete result.ClaimsEnabled;return JSON.stringify(result);}
    function phase(def,time) {
        if(!def)return "Disabled";
        if(time<Date.parse(def.StartsAt))return "Upcoming";
        if(time>=Date.parse(def.ClaimUntil))return "Archived";
        if(time>=Date.parse(def.EndsAt))return "ClaimOnly";
        return def.EarningEnabled ? "Active" : "Disabled";
    }
    function create(def){return {SchemaVersion:1,RulesRevision:def.RulesRevision,CurrentStreak:0,BestStreak:0,Heat:0,UnlockedTierIds:[],ClaimedTierIds:[],ProgressRevision:0,LastProcessedSpinId:""};}
    function validateProgress(state,def) {
        if(!state || state.SchemaVersion!==1 || state.RulesRevision!==def.RulesRevision || !integer(state.CurrentStreak,0) ||
            !integer(state.BestStreak,0) || state.BestStreak<state.CurrentStreak || !integer(state.Heat,0) || !integer(state.ProgressRevision,0) ||
            !Array.isArray(state.UnlockedTierIds) || !Array.isArray(state.ClaimedTierIds) || typeof state.LastProcessedSpinId!=="string")
            throw new Error("Invalid saved Hot Streak progress.");
        var seen={},claimed={};
        for(var i=0;i<state.UnlockedTierIds.length;++i){var tier=findTier(def,state.UnlockedTierIds[i]);if(!tier || seen[tier.TierId] || state.Heat<tier.HeatRequired)throw new Error("Invalid unlocked Hot Streak tier.");seen[tier.TierId]=true;}
        for(var c=0;c<state.ClaimedTierIds.length;++c){var id=state.ClaimedTierIds[c];if(!seen[id] || claimed[id])throw new Error("Invalid claimed Hot Streak tier.");claimed[id]=true;}
        return state;
    }
    function findTier(def,id){for(var i=0;i<def.Tiers.length;++i)if(def.Tiers[i].TierId===id)return def.Tiers[i];return null;}
    function evaluate(def,time,state,input) {
        validateDefinition(def);state=copy(validateProgress(state || create(def),def));
        var before=state.CurrentStreak;
        var delta={EventId:def.EventId,SpinId:input.SpinId,RulesRevision:def.RulesRevision,Eligible:false,StreakBefore:before,StreakAfter:before,
            HeatAdded:0,NewlyUnlockedTierIds:[],TriggerKind:"None",ProgressRevision:state.ProgressRevision};
        if(phase(def,time)!=="Active" || input.Mode!=="PaidBase" || def.EligibleGames.indexOf(input.Game)<0)return {State:state,Delta:delta};
        if(!integer(input.BaseWin,0) || !identifier(input.SpinId))throw new Error("Invalid settled Hot Streak outcome.");
        delta.Eligible=true;
        state.CurrentStreak=input.BaseWin>0 ? before+1 : 0;
        state.BestStreak=Math.max(state.BestStreak,state.CurrentStreak);
        delta.HeatAdded=state.CurrentStreak>=def.TriggerWins ? 1 : 0;
        state.Heat+=delta.HeatAdded;state.ProgressRevision+=1;state.LastProcessedSpinId=input.SpinId;
        if(!integer(state.Heat,0) || !integer(state.ProgressRevision,0) || !integer(state.CurrentStreak,0))throw new Error("Hot Streak progress exceeds safe range.");
        for(var i=0;i<def.Tiers.length;++i){var tier=def.Tiers[i];if(state.Heat>=tier.HeatRequired && state.UnlockedTierIds.indexOf(tier.TierId)<0){state.UnlockedTierIds.push(tier.TierId);delta.NewlyUnlockedTierIds.push(tier.TierId);}}
        delta.StreakAfter=state.CurrentStreak;delta.ProgressRevision=state.ProgressRevision;
        delta.TriggerKind=before===2 && state.CurrentStreak===3 ? "Entrance" : before>=3 && state.CurrentStreak===0 ? "StreakBreak" : delta.HeatAdded ? "Extension" : "None";
        return {State:state,Delta:delta};
    }
    function snapshot(def,time,state,revision,ownership) {
        var serverTime=new Date(time).toISOString();
        if(!def)return {Available:false,Phase:"Disabled",ServerTime:serverTime,AccountRevision:revision};
        state=validateProgress(state || create(def),def);
        var currentPhase=phase(def,time),canClaim=def.ClaimsEnabled && time>=Date.parse(def.StartsAt) && time<Date.parse(def.ClaimUntil);
        return {Available:true,EventId:def.EventId,RulesRevision:def.RulesRevision,Name:"Hot Streak with Pepper",Phase:currentPhase,
            ServerTime:serverTime,StartsAt:def.StartsAt,EndsAt:def.EndsAt,ClaimUntil:def.ClaimUntil,TriggerWins:def.TriggerWins,
            ClientPresentationVersion:def.ClientPresentationVersion,EligibleGames:copy(def.EligibleGames),Progress:copy(state),AccountRevision:revision,
            OwnedRewardIds:ownership ? ownership.owned.filter(function(id){return !!CasinoCosmeticRewards.find(id);}) : [],
            EquippedRewardId:ownership && ownership.equipped ? ownership.equipped.EventBadge || "" : "",
            Tiers:def.Tiers.map(function(tier){var reward=CasinoCosmeticRewards.find(tier.RewardId),unlocked=state.UnlockedTierIds.indexOf(tier.TierId)>=0,claimed=state.ClaimedTierIds.indexOf(tier.TierId)>=0;
                return {TierId:tier.TierId,HeatRequired:tier.HeatRequired,RewardId:tier.RewardId,RewardName:reward.Name,RewardKind:reward.RewardKind,EquipSlot:reward.EquipSlot,
                    RewardReady:reward.Ready,Unlocked:unlocked,Claimed:claimed,Claimable:canClaim && tier.ClaimEnabled && reward.Ready && unlocked && !claimed};})};
    }
    return {validateConfig:validateConfig,validateDefinition:validateDefinition,definition:definition,frozenDefinition:frozenDefinition,phase:phase,create:create,
        validateProgress:validateProgress,findTier:findTier,evaluate:evaluate,snapshot:snapshot,identifier:identifier};
}());
