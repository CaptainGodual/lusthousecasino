// Node 18+; no dependencies. Private operator tool, never included in game builds.
// Preview/freeze: --players players.json --export Saved/SettlementMigration/profiles.json
// Stage the frozen copy: --snapshot Saved/SettlementMigration/profiles.json --apply
const fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'../..');
function options(argv){const result={apply:false};for(let i=0;i<argv.length;++i){
 const arg=argv[i];if(arg==='--apply')result.apply=true;
 else if(['--players','--snapshot','--export'].includes(arg)){if(!argv[i+1]||argv[i+1].startsWith('--'))throw Error('Missing value for '+arg);result[arg.slice(2)]=argv[++i];}
 else throw Error('Unknown option '+arg);
}if(!!result.players===!!result.snapshot)throw Error('Use exactly one of --players or --snapshot.');return result;}
function validate(records){if(!Array.isArray(records)||!records.length)throw Error('No legacy profiles supplied.');const seen=new Set();
 for(const r of records){if(!r||!/^[A-Za-z0-9_-]{1,64}$/.test(r.PlayFabId)||seen.has(r.PlayFabId))throw Error('Invalid or duplicate player identifier.');seen.add(r.PlayFabId);
  if(!r.Profile||!Number.isSafeInteger(r.Profile.Coins)||r.Profile.Coins<0||!Number.isSafeInteger(r.Profile.XP)||r.Profile.XP<0)throw Error('Invalid legacy profile; no changes made.');
 }return records;}
function secret(name){const value=process.env[name];if(!value)throw Error('Set '+name+' in this private shell session.');return value;}
function safeBackendError(raw,secrets=[]){
 let message='No structured error message returned; inspect Nakama service logs.';
 try{const value=JSON.parse(raw);const detail=value.message || value.errorMessage || (value.error && value.error.message) || (typeof value.error==='string' ? value.error : null);if(typeof detail==='string')message=detail;}catch{}
 for(const secret of secrets)if(secret)message=message.split(secret).join('[REDACTED]');
 message=message.replace(/https?:\/\/[^\s"<>]+/gi,'[URL REDACTED]').replace(/[\r\n]+/g,' ');
 return message.slice(0,1600);
}
async function post(url,body,headers={}){let response;try{response=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json',...headers},body:JSON.stringify(body),signal:AbortSignal.timeout(20000)});}catch{throw Error('Backend request failed or timed out. Credentials and URLs are omitted.');}
 if(!response.ok){
  let raw='';try{raw=await response.text();}catch{}
  throw Error('Backend returned HTTP '+response.status+': '+safeBackendError(raw,[process.env.PLAYFAB_SECRET_KEY,process.env.NAKAMA_HTTP_KEY]));
 }
 try{return await response.json();}catch{throw Error('Backend returned invalid JSON.');}}
async function main(){const opt=options(process.argv.slice(2));let records;
 if(opt.snapshot){records=validate(JSON.parse(fs.readFileSync(path.resolve(opt.snapshot),'utf8')).Profiles);}
 else {
  if(opt.apply)throw Error('Freeze with --export first, then stage that exact --snapshot with --apply.');
  if(!opt.export)throw Error('--players requires --export to preserve the frozen source snapshot.');
  const ids=JSON.parse(fs.readFileSync(path.resolve(opt.players),'utf8'));
  if(!Array.isArray(ids)||!ids.length||ids.some(id=>typeof id!=='string'||!/^[A-Za-z0-9_-]{1,64}$/.test(id)))throw Error('players.json must contain a nonempty array of PlayFab IDs.');
  const title=secret('PLAYFAB_TITLE_ID');if(!/^[A-Za-z0-9]+$/.test(title))throw Error('Invalid title ID.');const key=secret('PLAYFAB_SECRET_KEY');records=[];
  for(const id of [...new Set(ids)]){
   const response=await post('https://'+title+'.playfabapi.com/Server/GetUserInternalData',{PlayFabId:id,Keys:['SugarLustProfile']},{'X-SecretKey':key});
   if(response.code!==200||!response.data)throw Error('PlayFab rejected the profile read.');
   const item=response.data.Data&&response.data.Data.SugarLustProfile;
   if(!item||!item.Value){console.log(id+': no legacy record; new-player defaults will apply.');continue;}
   let profile;try{profile=JSON.parse(item.Value);}catch{throw Error('Malformed legacy profile; stop migration.');}
   records.push({PlayFabId:id,Profile:profile,LegacyDataVersion:response.data.DataVersion});
  }
  validate(records);const file=path.resolve(opt.export);fs.mkdirSync(path.dirname(file),{recursive:true});
  // Never replace a previously frozen source accidentally.
  fs.writeFileSync(file,JSON.stringify({Format:'slot-economy-migration-v1',ExportedAt:new Date().toISOString(),Profiles:records},null,2),{flag:'wx'});
  console.log('Frozen private snapshot saved: '+file);
 }
 for(const r of records)console.log(r.PlayFabId+': '+r.Profile.Coins+' coins, '+r.Profile.XP+' XP');
 if(!opt.apply){console.log('PREVIEW ONLY. No Nakama writes. Keep this snapshot private.');return;}
 const endpoint=new URL(secret('NAKAMA_URL'));if(endpoint.protocol!=='https:'||endpoint.username||endpoint.password||endpoint.search||endpoint.hash)throw Error('NAKAMA_URL must be a plain HTTPS gateway base URL.');
 const key=secret('NAKAMA_HTTP_KEY');const url=endpoint.toString().replace(/\/+$/,'')+'/v2/rpc/sugarlust_progressive?http_key='+encodeURIComponent(key)+'&unwrap';
 for(const r of records){const response=await post(url,{Operation:'StageMigration',SettlementVersion:1,PlayFabId:r.PlayFabId,Profile:r.Profile});
  if(response.SettlementVersion!==1||!['staged','already_staged','already_migrated'].includes(response.Status))throw Error('Migration runtime contract rejected. Stop; no automatic fallback.');
  console.log(r.PlayFabId+': '+response.Status);
 }
 console.log('Frozen records staged. The import gate is NOT enabled by this tool.');
}
if(require.main===module)main().catch(error=>{console.error(error.message);process.exitCode=1;});
module.exports={options,validate,safeBackendError};
