// Deterministic local contract/atomic-write tests. No network or live accounts.
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const runtime=vm.createContext({});
vm.runInContext(fs.readFileSync(__dirname+'/index.js','utf8'),runtime);
let records=new Map(),failWrite=false,conflict=null,clock='2026-10-26T12:00:00Z';
runtime.Date=class extends Date{constructor(...args){super(...(args.length?args:[clock]));}};
const clone=x=>JSON.parse(JSON.stringify(x));
const key=o=>o.collection+'/'+o.userId+'/'+o.key;
const nk={storageWriteRetry(reads,fn){
    for(let n=0;n<10;n++){
        const objects=reads.map(r=>records.get(key(r))).filter(Boolean).reverse().map(clone);
        const writes=fn(objects);
        if(conflict){const f=conflict;conflict=null;f();continue;}
        if(failWrite)throw Error('injected database failure');
        for(const w of writes){const old=records.get(key(w));assert.equal(w.version,old?old.version:'*');assert.equal(w.permissionRead,0);assert.equal(w.permissionWrite,0);}
        for(const w of writes)records.set(key(w),{...clone(w),version:String(Number(records.get(key(w))?.version||0)+1)});
        return;
    }
    throw Error('too many conflicts');
}};
const request=(action,extra={},player='player')=>JSON.parse(runtime.rpcProgressive({},null,nk,JSON.stringify({Operation:'LustVend',Action:action,PlayFabId:player,...extra})));
const purchase=(sku='vid-chip',id='aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa',player='player')=>request('Purchase',{Sku:sku,Version:'lustvend-1',RequestId:id},player);
const state=()=>request('Get');
const stateRow=()=>[...records.values()].find(r=>r.key==='account_player');
let passed=0;
function test(name,fn){records=new Map();failWrite=false;conflict=null;clock='2026-10-26T12:00:00Z';fn();passed++;console.log('PASS '+name);}
test('no client-authoritative identity',()=>assert.throws(()=>runtime.rpcProgressive({userId:'client'},null,nk,'{}')));
test('daily credit deduplicates',()=>{assert.equal(request('Daily').balance,10);assert.equal(request('Daily').balance,10);assert.equal(records.size,2);});
test('empty wallet never overspends',()=>{assert.equal(purchase().error,'insufficient_gems');assert.equal(records.size,0);});
function fund(){request('Daily');stateRow().value.balance=300;}
test('debit ownership receipt ledger commit together',()=>{fund();const r=purchase();assert.equal(r.balance,240);assert.deepEqual(state().owned,['vid-chip']);assert.equal(state().pending.length,1);assert.equal([...records.values()].filter(r=>r.key.startsWith('ledger_')).length,2);});
test('duplicate purchase returns original receipt',()=>{fund();assert.deepEqual(purchase(),purchase());assert.equal(state().balance,240);});
test('reusing request for another SKU rejected',()=>{fund();purchase();assert.equal(purchase('holo-disk').error,'idempotency_conflict');});
test('ownership prevents another debit',()=>{fund();purchase();assert.equal(purchase('vid-chip','bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb').error,'already_owned');assert.equal(state().balance,240);});
test('server prices catalog and request validation',()=>{fund();assert.equal(request('Purchase',{Sku:'vid-chip',Version:'old',RequestId:'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa'}).error,'catalog_changed');assert.equal(request('Purchase',{Sku:'bad',Version:'lustvend-1',RequestId:'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa'}).error,'unknown_product');assert.equal(request('Purchase',{RequestId:'bad'}).error,'invalid_request_id');});
test('write failure preserves pretransaction state',()=>{fund();const before=JSON.stringify([...records]);failWrite=true;assert.throws(()=>purchase());assert.equal(JSON.stringify([...records]),before);});
test('conflict recomputes funds before debit',()=>{fund();conflict=()=>{stateRow().value.balance=30;stateRow().version='2';};assert.equal(purchase().error,'insufficient_gems');assert.equal(state().balance,30);});
test('collect isolated by account and idempotent',()=>{fund();const r=purchase();assert.equal(request('Collect',{Receipt:r.receipt},'attacker').error,'receipt_not_found');assert.equal(request('Collect',{Receipt:r.receipt}).collected,true);assert.equal(request('Collect',{Receipt:r.receipt}).collected,true);assert.equal(state().pending.length,0);assert.equal(state().balance,240);assert.deepEqual(state().owned,['vid-chip']);});
test('PlayFab bridge binds logged-in player and allowlists arguments',()=>{
    const captured=[];
    const cloud=vm.createContext({handlers:{},currentPlayerId:'verified-account',server:{GetTitleInternalData(){return {Data:{SugarLustNakamaGatewayUrl:'https://example.invalid',SugarLustNakamaHttpKey:'test-only'}};}},http:{request(url,method,body){captured.push(JSON.parse(body));return JSON.stringify({balance:10});}}});
    vm.runInContext(fs.readFileSync(__dirname+'/../PlayFabCloudScript/cloudscript.js','utf8'),cloud);
    cloud.handlers.LustVendPurchase({PlayFabId:'attacker',price:0,amount:999,requestId:'test',sku:'vid-chip',version:'lustvend-1'});
    assert.equal(captured[0].PlayFabId,'verified-account');assert.equal(captured[0].price,undefined);assert.equal(captured[0].amount,undefined);
    cloud.handlers.LustVendState();assert.equal(captured[1].Action,'Daily');
});
function spin(id,game='sugar'){
    return JSON.parse(runtime.rpcProgressive({},null,nk,JSON.stringify({Operation:game==='neon'?'NeonSpin':'Spin',PlayFabId:'player',SpinId:id,BetPerLine:1,ActiveLines:1,Bet:100})));
}
test('paid spin transaction awards three milestones and daily set',()=>{
    request('Daily');for(let i=0;i<50;i++)spin('mission-spin-'+i,i%2?'neon':'sugar');
    assert.equal(state().balance,30);assert.equal(state().earning.spins,50);assert.equal(state().earning.setComplete,true);
    for(let i=0;i<5;i++)spin('extra-spin-'+i);assert.equal(state().balance,30);
});
test('replaying paid spin does not advance mission',()=>{
    spin('replayed-spin');spin('replayed-spin');assert.equal(state().earning.spins,1);
});
test('old spin cannot count again after receipt window expires',()=>{
    spin('old-spin');for(let i=0;i<25;i++)spin('new-spin-'+i);
    clock='2026-10-27T12:00:00Z';spin('old-spin');assert.equal(state().earning.spins,0);
});
test('five distinct completed days award weekly bonus once',()=>{
    for(let day=26;day<=30;day++){clock=`2026-10-${day}T12:00:00Z`;request('Daily');for(let i=0;i<50;i++)spin('week-'+day+'-'+i);}
    assert.equal(state().balance,180);assert.equal(state().earning.weeklyDays,5);assert.equal(state().earning.weeklyPaid,true);
    for(let i=0;i<10;i++)spin('extra-week-'+i);assert.equal(state().balance,180);
    clock='2026-11-02T12:00:00Z';assert.equal(state().earning.weeklyDays,0);assert.equal(state().earning.weeklyPaid,false);
});
test('tutorial must be sequential and grants thirty once',()=>{
    assert.equal(request('Tutorial',{Step:3}).error,'tutorial_step_out_of_order');
    for(let step=1;step<=3;step++)request('Tutorial',{Step:step});
    assert.equal(state().balance,30);request('Tutorial',{Step:3});assert.equal(state().balance,30);
});
test('earning storage failure rolls back entire spin and grant',()=>{
    for(let i=0;i<9;i++)spin('rollback-'+i);const before=JSON.stringify([...records]);failWrite=true;
    assert.throws(()=>spin('tenth-spin'));assert.equal(JSON.stringify([...records]),before);
    failWrite=false;spin('tenth-spin');assert.equal(state().balance,5);assert.equal(state().earning.spins,10);
});
test('client cannot submit activity or arbitrary gem credits',()=>{
    assert.throws(()=>request('Activity',{amount:1000,spins:50}));assert.equal(state().balance,0);
});
console.log(`${passed} LUSTVEND Nakama/PlayFab tests passed.`);
