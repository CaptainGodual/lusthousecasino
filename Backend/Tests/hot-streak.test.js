// Trusted rule tests plus the real PlayFab bridge -> gateway -> atomic storage.
const fs=require('fs'),path=require('path'),vm=require('vm'),crypto=require('crypto'),assert=require('assert/strict');
const root=path.resolve(__dirname,'../..'),clone=x=>JSON.parse(JSON.stringify(x));
const source=fs.readFileSync(path.join(root,'Backend/NakamaRuntime/index.js'),'utf8');
const bridge=fs.readFileSync(path.join(root,'Backend/PlayFabCloudScript/cloudscript.js'),'utf8');
const fixture=JSON.parse(fs.readFileSync(path.join(__dirname,'fixtures/hot-streak-internal.json'),'utf8'));
const start=Date.parse('2026-11-01T00:00:00Z'),end=start+7*86400000,deadline=end+2*86400000;
function configuration(){const c=clone(fixture),d=c.Events[0];Object.assign(d,{StartsAt:new Date(start).toISOString(),EndsAt:new Date(end).toISOString(),ClaimUntil:new Date(deadline).toISOString(),EarningEnabled:true,ClaimsEnabled:true});return c;}
const eventId=fixture.ActiveEventId;
const key=r=>r.collection+'/'+r.key;
function setup(seed={Coins:10050000,XP:0},config=configuration()) {
 let time=start+1000,revision=0,interleave=null,fail=false,lost=false;
 class Clock extends Date {constructor(...args){super(...(args.length?args:[time]));}static now(){return time;}}
 const runtime=vm.createContext({Date:Clock,Math:Object.assign(Object.create(Math),{random:()=>0.5})});vm.runInContext(source,runtime);
 const db=new Map(),batches=[];
 const nk={sha256Hash:s=>crypto.createHash('sha256').update(s).digest('hex'),
  storageRead:reads=>reads.map(r=>db.get(key(r))).filter(Boolean).map(clone),
  storageWriteRetry(reads,fn,retries){for(let n=0;n<=retries;n++){
   const writes=fn(this.storageRead(reads));if(!writes.length)return;
   if(interleave){const f=interleave;interleave=null;f();}
   if(writes.some(w=>w.version!==(db.get(key(w))?.version||'*')))continue;
   if(fail)throw Error('injected commit failure');
   const names=new Set();for(const w of writes){assert(!names.has(key(w)));names.add(key(w));assert.equal(w.permissionRead,0);assert.equal(w.permissionWrite,0);}
   for(const w of writes)db.set(key(w),{...clone(w),version:String(++revision)});batches.push(clone(writes));return;
  }throw Error('version conflicts exhausted');}};
 function client(player='hot_test') {
  const cloud=vm.createContext({handlers:{},currentPlayerId:player,server:{
   GetTitleInternalData:()=>({Data:{SugarLustNakamaGatewayUrl:'https://local.invalid',SugarLustNakamaHttpKey:'test-only',SugarLustEconomyMigrationReady:'transactional-v1'}}),
   GetUserInternalData:()=>({Data:{SugarLustProfile:{Value:'frozen'}}})},
   http:{request(url,method,body){const r=runtime.rpcGateway({},null,nk,body);if(lost){lost=false;throw Error('lost response after commit');}return r;}}});
  vm.runInContext(bridge,cloud);return cloud.handlers;
 }
 runtime.rpcStageEconomyMigration({},nk,{SettlementVersion:1,PlayFabId:'hot_test',Profile:clone(seed)});
 if(config)runtime.rpcConfigureHotStreak({},nk,{Configuration:config,ExpectedVersion:'*'});
 let handlers=client();
 function outcome(win,extra={}){runtime.calculateWin=runtime.calculateNeonWin=()=>Object.assign({Win:win,ScatterCount:0,FreeSpinCount:0,JackpotCounts:[0,0,0,0]},extra);}
 outcome(1);
 const call=(name,args={})=>clone(handlers[name](args));
 const spin=(id,win=1,game='Spin',extra={})=>{outcome(win,extra);return call(game,game==='Spin'?{SpinId:id,BetPerLine:10,ActiveLines:20}:{SpinId:id,Bet:100});};
 return {runtime,db,nk,batches,call,spin,outcome,clock:t=>time=t,interleave:f=>interleave=f,fail:v=>fail=v,lose:()=>lost=true,
  account:()=>db.get('slot_economy/account_hot_test').value,
  progress:()=>db.get('slot_economy/account_hot_test')?.value.Profile.HotStreak?.[eventId],
  config:()=>db.get('event_config/hot_streak'),
  configure:c=>runtime.rpcConfigureHotStreak({},nk,{Configuration:c,ExpectedVersion:db.get('event_config/hot_streak')?.version||'*'}),
  restart:()=>{vm.runInContext(source,runtime);handlers=client();outcome(1);}};
}
let passed=0;function test(name,fn){fn();passed++;console.log('PASS '+name);}
function earned(e){e.spin('win1');e.spin('win2');return e.spin('win3');}
const claim=(id='claimA',tier='Mild')=>({ClaimId:id,EventId:eventId,TierId:tier});

