// Prepare a reviewable private gateway request. Never connects to a service.
const fs=require('fs'),path=require('path'),vm=require('vm');
const root=path.resolve(__dirname,'../..');
const args=process.argv.slice(2),options={};
const flags=new Set(['--earning-enabled','--claims-enabled','--include-neon']);
const values=new Set(['--starts-at','--event-id','--expected-version','--previous-config','--output']);
for(let i=0;i<args.length;i++){
    if(flags.has(args[i]))options[args[i]]=true;
    else if(values.has(args[i]) && args[i+1] && !args[i+1].startsWith('--'))options[args[i]]=args[++i];
    else throw Error('Unknown or incomplete Hot Streak argument: '+args[i]);
}
const start=Date.parse(options['--starts-at']);
if(!Number.isFinite(start) || !/Z$/.test(options['--starts-at']||''))throw Error('Supply --starts-at as an explicit UTC timestamp ending in Z.');
const fixture=JSON.parse(fs.readFileSync(path.join(root,'Backend/Tests/fixtures/hot-streak-internal.json'),'utf8'));
const definition=fixture.Events[0];
definition.EventId=options['--event-id']||definition.EventId;
definition.StartsAt=new Date(start).toISOString();definition.EndsAt=new Date(start+7*86400000).toISOString();definition.ClaimUntil=new Date(start+9*86400000).toISOString();
definition.EarningEnabled=!!options['--earning-enabled'];definition.ClaimsEnabled=!!options['--claims-enabled'];
definition.EligibleGames=options['--include-neon']?['SugarLust','NeonNights']:['SugarLust'];
const config=options['--previous-config']?JSON.parse(fs.readFileSync(path.resolve(options['--previous-config']),'utf8')):{SchemaVersion:1,ActiveEventId:definition.EventId,Events:[]};
if(config.Events.some(event=>event.EventId===definition.EventId))throw Error('Use a new EventId; previously prepared seasons must be retained unchanged.');
config.Events.push(definition);config.ActiveEventId=definition.EventId;
const context=vm.createContext({});vm.runInContext(fs.readFileSync(path.join(root,'Backend/NakamaRuntime/hot-streak-rules.js'),'utf8'),context);
context.HotStreakRules.validateConfig(config);
const output=path.resolve(options['--output']||path.join(root,'Saved/HotStreakVerification/configuration-request.json'));
fs.mkdirSync(path.dirname(output),{recursive:true});
fs.writeFileSync(output,JSON.stringify({Operation:'ConfigureHotStreak',ExpectedVersion:options['--expected-version']||'*',Configuration:config},null,2)+'\n');
console.log('Prepared private Hot Streak request: '+output);
console.log('Earning '+(definition.EarningEnabled?'enabled':'disabled')+'; claims '+(definition.ClaimsEnabled?'enabled':'disabled')+'. No service was changed.');
