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

const action='MegaLustPotSpin';
function ready(seed){const e=setup(seed);e.call('MegaLustPotState');return e;}
function args(e,id,bet=10){return {SpinId:id,Bet:bet,ExpectedRevision:e.account().Profile.MegaPot.Revision};}
test('Mega atomic wallet delta and immutable outcome',()=>{const e=ready(),before=e.coins(),r=e.call(action,args(e,'A'));assert.equal(e.coins(),before-r.Debit+r.Win);assert.equal(r.State.Revision,2);const db=JSON.stringify([...e.db]);e.call(action,{SpinId:'A',Bet:10,ExpectedRevision:1});assert.equal(JSON.stringify([...e.db]),db);assert.throws(()=>e.call(action,{SpinId:'A',Bet:20,ExpectedRevision:1}),/conflicts/);});
test('Mega A-B-A replay retains current profile and no second settlement',()=>{const e=ready(),a=args(e,'A');e.call(action,a);e.call(action,args(e,'B'));let db=JSON.stringify([...e.db]);let r=e.call(action,a);assert.equal(JSON.stringify([...e.db]),db);assert.equal(r.Profile.MegaPot.Revision,e.account().Profile.MegaPot.Revision);});
test('Mega lost response after commit can be replayed',()=>{const e=ready(),a=args(e,'lost');e.lose();assert.throws(()=>e.call(action,a),/response loss/);let c=e.coins(),rev=e.account().Revision;e.call(action,a);assert.equal(e.coins(),c);assert.equal(e.account().Revision,rev);});
test('Mega storage failure leaves coins, feature state and receipt unchanged',()=>{const e=ready(),a=args(e,'fail'),db=JSON.stringify([...e.db]);e.fail(true);assert.throws(()=>e.call(action,a),/commit failure/);assert.equal(JSON.stringify([...e.db]),db);e.fail(false);e.call(action,a);});
test('Mega concurrent duplicate settles one time',()=>{const e=ready(),a=args(e,'same');e.interleave(()=>e.call(action,a));const r=e.call(action,a);assert.equal([...e.db.values()].filter(v=>v.key.startsWith('receipt_')).length,1);assert.equal(e.coins(),10050000-r.Debit+r.Win);});
test('Mega stale distinct spins cannot consume the same state revision',()=>{const e=ready(),a=args(e,'outer'),b=args(e,'inner');e.interleave(()=>e.call(action,b));assert.throws(()=>e.call(action,a),/state changed/);assert.equal([...e.db.values()].filter(v=>v.key.startsWith('receipt_')).length,1);});
test('Mega synchronization cancels a late uncommitted request',()=>{const e=ready(),a=args(e,'late');e.interleave(()=>e.call('MegaLustPotState'));assert.throws(()=>e.call(action,a),/state changed/);assert.equal(e.coins(),10050000);});
test('Mega synchronization preserves already settled request',()=>{const e=ready(),a=args(e,'settled');e.call(action,a);const c=e.coins(),r=e.call('MegaLustPotState');assert.equal(r.Profile.Coins,c);assert(r.Synchronized);const snap=JSON.stringify([...e.db]);e.call(action,a);assert.equal(JSON.stringify([...e.db]),snap);});
test('Mega progress is isolated by wager',()=>{const e=ready();e.account().Profile.MegaPot.Pots={'1':[11,15,23]};e.call(action,args(e,'high',100));assert.deepEqual(e.account().Profile.MegaPot.Pots['1'],[11,15,23]);assert.deepEqual(e.account().Profile.MegaPot.Pots['100'],[0,0,0]);});
test('Mega forged state, wallet and outcomes are ignored by bridge',()=>{const e=ready(),a=args(e,'forge');Object.assign(a,{Coins:999999999,Free:100,Grid:Array(15).fill(16),PlayFabId:'victim',State:{Phase:'Result'}});let r=e.call(action,a);assert.equal(r.Kind,'Paid');assert.equal(r.State.Free,0);assert(!e.db.has('slot_economy/account_victim'));});
test('Mega cannot debit unavailable funds',()=>{const e=ready({Coins:199,XP:0}),db=JSON.stringify([...e.db]);assert.throws(()=>e.call(action,args(e,'poor')),/Not enough coins/);assert.equal(JSON.stringify([...e.db]),db);});
test('Mega free and hold states use server locked bet',()=>{let state=setup().runtime.MegaPotRules.create();state.Bet=10;state.Free=2;let e=ready({Coins:0,XP:0,MegaPot:state});let r=e.call(action,args(e,'free',100));assert.equal(r.Debit,0);assert.equal(r.State.Bet,10);assert.equal(r.State.Free,1);});
test('Mega hold completion pays once and survives a server restart',()=>{let state=setup().runtime.MegaPotRules.create();state.Bet=10;state.Phase='Hold';state.Respins=1;for(let i=0;i<6;i++)state.Locked[i]=200;const e=ready({Coins:0,XP:0,MegaPot:state}),a=args(e,'bonus');let r=e.call(action,a);assert.equal(r.Debit,0);assert.equal(r.Win,1200);assert.equal(r.State.Phase,'Result');vm.runInContext(runtimeSource,e.runtime);e.call(action,a);assert.equal(e.coins(),1200);assert.equal(e.call('MegaLustPotState').State.Phase,'Result');});
test('Mega corrupt state and mismatched version fail without writes',()=>{const e=ready();e.account().Profile.MegaPot.Version=99;const db=JSON.stringify([...e.db]);assert.throws(()=>e.call(action,args(e,'bad')),/Invalid Mega/);assert.equal(JSON.stringify([...e.db]),db);});
test('Mega and Sugar transactions preserve one shared wallet',()=>{const e=ready(),a=args(e,'mega');e.runtime.calculateWin=()=>({Win:0,ScatterCount:0,FreeSpinCount:0,JackpotCounts:[0,0,0,0]});e.interleave(()=>e.call('Spin',{SpinId:'sugar',BetPerLine:1,ActiveLines:1}));let r=e.call(action,a);assert.equal(e.coins(),10050000-1-r.Debit+r.Win);});
test('Mega RTP is exactly stake-linear across the minimum and maximum bet',()=>{const R=setup().runtime.MegaPotRules,a=R.create(),b=R.create();let x=7331,y=7331;const ra=()=>{x=(Math.imul(x,1664525)+1013904223)>>>0;return x/4294967296;},rb=()=>{y=(Math.imul(y,1664525)+1013904223)>>>0;return y/4294967296;};for(let i=0;i<20000;i++){let low=R.step(a,1,ra),high=R.step(b,100,rb);assert.equal(high.Win,low.Win*100);assert.equal(high.Debit,low.Debit*100);assert.equal(b.Phase,a.Phase);assert.equal(b.Free,a.Free);assert.equal(b.Royal,a.Royal);}});
console.log(passed+' Mega Pot transactional integration tests passed.');