test('W W W W L counts small base wins and banks Heat 0 0 1 2 2',()=>{
 const e=setup(),heats=[],streaks=[];for(const [i,w] of [1,1,1,1,0].entries()){const r=e.spin('sequence'+i,w);heats.push(r.Profile.HotStreak.Progress.Heat);streaks.push(r.HotStreakDelta.StreakAfter);}
 assert.deepEqual(heats,[0,0,1,2,2]);assert.deepEqual(streaks,[1,2,3,4,0]);assert.equal(e.progress().BestStreak,4);assert.deepEqual(e.progress().UnlockedTierIds,['Mild']);
});
test('both games share the paid streak and wager changes preserve it',()=>{const e=setup();e.spin('sugar');e.spin('neon',1,'NeonNightsSpin');const r=e.call('Spin',{SpinId:'changedbet',BetPerLine:1,ActiveLines:1});assert.equal(r.HotStreakDelta.HeatAdded,1);});
test('thresholds unlock once through Inferno without multiplying payouts',()=>{const e=setup();for(let i=0;i<32;i++){const r=e.spin('ladder'+i);assert.equal(r.Win,1);}assert.equal(e.progress().Heat,30);assert.deepEqual(e.progress().UnlockedTierIds,['Mild','Hot','ExtraHot','Inferno']);});
test('phase boundaries and emergency earning pause are exact',()=>{const e=setup(),R=e.runtime.HotStreakRules,d=configuration().Events[0];for(const [t,p] of [[start-1,'Upcoming'],[start,'Active'],[end-1,'Active'],[end,'ClaimOnly'],[deadline-1,'ClaimOnly'],[deadline,'Archived']])assert.equal(R.phase(d,t),p);d.EarningEnabled=false;assert.equal(R.phase(d,start),'Disabled');});
test('all excluded rule modes freeze streak, Heat and progress revision',()=>{const e=setup(),R=e.runtime.HotStreakRules,d=configuration().Events[0],s=R.create(d);s.CurrentStreak=s.BestStreak=4;s.Heat=2;s.UnlockedTierIds=['Mild'];for(const Mode of ['FreeSpin','CandyJar','NeonHeat','Offline','SpinDuel','MegaLustPot']){const r=R.evaluate(d,start,s,{Mode,Game:'SugarLust',BaseWin:100,SpinId:'excluded'});assert.deepEqual(clone(r.State),clone(s));assert.equal(r.Delta.Eligible,false);}});
test('Sugar free spins and retriggers cannot reset an established streak',()=>{const e=setup();earned(e);const p=e.account().Profile;p.FreeSpins.SugarLust={Remaining:1,BetPerLine:10,ActiveLines:20,TotalWin:0,TotalAwarded:10};const before=clone(e.progress());e.spin('free-loss',0,'Spin',{FreeSpinCount:3});assert.deepEqual(e.progress(),before);assert.equal(e.account().Profile.FreeSpins.SugarLust.Remaining,5);});
test('paid bonus trigger counts once and Candy Jar picks freeze progress',()=>{const e=setup();e.spin('a');e.spin('b');e.spin('trigger',1,'Spin',{ScatterCount:3});assert.equal(e.progress().Heat,1);const before=clone(e.progress());for(let i=0;i<3;i++)e.call('PickBonusCard',{Card:i});assert.deepEqual(e.progress(),before);});
test('Neon Heat respins and final collected payout freeze progress',()=>{const e=setup();e.spin('a');e.spin('b');e.spin('trigger',1,'NeonNightsSpin',{ScatterCount:3});const before=clone(e.progress());e.account().Profile.NeonHeatBonus.RespinsRemaining=1;e.spin('respin',0,'NeonNightsSpin');assert.deepEqual(e.progress(),before);});
test('jackpot-only paid result resets streak despite its positive total Win',()=>{const e=setup();earned(e);const r=e.spin('jackpot',0,'Spin',{JackpotCounts:[3,0,0,0]});assert(r.Win>0);assert.equal(r.BaseWin,0);assert.equal(e.progress().CurrentStreak,0);assert.equal(e.progress().Heat,1);});
test('older receipt replays immutable delta with a fresh snapshot after event end',()=>{const e=setup(),a=earned(e);e.spin('newer');e.clock(deadline);const before=JSON.stringify([...e.db]),r=e.spin('win3');assert(r.Replayed);assert.deepEqual(r.HotStreakDelta,a.HotStreakDelta);assert.equal(r.Profile.HotStreak.Progress.Heat,2);assert.equal(r.Profile.HotStreak.Phase,'Archived');assert.equal(JSON.stringify([...e.db]),before);});
test('lost settled reply and runtime restart cannot award a second Heat',()=>{const e=setup();e.spin('a');e.spin('b');e.lose();assert.throws(()=>e.spin('lost'),/lost response/);e.restart();const r=e.spin('lost');assert(r.Replayed);assert.equal(e.progress().Heat,1);});
test('concurrent paid wins serialize without lost streak updates',()=>{const e=setup();e.call('GetHotStreakState');e.interleave(()=>e.spin('inner'));e.spin('outer');e.spin('third');assert.equal(e.progress().CurrentStreak,3);assert.equal(e.progress().Heat,1);});
test('failed commit leaves wallet, Heat, pools and permanent receipt untouched',()=>{const e=setup();e.spin('a');e.spin('b');const before=JSON.stringify([...e.db]);e.fail(true);assert.throws(()=>e.spin('fail'),/commit failure/);assert.equal(JSON.stringify([...e.db]),before);e.fail(false);e.spin('fail');assert.equal(e.progress().Heat,1);});
test('CAS retry pins the original admission time across the end boundary',()=>{const e=setup();e.spin('a');e.spin('b');e.clock(end-1);e.interleave(()=>{e.clock(end);e.spin('at-end');});e.spin('admitted');assert.equal(e.progress().Heat,1);});
test('configuration disable racing settlement prevents new earning',()=>{const e=setup();e.spin('a');e.spin('b');e.interleave(()=>{const c=configuration();c.Events[0].EarningEnabled=false;e.configure(c);});e.spin('disabled');assert.equal(e.progress().Heat,0);assert.equal(e.progress().CurrentStreak,2);});
test('Mild claim atomically owns a supported title and debits no currency',()=>{const e=setup();earned(e);const coins=e.account().Profile.Coins;const r=e.call('ClaimHotStreakReward',claim());assert(r.Claimed);assert.equal(e.account().Profile.Coins,coins);const owned=e.db.get('lustvend/account_hot_test').value;assert(owned.owned.includes(r.RewardId));assert.equal(owned.cosmetics[r.RewardId].RewardKind,'Presentation');const batch=e.batches.at(-1);for(const prefix of ['account_hot_test','event_grant_','receipt_'])assert(batch.some(w=>w.key.startsWith(prefix)));});
test('different ClaimIds racing for Mild grant one entitlement and ledger',()=>{const e=setup();earned(e);e.interleave(()=>e.call('ClaimHotStreakReward',claim('inner')));const r=e.call('ClaimHotStreakReward',claim('outer'));assert(r.AlreadyClaimed);assert.equal([...e.db.values()].filter(r=>r.key.startsWith('event_grant_')).length,1);assert.deepEqual(e.progress().ClaimedTierIds,['Mild']);});
test('same ClaimId for a different tier conflicts before any mutation',()=>{const e=setup();earned(e);e.call('ClaimHotStreakReward',claim());const before=JSON.stringify([...e.db]);assert.throws(()=>e.call('ClaimHotStreakReward',claim('claimA','Hot')),/conflicts with original/);assert.equal(JSON.stringify([...e.db]),before);});
test('lost claim reply replays after deadline but a new late claim is rejected',()=>{const e=setup();earned(e);e.lose();assert.throws(()=>e.call('ClaimHotStreakReward',claim()),/lost response/);e.clock(deadline);assert(e.call('ClaimHotStreakReward',claim()).Replayed);assert.throws(()=>e.call('ClaimHotStreakReward',claim('late')),/claims are closed/);});
test('failed claim commits neither ownership, marker nor receipt',()=>{const e=setup();earned(e);const before=JSON.stringify([...e.db]);e.fail(true);assert.throws(()=>e.call('ClaimHotStreakReward',claim()),/commit failure/);assert.equal(JSON.stringify([...e.db]),before);});
test('preview wearable tiers cannot grant even with sufficient Heat',()=>{const e=setup();for(let i=0;i<7;i++)e.spin('w'+i);assert.throws(()=>e.call('ClaimHotStreakReward',claim('hot','Hot')),/preview/);assert(!e.progress().ClaimedTierIds.includes('Hot'));});
test('title equipment validates ownership and slot and survives reconnect',()=>{const e=setup();earned(e);const reward=fixture.Events[0].Tiers[0].RewardId,args={EquipId:'equip',RewardId:reward,Slot:'EventBadge'};assert.throws(()=>e.call('EquipCasinoCosmetic',args),/unowned or incompatible/);e.call('ClaimHotStreakReward',claim());assert.throws(()=>e.call('EquipCasinoCosmetic',{...args,Slot:'Outfit'}),/incompatible/);e.call('EquipCasinoCosmetic',args);e.restart();assert.equal(e.call('GetHotStreakState').Profile.HotStreak.EquippedRewardId,reward);});
test('claims ignore forged player, progress, reward and price fields',()=>{const e=setup();earned(e);const r=e.call('ClaimHotStreakReward',{...claim(),PlayFabId:'victim',RewardId:'forged',Heat:9999,Coins:999999});assert.equal(r.RewardId,fixture.Events[0].Tiers[0].RewardId);assert(!e.db.has('slot_economy/account_victim'));});
test('missing or corrupt event config does not break an otherwise valid spin',()=>{for(const config of [null,{SchemaVersion:999}]){const e=setup(undefined,null);if(config)e.db.set('event_config/hot_streak',{collection:'event_config',key:'hot_streak',value:config,version:'bad'});const r=e.spin('normal');assert.equal(r.Win,1);assert.equal(r.Profile.HotStreak.Available,false);}});
test('additive initialization preserves missions, balances and old seasons',()=>{const old={keep:{Heat:99}},e=setup({Coins:4321,XP:7,HotStreak:old});const r=e.call('GetHotStreakState');assert.equal(r.Profile.Coins,4321);assert.equal(e.account().Profile.XP,7);assert.deepEqual(e.account().Profile.HotStreak.keep,{Heat:99});assert.equal(e.progress().Heat,0);});
test('corrupt saved event progress is preserved and earning is disabled',()=>{const seed={Coins:1000,XP:0,HotStreak:{[eventId]:{Heat:'bad'}}},e=setup(seed),r=e.spin('normal');assert.equal(r.Profile.HotStreak.Available,false);assert.deepEqual(e.account().Profile.HotStreak[eventId],{Heat:'bad'});});
test('claims remain available during an emergency earning pause',()=>{const e=setup();earned(e);const c=configuration();c.Events[0].EarningEnabled=false;e.configure(c);assert(e.call('ClaimHotStreakReward',claim()).Claimed);assert.equal(e.progress().Heat,1);});
test('already owned permanent reward is collected without currency compensation',()=>{const e=setup();earned(e);const state=e.db.get('lustvend/account_hot_test').value;state.owned.push(fixture.Events[0].Tiers[0].RewardId);const balance=state.balance,coins=e.account().Profile.Coins;const r=e.call('ClaimHotStreakReward',claim());assert(r.AlreadyOwned);assert.equal(e.db.get('lustvend/account_hot_test').value.balance,balance);assert.equal(e.account().Profile.Coins,coins);assert.deepEqual(e.progress().ClaimedTierIds,['Mild']);});
test('new season starts fresh and an older valid claim keeps the active projection',()=>{const e=setup();earned(e);const c=configuration(),next=clone(c.Events[0]);next.EventId='pepper_hot_streak_internal_v2';c.Events.push(next);c.ActiveEventId=next.EventId;e.configure(c);const fresh=e.call('GetHotStreakState').Profile.HotStreak;assert.equal(fresh.EventId,next.EventId);assert.equal(fresh.Progress.Heat,0);const claimed=e.call('ClaimHotStreakReward',claim());assert(claimed.Claimed);assert.equal(claimed.Profile.HotStreak.EventId,next.EventId);assert.equal(e.progress().Heat,1);});
test('rules are immutable and config changes use a version guard',()=>{const e=setup(),c=configuration();c.Events[0].Tiers[0].HeatRequired=2;assert.throws(()=>e.configure(c),/immutable/);assert.throws(()=>e.runtime.rpcConfigureHotStreak({},e.nk,{Configuration:configuration(),ExpectedVersion:'stale'}),/version changed/);assert.throws(()=>e.runtime.rpcConfigureHotStreak({userId:'player'},e.nk,{}),/server-to-server/);});
console.log(passed+' Hot Streak rule and transactional integration tests passed.');
