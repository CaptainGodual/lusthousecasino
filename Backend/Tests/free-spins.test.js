const fs = require('fs');
const path = require('path');
const vm = require('vm');
const assert = require('assert/strict');
const root = process.argv[2] || path.resolve(__dirname, '../..');
const clone = value => JSON.parse(JSON.stringify(value));
const nkContext = { Math: Object.assign(Object.create(Math), {random: () => 0.5}) };
vm.createContext(nkContext);
vm.runInContext(fs.readFileSync(path.join(root, 'Backend/NakamaRuntime/index.js'), 'utf8'), nkContext);
assert.equal(nkContext.REEL_STRIPS.length, 5);
for (const strip of nkContext.REEL_STRIPS) {
    assert.equal(strip.length, 30, 'Sugar Lust keeps 30 stops');
    assert.equal(strip.filter(s => s === 17).length, 2, 'Two dedicated Free Spins symbols per reel');
}

// A server stop must produce exactly the same symbols in the Unreal client.
const modelHeader = fs.readFileSync(path.join(root,'Source/SugarLustUE/SugarLustSlotMachine.h'),'utf8');
const modelSource = fs.readFileSync(path.join(root,'Source/SugarLustUE/SugarLustSlotMachine.cpp'),'utf8');
const enumNames = modelHeader.match(/enum class ESugarLustSymbol[^]*?\{([^]*?)\};/)[1].split(',').map(s=>s.trim());
const modelBlock = modelSource.split('    ReelStrips =')[1].split('    // Double each')[0];
const halves = [...modelBlock.matchAll(/\{([^{}]+)\}/g)].map(match=>[...match[1].matchAll(/ESugarLustSymbol::(\w+)/g)].map(m=>enumNames.indexOf(m[1])));
assert.equal(halves.length,5);
const freePositions = modelSource.match(/FreePositions\[5\] = \{([^}]+)\}/)[1].split(',').map(Number);
for(let reel=0;reel<5;++reel) {
    const first=halves[reel];
    const strip=first.concat(first.map((_,i)=>first[(i+7)%15]));
    strip[freePositions[reel]]=17;
    strip[15+(freePositions[reel]+8)%15]=17;
    let foundScatter=false;
    for(let i=0;i<strip.length;++i) {
        if(strip[i]!==8) continue;
        if(foundScatter) strip[i]=5;
        else foundScatter=true;
    }
    if(!foundScatter) strip[13]=8;
    assert.equal(strip.filter(s=>s===8).length,1,'One Scatter per Sugar Lust reel');
    const positionRows=modelSource.match(/JackpotPositions\[5\]\[4\] = \{([^;]+)\};/)[1];
    const positions=[...positionRows.matchAll(/\{([^}]+)\}/g)].map(m=>m[1].split(',').map(Number));
    for(let tier=0;tier<4;++tier) strip[positions[reel][tier]]=18+tier;
    assert.deepEqual(strip,clone(nkContext.REEL_STRIPS[reel]),'Client/server Sugar Lust symbols must match at every stop');
}

for (const strip of nkContext.NEON_REEL_STRIPS) assert(strip.includes(13));
assert.equal(nkContext.calculateNeonWin(Array(20).fill(13), 100).Win, 0, 'Free Spins symbol is not a line-pay symbol');
assert.equal(nkContext.calculateNeonWin(Array(20).fill(13), 100).FreeSpinCount, 20);
const fsStops = nkContext.REEL_STRIPS.map(s => s.indexOf(17));
assert.equal(nkContext.calculateWin(fsStops, 10, 20).FreeSpinCount, 5);


