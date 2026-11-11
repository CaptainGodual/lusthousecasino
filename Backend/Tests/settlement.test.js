// Full PlayFab bridge -> registered Nakama gateway -> conditional storage batch.
// Fake local accounts only. Reentrant calls deliberately interleave transactions.
const fs=require('fs'),path=require('path'),vm=require('vm'),crypto=require('crypto'),assert=require('assert/strict');
const root=path.resolve(__dirname,'../..'),clone=x=>JSON.parse(JSON.stringify(x));
const runtimeSource=fs.readFileSync(path.join(root,'Backend/NakamaRuntime/index.js'),'utf8');
const bridgeSource=fs.readFileSync(path.join(root,'Backend/PlayFabCloudScript/cloudscript.js'),'utf8');
const key=r=>r.collection+'/'+r.key;
let passed=0;
function setup(seed={Coins:10050000,XP:0}) {
 const runtime=vm.createContext({Math:Object.assign(Object.create(Math),{random:()=>0.5})});
 vm.runInContext(runtimeSource,runtime);
 const db=new Map();let revision=0,interleave=null,fail=false,lost=false;const batches=[];
 const nk={sha256Hash:s=>crypto.createHash('sha256').update(s).digest('hex'),
  storageRead:reads=>reads.map(r=>db.get(key(r))).filter(Boolean).map(clone),
  storageWriteRetry(reads,fn,maxRetries) {
   for(let attempt=0;attempt<=maxRetries;++attempt) {
    const writes=fn(this.storageRead(reads));
    if(!writes.length)return [];
    if(interleave){const hook=interleave;interleave=null;hook();}
    const mismatch=writes.some(w=>{const old=db.get(key(w));return w.version!==(old?old.version:'*');});
    if(mismatch)continue;
    if(fail)throw Error('injected commit failure');
    // Check the entire batch before applying any part of it.
    const names=new Set();for(const w of writes){assert(!names.has(key(w)));names.add(key(w));assert.equal(w.permissionRead,0);assert.equal(w.permissionWrite,0);}
    for(const w of writes)db.set(key(w),{...clone(w),version:String(++revision)});
    batches.push(clone(writes));return [];
   }
   throw Error('version conflicts exhausted');
  }};
 let legacy=clone(seed),calls=0;
 function client(player='audit') {
  const cloud=vm.createContext({handlers:{},currentPlayerId:player,server:{
   GetTitleInternalData:()=>({Data:{SugarLustNakamaGatewayUrl:'https://local.invalid',SugarLustNakamaHttpKey:'local-test-only',SugarLustEconomyMigrationReady:'transactional-v1'}}),
   GetUserInternalData:()=>({Data:{SugarLustProfile:{Value:JSON.stringify(legacy)}}}),
   UpdateUserInternalData:()=>{throw Error('legacy PlayFab write forbidden');}
  },http:{request(url,method,body){++calls;const value=runtime.rpcGateway({},null,nk,body);if(lost){lost=false;throw Error('injected response loss AFTER commit');}return value;}}});
  vm.runInContext(bridgeSource,cloud);return cloud.handlers;
 }
 runtime.rpcStageEconomyMigration({},nk,{SettlementVersion:1,PlayFabId:'audit',Profile:clone(seed)});
 const handlers=client();
 const call=(action,args={},player)=>clone((player?client(player):handlers)[action](args));
 return {runtime,nk,db,batches,handlers,call,client,
  seed:()=>legacy,replaceSeed:s=>legacy=clone(s),
  interleave:f=>interleave=f,fail:v=>fail=v,lose:()=>lost=true,
  coins:()=>db.get('slot_economy/account_audit').value.Profile.Coins,
  account:()=>db.get('slot_economy/account_audit').value,
  rpc:r=>JSON.parse(runtime.rpcGateway({},null,nk,JSON.stringify(r)))};
}
function test(name,fn){fn();++passed;console.log('PASS '+name);}
const args=(game,id,bet=100)=>game==='Spin'?{SpinId:id,BetPerLine:bet,ActiveLines:1}:{SpinId:id,Bet:bet};
function zero(e){e.runtime.calculateWin=()=>({Win:0,ScatterCount:0,FreeSpinCount:0,JackpotCounts:[0,0,0,0]});e.runtime.calculateNeonWin=e.runtime.calculateWin;}
function receiptCount(e){return [...e.db.values()].filter(r=>r.collection==='slot_economy'&&r.key.startsWith('receipt_')).length;}
for(const game of ['Spin','NeonNightsSpin']) {
 test(game+' A-B-A replay preserves balance, XP, features and pools',()=>{
  const e=setup();zero(e);
  e.runtime.calculateWin=e.runtime.calculateNeonWin=()=>({Win:2500,ScatterCount:0,FreeSpinCount:0,JackpotCounts:[0,0,0,0]});
  const a=e.call(game,args(game,'A'));
  e.call(game,args(game,'B'));
  const before=JSON.stringify([...e.db]);const replay=e.call(game,args(game,'A'));
  assert.equal(replay.Win,a.Win);assert.equal(JSON.stringify([...e.db]),before);assert.equal(replay.Profile.Coins,e.coins());
  assert.throws(()=>e.call(game,args(game,'A',1000)),/conflicts with original/);
  assert.equal(JSON.stringify([...e.db]),before);
 });
 test(game+' durable replay survives cache eviction and runtime restart',()=>{
  const e=setup();zero(e);e.call(game,args(game,'old'));
  for(let i=0;i<25;i++)e.call(game,args(game,'other-'+i));
  const before=JSON.stringify([...e.db]);vm.runInContext(runtimeSource,e.runtime);zero(e);
  e.call(game,args(game,'old'));assert.equal(JSON.stringify([...e.db]),before);assert.equal(receiptCount(e),26);
 });
 test(game+' response loss after atomic commit replays once',()=>{
  const e=setup();zero(e);e.lose();assert.throws(()=>e.call(game,args(game,'lost')),/response loss/);
  const balance=e.coins();e.call(game,args(game,'lost'));assert.equal(e.coins(),balance);assert.equal(receiptCount(e),1);
 });
 test(game+' storage failure rolls back wallet, jackpot, receipt and gems',()=>{
  const e=setup();e.call('GetOrMigratePlayerProfile');const before=JSON.stringify([...e.db]);e.fail(true);
  assert.throws(()=>e.call(game,args(game,'failed')),/commit failure/);assert.equal(JSON.stringify([...e.db]),before);
  e.fail(false);e.call(game,args(game,'failed'));assert.equal(receiptCount(e),1);
  const batch=e.batches.at(-1);assert(batch.some(r=>r.collection==='slot_economy'));assert(batch.some(r=>r.key==='progressive_jackpots'));assert(batch.some(r=>r.collection==='lustvend'));
 });
 test(game+' concurrent duplicate settles once after CAS retry',()=>{
  const e=setup();zero(e);e.call('GetOrMigratePlayerProfile');e.interleave(()=>e.call(game,args(game,'same')));
  e.call(game,args(game,'same'));assert.equal(e.coins(),10050000-(game==='Spin'?100:500));assert.equal(receiptCount(e),1);
 });
}
test('cross-game stale transaction recomputes against the new balance',()=>{
 const e=setup();zero(e);e.call('GetOrMigratePlayerProfile');
 e.interleave(()=>e.call('NeonNightsSpin',args('NeonNightsSpin','B',10000)));
 e.call('Spin',args('Spin','A',1));assert.equal(e.coins(),9999999);assert.equal(receiptCount(e),2);
 assert.equal(e.account().Profile.XP,70); // 60 XP Neon + 10 XP Sugar.
});
test('concurrent wagers cannot spend the same available coins',()=>{
 const e=setup({Coins:500,XP:0});zero(e);e.call('GetOrMigratePlayerProfile');
 e.interleave(()=>e.call('NeonNightsSpin',args('NeonNightsSpin','B')));
 assert.throws(()=>e.call('NeonNightsSpin',args('NeonNightsSpin','A')),/Not enough coins/);
 assert.equal(e.coins(),0);assert.equal(receiptCount(e),1);
});
test('migration is once-only and client seed/identity/coins cannot override it',()=>{
 const e=setup({Coins:321,XP:7});const r=e.call('GetOrMigratePlayerProfile',{Coins:999999,PlayFabId:'victim',SeedProfile:{Coins:99999}});assert.equal(r.Coins,321);
 e.replaceSeed({Coins:9000000,XP:0});assert.equal(e.call('GetOrMigratePlayerProfile').Coins,321);
 assert(!e.db.has('slot_economy/account_victim'));assert.equal(e.account().Profile.XP,7);
});
test('new account concurrent migration imports once and preserves both wagers',()=>{
 const e=setup();zero(e);e.interleave(()=>e.call('NeonNightsSpin',args('NeonNightsSpin','B')));
 e.call('Spin',args('Spin','A'));assert.equal(e.coins(),10049400);assert.equal(receiptCount(e),2);
});
test('daily claim and wager share one conditional wallet row',()=>{
 const e=setup();zero(e);e.call('GetOrMigratePlayerProfile');e.interleave(()=>e.call('ClaimDailyBonus'));
 e.call('Spin',args('Spin','A'));assert.equal(e.coins(),10099900);
 assert.throws(()=>e.call('ClaimDailyBonus'),/already been claimed/);assert.equal(e.coins(),10099900);
});
test('completed mission claim racing a spin cannot overwrite either award or debit',()=>{
 const e=setup();zero(e);e.call('GetOrMigratePlayerProfile');
 const profile=e.account().Profile;profile.Missions[0].Progress=10;
 e.interleave(()=>e.call('ClaimMissionReward',{MissionId:'spin_10',ClaimId:'claim'}));
 e.call('Spin',args('Spin','A'));assert.equal(e.coins(),10074900);
 const before=JSON.stringify([...e.db]);e.call('ClaimMissionReward',{MissionId:'spin_10',ClaimId:'claim'});assert.equal(JSON.stringify([...e.db]),before);
 assert.throws(()=>e.call('ClaimMissionReward',{MissionId:'win_50000',ClaimId:'claim'}),/conflicts with original/);
});
test('final Candy Jar payout racing another game cannot overwrite its debit',()=>{
 const e=setup({Coins:1000,XP:0,BonusState:{ScatterCount:3,Prizes:[10,20,30,40,50,60],Revealed:[true,true,false,false,false,false],PicksRemaining:1,Total:30}});zero(e);
 e.call('GetOrMigratePlayerProfile');e.interleave(()=>e.call('PickBonusCard',{Card:2}));
 e.call('NeonNightsSpin',args('NeonNightsSpin','A'));assert.equal(e.coins(),560);assert.equal(e.account().Profile.BonusState,null);
 assert.throws(()=>e.call('PickBonusCard',{Card:2}),/no active bonus/);assert.equal(e.coins(),560);
});
test('free-spin retry never consumes another spin or trusts a new client wager',()=>{
 const e=setup({Coins:0,XP:0,FreeSpins:{NeonNights:{Remaining:2,BetPerLine:100,ActiveLines:5,TotalWin:0,TotalAwarded:10}}});zero(e);
 e.call('NeonNightsSpin',args('NeonNightsSpin','free',10000));assert.equal(e.coins(),0);assert.equal(e.account().Profile.FreeSpins.NeonNights.Remaining,1);
 const before=JSON.stringify([...e.db]);e.call('NeonNightsSpin',args('NeonNightsSpin','free',10000));assert.equal(JSON.stringify([...e.db]),before);
 assert.throws(()=>e.call('NeonNightsSpin',args('NeonNightsSpin','free',100)),/conflicts with original/);
});
test('concurrent final free spins recompute mode and reject unpaid continuation',()=>{
 const e=setup({Coins:0,XP:0,FreeSpins:{SugarLust:{Remaining:1,BetPerLine:100,ActiveLines:1,TotalWin:0,TotalAwarded:10}}});zero(e);
 e.call('GetOrMigratePlayerProfile');e.interleave(()=>e.call('Spin',args('Spin','B')));
 assert.throws(()=>e.call('Spin',args('Spin','A')),/Not enough coins/);assert.equal(e.account().Profile.FreeSpins.SugarLust.Remaining,0);
});
test('legacy recent and evicted marked receipts fail closed rather than reroll',()=>{
 const e=setup();zero(e);
 function put(collection,key,value){e.db.set(collection+'/'+key,{collection,key,userId:e.runtime.SYSTEM_USER_ID,value,version:'legacy'});}
 put('sugar_lust','spin_receipts_audit',{Recent:[{SpinId:'old',Result:{Win:999}}]});
 put('lustvend','played_audit_neon_evicted',{at:'old'});
 assert.throws(()=>e.call('Spin',args('Spin','old')),/reconciliation/);
 assert.throws(()=>e.call('NeonNightsSpin',args('NeonNightsSpin','evicted')),/reconciliation/);
 assert(!e.db.has('slot_economy/account_audit'));
});
test('legacy known settled last response imports without paying or rerolling',()=>{
 const e=setup({Coins:1200,XP:1,LastSpin:{SpinId:'known',Response:{Win:700,RulesVersion:4,Profile:{Coins:1200}}}});
 const r=e.call('Spin',args('Spin','known'));assert.equal(r.Win,700);assert.equal(e.coins(),1200);assert.equal(receiptCount(e),1);
 e.call('Spin',args('Spin','known'));assert.equal(e.coins(),1200);
});
test('registered RPC denies end users, unsupported contracts and legacy spin mutations',()=>{
 const e=setup();let gateway;e.runtime.InitModule({}, {info(){}},e.nk,{registerRpc:(name,fn)=>gateway=fn});
 assert.throws(()=>gateway({userId:'user'},null,e.nk,JSON.stringify({Operation:'Economy'})),/reserved/);
 assert.throws(()=>gateway({},null,e.nk,JSON.stringify({Operation:'Spin'})),/Legacy spin RPC disabled/);
 assert.throws(()=>e.rpc({Operation:'Economy',SettlementVersion:0}),/Unsupported/);
 assert.equal(e.handlers.DebugTriggerBonus,undefined);
});
test('bridge never falls back to PlayFab settlement if runtime contract is old',()=>{
 const cloud=vm.createContext({handlers:{},currentPlayerId:'test',server:{GetUserInternalData:()=>({Data:{}}),GetTitleInternalData:()=>({Data:{SugarLustNakamaGatewayUrl:'https://local.invalid',SugarLustNakamaHttpKey:'local'}})},http:{request:()=>JSON.stringify({Jackpots:{},Win:99})}});
 vm.runInContext(bridgeSource,cloud);assert.throws(()=>cloud.handlers.Spin(args('Spin','old-runtime')),/backend is not deployed/);
});
test('jackpot award, reset and coin credit are one settlement across two players',()=>{
 const e=setup();zero(e);
 e.runtime.calculateWin=()=>({Win:0,ScatterCount:0,FreeSpinCount:0,JackpotCounts:[3,0,0,0]});
 e.call('GetOrMigratePlayerProfile');e.runtime.rpcStageEconomyMigration({},e.nk,{SettlementVersion:1,PlayFabId:'other',Profile:{Coins:10050000,XP:0}});e.call('GetOrMigratePlayerProfile',{},'other');
 let other;
 e.interleave(()=>other=e.call('Spin',args('Spin','other-win'),'other'));
 const own=e.call('Spin',args('Spin','own-win'));
 assert.equal(own.JackpotWin.Tier,'Mini');assert.equal(other.JackpotWin.Tier,'Mini');
 assert.equal(e.coins(),10049900+own.Win);
 assert.equal(e.db.get('slot_economy/account_other').value.Profile.Coins,10049900+other.Win);
 const before=JSON.stringify([...e.db]);e.call('Spin',args('Spin','own-win'));assert.equal(JSON.stringify([...e.db]),before);
 assert.equal(e.db.get('sugar_lust/progressive_jackpots').value.Mini,e.runtime.SEEDS.Mini);
});
test('Neon Heat completion racing another wager pays once and pauses free spins',()=>{
 const e=setup({Coins:1000,XP:0,NeonHeatBonus:{Active:true,Bet:100,TotalBet:500,RespinsRemaining:1,Total:250,Locked:[{Index:0,Prize:250}],FreeSpinFeature:true},FreeSpins:{NeonNights:{Remaining:2,BetPerLine:100,ActiveLines:5,TotalWin:0,TotalAwarded:10}}});zero(e);
 e.call('GetOrMigratePlayerProfile');e.interleave(()=>e.call('Spin',args('Spin','paid')));
 const result=e.call('NeonNightsSpin',args('NeonNightsSpin','heat'));
 assert.equal(result.Win,250);assert.equal(result.Bonus.Complete,true);assert.equal(e.coins(),1150);
 assert.equal(e.account().Profile.FreeSpins.NeonNights.Remaining,2);assert.equal(e.account().Profile.FreeSpins.NeonNights.TotalWin,250);
 const before=JSON.stringify([...e.db]);e.call('NeonNightsSpin',args('NeonNightsSpin','heat'));assert.equal(JSON.stringify([...e.db]),before);
});
test('legacy settled rules-3 result is upgraded without another credit',()=>{
 const e=setup({Coins:100,XP:0,LastSpin:{SpinId:'known-v3',Response:{Win:50,RulesVersion:3,Profile:{Coins:100}}}});
 const r=e.call('Spin',args('Spin','known-v3'));assert.equal(r.RulesVersion,4);assert.equal(r.RecoveredLegacyReceipt,true);assert.equal(e.coins(),100);
});
test('gem purchase racing a spin milestone preserves ownership, debit and grant',()=>{
 const e=setup();zero(e);e.call('GetOrMigratePlayerProfile');e.handlers.LustVendState();
 const gem=e.db.get('lustvend/account_audit');gem.value.balance=100;gem.value.earning.spins=9;
 e.interleave(()=>e.handlers.LustVendPurchase({requestId:'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa',sku:'vid-chip',version:'lustvend-1'}));
 e.call('Spin',args('Spin','milestone'));
 const state=e.db.get('lustvend/account_audit').value;assert.equal(state.balance,45);assert.deepEqual(state.owned,['vid-chip']);assert.equal(state.earning.spins,10);
});
test('client free-spin and balance overrides never change settlement authority',()=>{
 const e=setup();zero(e);e.call('Spin',{...args('Spin','override'),FreeSpin:true,Coins:999999999,PlayFabId:'victim'});
 assert.equal(e.coins(),10049900);assert(!e.db.has('slot_economy/account_victim'));
});
test('migration import gate is enforced on the server',()=>{
 const e=setup();assert.throws(()=>e.rpc({Operation:'Economy',SettlementVersion:1,PlayFabId:'new',Action:'GetOrMigratePlayerProfile',Args:{},SeedProfile:{Coins:99,XP:0}}),/maintenance/);
 assert.equal(e.db.size,1);
});
test('legacy profile cannot be imported from forged bridge seeds or live mutable data',()=>{
 const e=setup();assert.throws(()=>e.rpc({Operation:'Economy',SettlementVersion:1,MigrationReady:true,PlayFabId:'unstaged',Action:'GetOrMigratePlayerProfile',Args:{},HasLegacyProfile:true,SeedProfile:{Coins:999999,XP:0}}),/frozen legacy import/);
 assert(!e.db.has('slot_economy/account_unstaged'));
});
test('new players use server defaults, never a supplied seed',()=>{
 const e=setup();const r=e.rpc({Operation:'Economy',SettlementVersion:1,MigrationReady:true,PlayFabId:'new',Action:'GetOrMigratePlayerProfile',Args:{},HasLegacyProfile:false,SeedProfile:{Coins:99999999,XP:90}});
 assert.equal(r.Result.Coins,10050000);assert.equal(r.Result.XP,0);
});
test('frozen imports cannot be overwritten and migrated accounts cannot be reseeded',()=>{
 const e=setup();assert.throws(()=>e.rpc({Operation:'StageMigration',SettlementVersion:1,PlayFabId:'audit',Profile:{Coins:999,XP:0}}),/No overwrite/);
 e.call('GetOrMigratePlayerProfile');const balance=e.coins();
 assert.equal(e.rpc({Operation:'StageMigration',SettlementVersion:1,PlayFabId:'audit',Profile:{Coins:999,XP:0}}).Status,'already_migrated');assert.equal(e.coins(),balance);
});
console.log(passed+' transactional economy integration tests passed.');
