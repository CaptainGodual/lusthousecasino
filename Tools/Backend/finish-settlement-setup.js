// Finish the staged migration using one privately entered PlayFab title key.
// No spins, payments, legacy-profile overwrites, script uploads or account resets.
const fs=require('fs'),path=require('path');
const {validate,safeBackendError}=require('./stage-economy-migration');
async function finishSetup({root,title,key,request,log=console.log}) {
 if(!/^[A-Za-z0-9]+$/.test(title)||!key)throw Error('The PlayFab title key is required.');
 const file=path.join(root,'Saved/SettlementMigration/profiles.json');
 if(!fs.existsSync(file))throw Error('The saved export is missing. Stop here; do not reset the account.');
 const backup=JSON.parse(fs.readFileSync(file,'utf8'));
 if(backup.Format!=='slot-economy-migration-v1')throw Error('Unrecognized backup format; no changes made.');
 const records=validate(backup.Profiles), sensitive=[key];
 async function network(url,body,headers={}) {
  let response;
  try{response=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json',...headers},body:JSON.stringify(body),signal:AbortSignal.timeout(20000)});}catch{throw Error('Connection failed. Your backup is unchanged. Rerunning this launcher is safe.');}
  const raw=await response.text();
  if(!response.ok)throw Error('HTTP '+response.status+': '+safeBackendError(raw,sensitive));
  try{return JSON.parse(raw);}catch{throw Error('The server returned unreadable data; the backup is unchanged.');}
 }
 const send=request||network;
 async function playfab(api,body) {
  const response=await send('https://'+title+'.playfabapi.com/'+api,body,{'X-SecretKey':key});
  if(response.code!==200||!response.data)throw Error('PlayFab: '+safeBackendError(JSON.stringify(response),sensitive));
  return response.data;
 }
 log('1/4 Reading your saved server settings automatically...');
 const names=['SugarLustNakamaGatewayUrl','SugarLustNakamaHttpKey','SugarLustEconomyMigrationReady'];
 const settings=await playfab('Server/GetTitleInternalData',{Keys:names});
 function value(name){const v=(settings.Data||{})[name];return typeof v==='string'?v:v&&typeof v.Value==='string'?v.Value:'';}
 const address=value(names[0]),httpKey=value(names[1]);
 if(!address||!httpKey)throw Error('Your PlayFab Title Internal Data is missing the existing Nakama gateway or HTTP key. Nothing has been enabled.');
 sensitive.push(httpKey);
 let base;try{base=new URL(address.trim());}catch{throw Error('The saved Nakama gateway address is invalid. Nothing enabled.');}
 if(base.protocol!=='https:'||base.username||base.password||base.search||base.hash)throw Error('The saved gateway must be a plain HTTPS base URL. Nothing enabled.');
 const gateway=base.toString().replace(/\/+$/,'')+'/v2/rpc/sugarlust_progressive?http_key='+encodeURIComponent(httpKey)+'&unwrap';
 function verify(profile,record,where) {
  if(!profile||profile.Coins!==record.Profile.Coins||profile.XP!==record.Profile.XP)
   throw Error(where+': server balance/XP differs from the frozen backup. It was NOT reset or overwritten. Keep gameplay stopped for reconciliation.');
  const saved=record.Profile.FreeSpins||{};
  for(const game of Object.keys(saved))for(const field of ['Remaining','BetPerLine','ActiveLines','TotalWin','TotalAwarded'])
   if(saved[game][field]!==undefined&&(!profile.FreeSpins||!profile.FreeSpins[game]||profile.FreeSpins[game][field]!==saved[game][field]))
    throw Error(where+': saved free-spin state differs. No reset was attempted.');
  if(record.Profile.BonusState){const old=record.Profile.BonusState,current=profile.PendingBonus;
   if(!current||current.PicksRemaining!==old.PicksRemaining||current.Total!==old.Total)throw Error(where+': Candy Jar state differs. Stop before playing.');}
  const heat=record.Profile.NeonHeatBonus;
  if(heat&&heat.Active&&(!profile.NeonHeatBonus||profile.NeonHeatBonus.Total!==heat.Total||profile.NeonHeatBonus.RespinsRemaining!==heat.RespinsRemaining))
   throw Error(where+': Neon Heat state differs. Stop before playing.');
 }
 log('2/4 Verifying/importing ONLY your already-staged profiles...');
 for(const record of records){
  const response=await send(gateway,{Operation:'Economy',SettlementVersion:1,Action:'GetOrMigratePlayerProfile',PlayFabId:record.PlayFabId,Args:{},HasLegacyProfile:true,MigrationReady:true});
  if(response.SettlementVersion!==1||!response.Result)throw Error('The transactional Nakama runtime did not answer correctly. Nothing has been enabled; preserve your backup.');
  verify(response.Result,record,'Nakama');
 }
 log('3/4 Checking the actual LIVE PlayFab profile handler...');
 const verified=[];
 for(const record of records){
  const result=await playfab('Server/ExecuteCloudScript',{PlayFabId:record.PlayFabId,FunctionName:'GetOrMigratePlayerProfile',FunctionParameter:{},RevisionSelection:'Live',GeneratePlayStreamEvent:true});
  if(result.Error)throw Error('Live CloudScript: '+safeBackendError(JSON.stringify({message:result.Error.StackTrace||result.Error.Message}),sensitive));
  verify(result.FunctionResult,record,'Live PlayFab');
  verified.push({Player:record.PlayFabId,Coins:result.FunctionResult.Coins,XP:result.FunctionResult.XP,Revision:result.Revision});
 }
 log('4/4 Enabling the migration setting and checking it was saved...');
 if(value(names[2])!=='transactional-v1')await playfab('Admin/SetTitleInternalData',{Key:names[2],Value:'transactional-v1'});
 const check=await playfab('Server/GetTitleInternalData',{Keys:[names[2]]});
 const flag=(check.Data||{})[names[2]];
 if((typeof flag==='string'?flag:flag&&flag.Value)!=='transactional-v1')throw Error('The enable setting has not appeared on readback yet. Profiles verified; wait briefly and rerun this launcher. Do not reset anything.');
 const report={Status:'Live profile verification passed; migration flag enabled',VerifiedAt:new Date().toISOString(),Accounts:verified};
 const dir=path.join(root,'Saved/SettlementFixVerification');fs.mkdirSync(dir,{recursive:true});
 fs.writeFileSync(path.join(dir,'live-setup-result.json'),JSON.stringify(report,null,2));
 for(const r of verified)log('VERIFIED LIVE: '+r.Coins+' coins, '+r.XP+' XP (CloudScript revision '+r.Revision+').');
 log('SETUP COMPLETE. Restart Play in Unreal and confirm profile loading succeeds. No test spins were performed.');
 return report;
}
if(require.main===module)finishSetup({root:path.resolve(__dirname,'../..'),title:process.env.PLAYFAB_TITLE_ID||'16CECB',key:process.env.PLAYFAB_SECRET_KEY}).catch(e=>{console.error(e.message);process.exitCode=1;});
module.exports={finishSetup};