// Calculate exact trigger probability from independent, uniformly sampled stops.
function scatterRate(strips, rows, symbol, weights) {
    let probability=[1];
    for(let reel=0;reel<strips.length;++reel) {
        const strip=strips[reel];
        const counts=Array(rows+1).fill(0);
        for(let stop=0;stop<strip.length;++stop) {
            let count=0;
            for(let row=0;row<rows;++row) if(strip[(stop+row)%strip.length]===symbol) ++count;
            counts[count]+=weights ? weights[reel][stop] : 1/strip.length;
        }
        const next=Array(probability.length+counts.length-1).fill(0);
        for(let i=0;i<probability.length;++i)
            for(let j=0;j<counts.length;++j) next[i+j]+=probability[i]*counts[j];
        probability=next;
    }
    return probability.slice(3).reduce((a,b)=>a+b,0);
}
for(const strip of nkContext.NEON_REEL_STRIPS) assert.equal(strip.filter(s=>s===12).length,1,'No adjacent Neon Scatter pair');
const sugarRate=scatterRate(nkContext.REEL_STRIPS,3,8,nkContext.SUGAR_STOP_WEIGHTS);
const neonRate=scatterRate(nkContext.NEON_REEL_STRIPS,4,12,nkContext.NEON_STOP_WEIGHTS);
assert(sugarRate>1/125 && sugarRate<1/110);
assert(neonRate>1/165 && neonRate<1/145);
console.log('Scatter trigger odds: Sugar Lust 1 in '+(1/sugarRate).toFixed(1)+', Neon Nights 1 in '+(1/neonRate).toFixed(1));

function storageNakama() {
    const rows = new Map();
    const key = r => r.collection + '/' + r.key;
    return {
        storageRead: reads => reads.map(r => rows.get(key(r))).filter(Boolean).map(clone),
        storageWriteRetry(reads, fn) {
            const writes = fn(this.storageRead(reads));
            for (const row of writes) rows.set(key(row), clone(row));
        }
    };
}
for (const operation of ['Spin', 'NeonSpin']) {
    const nk = storageNakama();
    const request = {Operation:operation, PlayFabId:'test', SpinId:'free', FreeSpin:true, Bet:100, BetPerLine:10, ActiveLines:20};
    const a = JSON.parse(nkContext.rpcProgressive({}, {}, nk, JSON.stringify(request)));
    assert.equal(a.RulesVersion, operation === 'NeonSpin' ? 5 : 4);
    assert.deepEqual(a.Jackpots, clone(nkContext.SEEDS), 'Free spins do not contribute a fictitious paid wager');
    const duplicate = JSON.parse(nkContext.rpcProgressive({}, {}, nk, JSON.stringify(request)));
    assert.deepEqual(a, duplicate, 'Nakama replays the same spin receipt');
    request.FreeSpin = false; request.SpinId = 'paid';
    const paid = JSON.parse(nkContext.rpcProgressive({}, {}, nk, JSON.stringify(request)));
    assert(paid.Jackpots.Mini > a.Jackpots.Mini || operation === 'Spin');
}


