const {finishSetup}=require('../../Tools/Backend/finish-settlement-setup');
const fs=require('fs'),os=require('os'),path=require('path'),assert=require('assert/strict');
let count=0;
async function run(name,fn){await fn();++count;console.log('PASS '+name);}
function fixture(opt={}){
 const root=fs.mkdtempSync(path.join(os.tmpdir(),'settlement-finish-'));const dir=path.join(root,'Saved/SettlementMigration');fs.mkdirSync(dir,{recursive:true});
 const source=JSON.stringify({Format:'slot-economy-migration-v1',Profiles:[{PlayFabId:'audit',Profile:{Coins:18305280,XP:10593,FreeSpins:{NeonNights:{Remaining:4,BetPerLine:100,ActiveLines:5,TotalWin:13000}}}}]});
 fs.writeFileSync(path.join(dir,'profiles.json'),source);
 const profile={Coins:18305280,XP:10593,FreeSpins:{NeonNights:{Remaining:4,BetPerLine:100,ActiveLines:5,TotalWin:13000}}};
 let flag=opt.enabled?'transactional-v1':'',sets=0;const calls=[],logs=[];
 async function request(url,body,headers){calls.push({url,body,headers});
  if(url.includes('/Server/GetTitleInternalData'))return {code:200,data:{Data:{SugarLustNakamaGatewayUrl:'https://example.invalid:7350',SugarLustNakamaHttpKey:'private-nakama-key',SugarLustEconomyMigrationReady:flag}}};
  if(url.includes('/v2/rpc/')){assert.equal(body.Operation,'Economy');assert.equal(body.MigrationReady,true);assert.equal(body.HasLegacyProfile,true);assert.equal(body.SeedProfile,undefined);
   if(opt.missingStage)throw Error('Existing player requires a frozen legacy import');
   return {SettlementVersion:1,Result:{...profile,Coins:opt.mismatch?4:profile.Coins}};}
  if(url.includes('/Server/ExecuteCloudScript')){assert.equal(body.RevisionSelection,'Live');assert.equal(body.FunctionName,'GetOrMigratePlayerProfile');
   return {code:200,data:opt.liveError?{Error:{StackTrace:'Failure private-nakama-key https://example.invalid/private'}}:{Revision:28,FunctionResult:profile}};}
  if(url.includes('/Admin/SetTitleInternalData')){assert.deepEqual(body,{Key:'SugarLustEconomyMigrationReady',Value:'transactional-v1'});++sets;flag=body.Value;return {code:200,data:{}};}
  throw Error('Unexpected endpoint');
 }
 return {root,calls,logs,get sets(){return sets;},start:()=>finishSetup({root,title:'16CECB',key:'private-playfab-key',request,log:s=>logs.push(s)}),cleanup(){assert.equal(fs.readFileSync(path.join(dir,'profiles.json'),'utf8'),source);fs.rmSync(root,{recursive:true,force:true});}};
}
(async()=>{
 await run('one key retrieves gateway, verifies staged account and Live handler, then enables only the gate',async()=>{const f=fixture();try{const r=await f.start();assert.equal(r.Accounts[0].Coins,18305280);assert.equal(f.sets,1);assert(f.calls.findIndex(c=>c.url.includes('/ExecuteCloudScript'))<f.calls.findIndex(c=>c.url.includes('/SetTitleInternalData')));assert(!JSON.stringify(f.logs).includes('private-'));assert(!fs.readFileSync(path.join(f.root,'Saved/SettlementFixVerification/live-setup-result.json'),'utf8').includes('private-'));}finally{f.cleanup();}});
 await run('rerun with enabled flag does not overwrite settings or restage',async()=>{const f=fixture({enabled:true});try{await f.start();assert.equal(f.sets,0);assert(!f.calls.some(c=>c.body.Operation==='StageMigration'));}finally{f.cleanup();}});
 await run('balance mismatch blocks enabling and never overwrites wallet',async()=>{const f=fixture({mismatch:true});try{await assert.rejects(f.start(),/NOT reset or overwritten/);assert.equal(f.sets,0);assert(!f.calls.some(c=>c.url.includes('ExecuteCloudScript')));}finally{f.cleanup();}});
 await run('unstaged account stops without a default seed or flag write',async()=>{const f=fixture({missingStage:true});try{await assert.rejects(f.start(),/frozen legacy import/);assert.equal(f.sets,0);}finally{f.cleanup();}});
 await run('Live handler failure blocks enabling and redacts credentials and gateway URLs',async()=>{const f=fixture({liveError:true});try{await assert.rejects(f.start(),e=>!e.message.includes('private-nakama-key')&&!e.message.includes('example.invalid')&&e.message.includes('Live CloudScript'));assert.equal(f.sets,0);}finally{f.cleanup();}});
 await run('missing key fails before any requests',async()=>{await assert.rejects(finishSetup({root:os.tmpdir(),title:'16CECB',key:'',request:()=>{throw Error('must not contact server');}}),/title key/);});
 console.log(count+' setup workflow tests passed. No live requests made.');
})().catch(e=>{console.error(e);process.exitCode=1;});
