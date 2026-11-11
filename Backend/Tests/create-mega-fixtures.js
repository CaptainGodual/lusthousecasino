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


const e=setup();const state=e.call('MegaLustPotState');let s=e.runtime.MegaPotRules.create();s.Phase='Hold';s.Respins=1;for(let i=0;i<6;i++)s.Locked[i]=200;let f=setup({Coins:0,XP:0,MegaPot:s});const result=f.call('MegaLustPotSpin',{SpinId:'fixture-only',Bet:10,ExpectedRevision:0});fs.mkdirSync(path.join(root,'Backend/Tests/fixtures'),{recursive:true});fs.writeFileSync(path.join(root,'Backend/Tests/fixtures/mega-pot-state.json'),JSON.stringify(state));fs.writeFileSync(path.join(root,'Backend/Tests/fixtures/mega-pot-result.json'),JSON.stringify(result));console.log('Fake-account fixtures written.');