// Validate both the symbol trigger and its server-side jackpot ledger effect.
const tierNames=['Mini','Minor','Major','Grand'];
for(const [strips,weights,rows,firstId,scatter,free] of [
    [nkContext.REEL_STRIPS,nkContext.SUGAR_STOP_WEIGHTS,3,18,8,17],
    [nkContext.NEON_REEL_STRIPS,nkContext.NEON_STOP_WEIGHTS,4,14,12,13]
]) {
    for(let reel=0;reel<5;++reel) {
        assert(Math.abs(weights[reel].reduce((a,b)=>a+b,0)-1)<1e-12);
        for(let stop=0;stop<strips[reel].length;++stop) {
            const window=Array.from({length:rows},(_,r)=>strips[reel][(stop+r)%strips[reel].length]);
            if(window.includes(scatter)||window.includes(free))
                assert(Math.abs(weights[reel][stop]-1/strips[reel].length)<1e-12,'Existing feature-window probability is preserved');
        }
    }
    for(let tier=0;tier<4;++tier) {
        const rate=scatterRate(strips,rows,firstId+tier,weights);
        assert(Math.abs(rate-nkContext.JACKPOT_TARGET_RATES[tierNames[tier]])<1e-12,'Matching-symbol rate equals the configured tier rarity');
    }
}
const state=clone(nkContext.seedJackpots());
assert.equal(nkContext.awardSymbolJackpot(state,[1,1,1,1]),null,'Mixed jackpot symbols do not pay');
assert.equal(nkContext.awardSymbolJackpot(state,[2,0,0,0]),null,'Two matching symbols do not pay');
for(let tier=0;tier<4;++tier) {
    const pools=clone(nkContext.seedJackpots()); pools[tierNames[tier]]+=1234;
    const counts=[0,0,0,0];counts[tier]=3;
    const prize=nkContext.awardSymbolJackpot(pools,counts);
    assert.equal(prize.Tier,tierNames[tier]);
    assert.equal(prize.Prize,nkContext.SEEDS[tierNames[tier]]+1234);
    assert.equal(pools[tierNames[tier]],nkContext.SEEDS[tierNames[tier]],'Only the awarded pool resets');
}
for(const [operation,strips,weights,rows,firstId] of [
    ['Spin',nkContext.REEL_STRIPS,nkContext.SUGAR_STOP_WEIGHTS,3,18],
    ['NeonSpin',nkContext.NEON_REEL_STRIPS,nkContext.NEON_STOP_WEIGHTS,4,14]
]) {
    for(let tier=0;tier<4;++tier) {
        const randomValues=[];
        for(let reel=0;reel<5;++reel) {
            const stop=weights[reel].findIndex((_,s)=>Array.from({length:rows},(_,r)=>strips[reel][(s+r)%strips[reel].length]).includes(firstId+tier));
            randomValues.push(weights[reel].slice(0,stop).reduce((a,b)=>a+b,0)+weights[reel][stop]/2);
        }
        nkContext.Math.random=()=>randomValues.length?randomValues.shift():0.5;
        const nk=storageNakama();
        const request={Operation:operation,PlayFabId:'jackpot-test',SpinId:'matching-'+tier,Bet:100,BetPerLine:10,ActiveLines:20};
        const win=JSON.parse(nkContext.rpcProgressive({}, {}, nk,JSON.stringify(request)));
        assert.equal(win.JackpotWin.Tier,tierNames[tier]);
        assert.equal(win.JackpotWin.Count,5);
        assert.equal(win.Win,win.BaseWin+win.JackpotWin.Prize);
        const after=JSON.stringify(nk.storageRead([{collection:nkContext.COLLECTION,key:nkContext.JACKPOT_KEY}]));
        assert.deepEqual(JSON.parse(nkContext.rpcProgressive({}, {}, nk,JSON.stringify(request))),win,'Retry replays the same jackpot award');
        assert.equal(JSON.stringify(nk.storageRead([{collection:nkContext.COLLECTION,key:nkContext.JACKPOT_KEY}])),after,'Retry does not reset or contribute to the pools twice');
    }
}
nkContext.Math.random=()=>0.5;
console.log('PASS: matching jackpot symbols, all four pools, atomic award/retry, weighted rarity, and unchanged Scatter/Free Spins probabilities.');

