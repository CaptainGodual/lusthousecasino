const R=require('../../Backend/NakamaRuntime/mega-pot-rules.js');
let seed=Number(process.argv[3]||127);function rng(){seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;}
const paidTarget=Number(process.argv[2]||200000),s=R.create();let stake=0,paid=0,ret=0,steps=0,free=0,holds=0,hit=0,mega=0;let cycle=0,cycleStake=0,cycleSum=0,cycleSq=0,cycles=0,cycleHits=0,maxCycle=0,buckets=[0,0,0,0,0,0];function finishCycle(){if(!cycleStake)return;let v=cycle/cycleStake;cycleSum+=v;cycleSq+=v*v;cycles++;if(v>0)cycleHits++;maxCycle=Math.max(maxCycle,v);buckets[v===0?0:v<1?1:v<5?2:v<20?3:v<100?4:5]++;}let sum=0,sq=0,blockStake=0,blockReturn=0,nblocks=0;
if(process.argv[4])Object.assign(R.Config,JSON.parse(process.argv[4]));
while(paid<paidTarget||s.Phase==='Hold'||s.Free||s.Royal){const x=R.step(s,10,rng);steps++;if(x.Debit){finishCycle();cycle=0;cycleStake=x.Debit;paid++;stake+=x.Debit;blockStake+=x.Debit;}ret+=x.Win;cycle+=x.Win;blockReturn+=x.Win;if(x.Win)hit++;if(x.Kind==='Free')free++;if(x.Triggered)holds++;if(x.State.Jackpot==='Mega')mega++;
 if(paid>0&&paid%1000===0&&blockStake>=200000&&s.Phase!=='Hold'&&!s.Free&&!s.Royal){let v=blockReturn/blockStake;sum+=v;sq+=v*v;nblocks++;blockStake=0;blockReturn=0;}
 if(steps>paidTarget*10)throw Error('Feature chain exceeded simulation bound');}
finishCycle();let se=nblocks>1?Math.sqrt(Math.max(0,(sq-sum*sum/nblocks)/(nblocks-1))/nblocks):null;
console.log(JSON.stringify({paid,steps,stake,returned:ret,rtp:ret/stake,free,holds,mega,hitRate:hit/steps,blocks:nblocks,approx95CI:se?[ret/stake-1.96*se,ret/stake+1.96*se]:null,volatility:{paidCycles:cycles,paidCycleHitRate:cycleHits/cycles,cycleReturnSD:Math.sqrt(cycleSq/cycles-Math.pow(cycleSum/cycles,2)),maxObservedReturnInTotalBets:maxCycle,buckets},config:R.Config}));