let saved = null;
let nextOutcome = null;
let lastRequest = null;
let requests = 0;
const context = {
    handlers:{}, currentPlayerId:'test-player', Math:Object.assign(Object.create(Math), {random:()=>0.5}),
    server:{
        GetUserInternalData:()=>({Data:saved ? {SugarLustProfile:{Value:JSON.stringify(saved)}} : {}}),
        UpdateUserInternalData:r=>{saved=JSON.parse(r.Data.SugarLustProfile);}
    }
};
vm.createContext(context);
vm.runInContext(fs.readFileSync(path.join(root,'Backend/NakamaRuntime/mega-pot-rules.js'),'utf8'),context);
vm.runInContext(fs.readFileSync(path.join(root,'Backend/NakamaRuntime/economy-domain.js'),'utf8'),context);
Object.assign(context.handlers,context.createEconomyDomain(context.currentPlayerId,{read:()=>saved?clone(saved):null,write:p=>saved=clone(p)},request=>context.callNakama(request)));
context.callNakama = request => {
    if (request.Operation === 'Get') return {Jackpots:clone(nkContext.SEEDS)};
    ++requests; lastRequest=clone(request);
    assert(nextOutcome, 'Test must supply a spin result');
    const result=clone(nextOutcome);
    if(result.RulesVersion==='current') result.RulesVersion=request.Operation==='NeonSpin'?5:4;
    return result;
};
function outcome(symbols=0, win=0, scatters=0) {
    nextOutcome={Stops:[0,0,0,0,0],Grid:Array(20).fill(0),Win:win,BaseWin:win,ScatterCount:scatters,
        FreeSpinCount:symbols,RulesVersion:'current',Jackpots:clone(nkContext.SEEDS),JackpotWin:null};
}
for (const game of ['SugarLust','NeonNights']) {
    saved=null;
    const handler=game==='SugarLust'?'Spin':'NeonNightsSpin';
    const wager=game==='SugarLust'?200:500;
    const args=id=>({SpinId:id,BetPerLine:10,ActiveLines:20,Bet:100});
    outcome(3);
    const trigger=context.handlers[handler](args('trigger'));
    assert.equal(trigger.Profile.FreeSpins[game].Remaining,10);
    assert.equal(trigger.Profile.Coins,10050000-wager);
    const count=requests;
    assert.deepEqual(clone(context.handlers[handler](args('trigger'))), clone(trigger));
    assert.equal(requests,count,'Duplicate does not consume a spin or charge again');
    saved.Coins=0;
    outcome(0,750);
    const changed=args('free-1'); changed.BetPerLine=9999;changed.ActiveLines=1;changed.Bet=9999;
    const free=context.handlers[handler](changed);
    assert.equal(free.Profile.Coins,750,'Free spin remains playable at zero coins');
    assert.equal(free.Profile.FreeSpins[game].Remaining,9);
    assert.equal(free.Profile.FreeSpins[game].TotalWin,750);
    assert.equal(lastRequest.FreeSpin,true);
    assert.equal(game==='SugarLust'?lastRequest.BetPerLine:lastRequest.Bet,game==='SugarLust'?10:100,'Server ignores client wager changes');
    const other=game==='SugarLust'?'NeonNights':'SugarLust';
    assert.equal(free.Profile.FreeSpins[other].Remaining,0,'Games have isolated feature counts');
    outcome(4);
    const retrigger=context.handlers[handler](args('retrigger'));
    assert.equal(retrigger.Profile.FreeSpins[game].Remaining,13,'Consume one then add five, not a fresh round');
    saved.FreeSpins[game].TotalAwarded=48;
    saved.FreeSpins[game].Remaining=2;
    outcome(5);
    const capped=context.handlers[handler](args('cap'));
    assert.equal(capped.Profile.FreeSpins[game].Remaining,3);
    assert.equal(capped.Profile.FreeSpins[game].TotalAwarded,50);
    assert.equal(capped.Profile.FreeSpins[game].LastAward,2);
    outcome(5);
    const noMore=context.handlers[handler](args('at-limit'));
    assert.equal(noMore.Profile.FreeSpins[game].Remaining,2);
    assert.equal(noMore.Profile.FreeSpins[game].LastAward,0,'Feature cannot award unlimited extra spins');
    saved.FreeSpins[game].Remaining=1;
    outcome(0,125);
    const final=context.handlers[handler](args('final'));
    assert.equal(final.Profile.FreeSpins[game].Remaining,0);
    assert.equal(final.Profile.FreeSpins[game].TotalWin,875);
    assert.equal(final.Profile.Coins,875);
    const resumed=context.handlers.GetOrMigratePlayerProfile({});
    assert.equal(resumed.FreeSpins[game].TotalWin,875,'Reconnect restores completed feature result');
    saved.Coins=0; outcome();
    assert.throws(()=>context.handlers[handler](args('paid-again')),/Not enough coins/,'A finished feature cannot grant free paid spins');
}

// A Sugar Lust Scatter bonus pauses free spins, and its prize belongs to that feature.
saved=null;outcome(3);context.handlers.Spin({SpinId:'sugar-trigger',BetPerLine:10,ActiveLines:20});
outcome(0,0,3);context.handlers.Spin({SpinId:'sugar-scatter',BetPerLine:10,ActiveLines:20});
const remaining=saved.FreeSpins.SugarLust.Remaining;
assert.throws(()=>context.handlers.Spin({SpinId:'blocked',BetPerLine:10,ActiveLines:20}),/pending bonus/);
for(let card=0;card<3;++card) context.handlers.PickBonusCard({Card:card});
assert.equal(saved.FreeSpins.SugarLust.Remaining,remaining);
assert(saved.FreeSpins.SugarLust.TotalWin>0);

// A real Scatter trigger during free spins pauses the remaining count.
saved=null;outcome(3);context.handlers.NeonNightsSpin({SpinId:'neon-trigger',Bet:100});
outcome(0,0,3);nextOutcome.Grid[0]=nextOutcome.Grid[6]=nextOutcome.Grid[12]=12;
const heatTrigger=context.handlers.NeonNightsSpin({SpinId:'heat-trigger',Bet:100});
assert.equal(heatTrigger.Profile.FreeSpins.NeonNights.Remaining,9,'Only the triggering free spin is consumed');
assert.equal(saved.NeonHeatBonus.FreeSpinFeature,true);
const heatPrize=saved.NeonHeatBonus.Total;
for (let respin=0;respin<3;++respin) {
    context.handlers.NeonNightsSpin({SpinId:'heat-'+respin,Bet:100});
    assert.equal(saved.FreeSpins.NeonNights.Remaining,9,'Every Neon Heat respin preserves queued free spins');
}
assert.equal(saved.NeonHeatBonus,null);
assert.equal(saved.FreeSpins.NeonNights.TotalWin,heatPrize);
outcome();context.handlers.NeonNightsSpin({SpinId:'resumed-free-spin',Bet:100});
assert.equal(saved.FreeSpins.NeonNights.Remaining,8,'Free spins resume after Neon Heat finishes');
console.log('PASS: reel symbols, payout exclusion, free-spin wagers, locked bets, retries, retriggers, reconnects, feature isolation, Candy Jar pause and Neon Heat pause.');

// A historical Nakama receipt must not be silently reinterpreted or consume progress.
for (const [handler, game] of [['Spin','SugarLust'], ['NeonNightsSpin','NeonNights']]) {
    saved=null; outcome(3);
    const args={SpinId:'contract-trigger',BetPerLine:10,ActiveLines:20,Bet:100};
    context.handlers[handler](args);
    const before=clone(saved);
    outcome(0,100); nextOutcome.RulesVersion=3;
    args.SpinId='historical-receipt';
    assert.throws(()=>context.handlers[handler](args), /RulesVersion=3 expected=[45]/);
    assert.deepEqual(saved,before,'Rejected historical receipt cannot change coins, XP or free spins');
    outcome(); delete nextOutcome.FreeSpinCount;
    assert.throws(()=>context.handlers[handler](args), /FreeSpinCount=undefined/);
    assert.deepEqual(saved,before,'Missing feature fields cannot consume a free spin');
}
console.log('PASS: incompatible receipts report exact contract failures and preserve balances/free spins.');

// A current runtime's attested historical receipt settles its ORIGINAL win once.
for (const handler of ['Spin','NeonNightsSpin']) {
    saved=null; outcome(3);
    const args={SpinId:'recovery-trigger',BetPerLine:10,ActiveLines:20,Bet:100};
    context.handlers[handler](args);
    const coins=saved.Coins;
    outcome(0,1234); nextOutcome.RulesVersion=3;
    nextOutcome.ReceiptRuntimeRulesVersion=handler==='NeonNightsSpin'?5:4; nextOutcome.RecoveredLegacyReceipt=true;
    args.SpinId='recovery';
    const recovered=context.handlers[handler](args);
    assert.equal(recovered.RecoveredLegacyReceipt,true);
    assert.equal(recovered.Win,1234);
    assert.equal(saved.Coins,coins+1234);
    const game=handler==='Spin'?'SugarLust':'NeonNights';
    assert.equal(saved.FreeSpins[game].Remaining,9);
    assert.deepEqual(clone(context.handlers[handler](args)),clone(recovered));
    assert.equal(saved.Coins,coins+1234,'Retry cannot pay the recovered win twice');
    assert.equal(saved.FreeSpins[game].Remaining,9,'Retry cannot consume another free spin');
}
for (const operation of ['Spin','NeonSpin']) {
    const nk=storageNakama();
    const request={Operation:operation,PlayFabId:'legacy',SpinId:'legacy',FreeSpin:true,Bet:100,BetPerLine:10,ActiveLines:20};
    const prefix=operation==='Spin'?nkContext.RECEIPTS_KEY_PREFIX:nkContext.NEON_RECEIPTS_KEY_PREFIX;
    const receipt={RulesVersion:3,Win:1234,BaseWin:1234,FreeSpinCount:0,ScatterCount:0,Stops:[0,0,0,0,0],Grid:Array(20).fill(0),Jackpots:clone(nkContext.SEEDS)};
    const read={collection:nkContext.COLLECTION,key:prefix+'legacy',userId:nkContext.SYSTEM_USER_ID};
    nk.storageWriteRetry([],()=>[Object.assign({},read,{value:{Recent:[{SpinId:'legacy',Result:receipt}]}})]);
    const before=clone(nk.storageRead([read]));
    const replay=JSON.parse(nkContext.rpcProgressive({}, {}, nk, JSON.stringify(request)));
    assert.equal(replay.ReceiptRuntimeRulesVersion,operation==='NeonSpin'?5:4);
    assert.equal(replay.RecoveredLegacyReceipt,true);
    assert.equal(replay.Win,1234);
    assert.deepEqual(nk.storageRead([read]),before,'Historical receipt is never rewritten');
}
console.log('PASS: version-3 receipt recovery preserves original outcomes and settles each payout/free spin exactly once.');

for (const [handler,key] of [['Spin','LastSpin'],['NeonNightsSpin','LastNeonSpin']]) {
    saved=null; outcome();
    const args={SpinId:'settled-v3',BetPerLine:10,ActiveLines:20,Bet:100};
    context.handlers[handler](args);
    saved[key].Response.RulesVersion=3;
    const before=clone(saved), calls=requests;
    const replay=context.handlers[handler](args);
    assert.equal(replay.RulesVersion,handler==='NeonNightsSpin'?5:4);
    assert.equal(replay.RecoveredLegacyReceipt,true);
    assert.equal(requests,calls,'Settled PlayFab receipt needs no Nakama call');
    assert.deepEqual(saved,before,'Settled PlayFab receipt cannot change progress or balance');
}
console.log('PASS: already-settled PlayFab version-3 responses recover without another debit, credit or RPC.');

// Neon Muse: one ordinary symbol per strip, highest regular multiplier, no feature effects.
for (const strip of nkContext.NEON_REEL_STRIPS) {
    assert.equal(strip.filter(s=>s===18).length,1);
    assert.equal(strip.length,nkContext.NEON_BASE_STRIP.length);
}
for (let length=3; length<=5; ++length) {
    const grid=Array(20).fill(12);
    // Avoid scatter payout while retaining a nonpaying filler.
    grid.fill(14);
    for(let reel=0;reel<length;++reel) grid[reel]=18;
    const win=nkContext.calculateNeonWin(grid,100);
    assert.equal(win.Win,100*[10,30,100][length-3]);
    assert.equal(win.ScatterCount,0); assert.equal(win.FreeSpinCount,0);
    grid[1]=11;
    assert.equal(nkContext.calculateNeonWin(grid,100).Win,win.Win,'Wild substitutes for Muse');
}
const noMuseWin=Array(20).fill(14); noMuseWin[0]=noMuseWin[1]=18;
assert.equal(nkContext.calculateNeonWin(noMuseWin,100).Win,0,'Two Muse reels do not pay');
console.log('PASS: Neon Muse reel frequency, 3/4/5-reel payouts, Wild substitution and losing combinations.');

// Rules-4 Neon receipts remain payable after the Muse/rules-5 rollout.
{
    const nk=storageNakama();
    const read={collection:nkContext.COLLECTION,key:nkContext.NEON_RECEIPTS_KEY_PREFIX+'v4',userId:nkContext.SYSTEM_USER_ID};
    const old={RulesVersion:4,Win:4321,BaseWin:4321,Grid:Array(20).fill(0),FreeSpinCount:0,ScatterCount:0,Jackpots:clone(nkContext.SEEDS)};
    nk.storageWriteRetry([],()=>[Object.assign({},read,{value:{Recent:[{SpinId:'v4',Result:old}]}})]);
    const replay=JSON.parse(nkContext.rpcProgressive({}, {}, nk, JSON.stringify({Operation:'NeonSpin',PlayFabId:'v4',SpinId:'v4',Bet:100,FreeSpin:true})));
    assert.equal(replay.RulesVersion,4);
    assert.equal(replay.ReceiptRuntimeRulesVersion,5);
    assert.equal(replay.Win,4321);
    saved=null;outcome(3);context.handlers.NeonNightsSpin({SpinId:'v4-trigger',Bet:100});
    const coins=saved.Coins;
    nextOutcome=replay;
    const recovered=context.handlers.NeonNightsSpin({SpinId:'v4',Bet:100});
    assert.equal(recovered.RulesVersion,5);
    assert.equal(saved.Coins,coins+4321);
    assert.equal(saved.FreeSpins.NeonNights.Remaining,9);
    context.handlers.NeonNightsSpin({SpinId:'v4',Bet:100});
    assert.equal(saved.Coins,coins+4321);
    saved.LastNeonSpin.Response.RulesVersion=4;
    assert.equal(context.handlers.NeonNightsSpin({SpinId:'v4',Bet:100}).RulesVersion,5);
    assert.equal(saved.Coins,coins+4321);
}
console.log('PASS: Neon rules-4 receipts recover under rules 5 without reroll, double pay or duplicate free-spin consumption.');
