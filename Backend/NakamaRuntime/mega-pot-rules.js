// Mega Lust Pot rules v1. Pure rules shared by the server and offline simulation.
// No client-supplied outcomes, prizes, counters or balances are accepted.
var MegaPotRules=(function(){
 var C={Version:1,CrimsonGoal:12,RoyalGoal:16,MegaGoal:24,FreeAward:8,RoyalAward:3,
  HeartProbability:.02664,CrimsonProbability:.025,RoyalProbability:.008,WildProbability:.035,FreeWildProbability:.095,
  RespinCoinProbability:.035,CollectorProbability:.01,Mini:25,Major:100,Mega:1000};
 var lines=[[1,1,1,1,1],[0,0,0,0,0],[2,2,2,2,2],[0,1,2,1,0],[2,1,0,1,2],[0,0,1,0,0],[2,2,1,2,2],[1,0,0,0,1],[1,2,2,2,1],[0,1,1,1,0],[2,1,1,1,2],[1,0,1,0,1],[1,2,1,2,1],[0,2,0,2,0],[2,0,2,0,2],[0,1,0,1,0],[2,1,2,1,2],[0,2,2,2,0],[2,0,0,0,2],[1,1,0,1,1]];
 var pays=[[10,30,100],[8,25,80],[7,20,60],[6,18,50],[5,15,40],[4,12,30],[4,10,25],[3,8,20],[2,5,15],[2,5,15],[2,5,15],[1,4,10],[1,4,10],[15,50,150]];
 function zeros(){return [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];}
 function create(){return {Version:1,Revision:0,Bet:10,Phase:'Base',Pots:{},Free:0,Royal:0,Respins:0,Collector:1,Locked:zeros(),Grid:zeros(),Win:0,Jackpot:'',LastDebit:0};}
 function integer(v,min,max){return typeof v==='number'&&isFinite(v)&&Math.floor(v)===v&&v>=min&&v<=max;}
 function validate(s){
  if(!s||s.Version!==1||!integer(s.Revision,0,9007199254740990)||!integer(s.Bet,1,100)||['Base','Hold','Result'].indexOf(s.Phase)<0||!integer(s.Free,0,100)||!integer(s.Royal,0,100)||!integer(s.Respins,0,3)||!integer(s.Collector,1,10)||!Array.isArray(s.Grid)||s.Grid.length!==15||!Array.isArray(s.Locked)||s.Locked.length!==15||!s.Pots)throw Error('Invalid Mega Pot state; reconciliation required.');
  for(var i=0;i<15;i++)if(!integer(s.Grid[i],0,16)||!integer(s.Locked[i],0,s.Bet*200))throw Error('Invalid Mega Pot board.');
  for(var k in s.Pots)if(Object.prototype.hasOwnProperty.call(s.Pots,k)){var p=s.Pots[k];if(!/^(?:[1-9]|[1-9][0-9]|100)$/.test(k)||!p||!integer(p[0],0,C.CrimsonGoal-1)||!integer(p[1],0,C.RoyalGoal-1)||!integer(p[2],0,C.MegaGoal-1))throw Error('Invalid Mega Pot progress.');}
  if(s.Phase==='Hold'&&(count(s)<6||s.Respins<1||count(s)===15))throw Error('Invalid Mega Pot hold state.');
  return s;
 }
 function count(s){var n=0;for(var i=0;i<15;i++)if(s.Locked[i]>0)n++;return n;}
 function total(s){var n=0;for(var i=0;i<15;i++)n+=s.Locked[i];return n;}
 function evaluate(g,bet){var win=0,won=[];for(var l=0;l<20;l++){
  var wilds=0,target=-1,n=0;for(var r=0;r<5;r++){var v=g[lines[l][r]*5+r];if(v===13){if(target<0)wilds++;n++;}else if(v<13&&(target<0||target===v)){target=v;n++;}else break;}
  var a=target>=0&&n>=3?pays[target][n-3]:0,b=wilds>=3?pays[13][wilds-3]:0;
  if(a||b){win+=Math.max(a,b)*bet;won.push({Line:l,Length:b>a?wilds:n});}
 }return {Win:win,Lines:won};}
 function roll(rng,free){var x=rng(),w=free?C.FreeWildProbability:C.WildProbability;if(x<w)return 13;x-=w;if(x<C.HeartProbability)return 16;x-=C.HeartProbability;if(x<C.CrimsonProbability)return 14;x-=C.CrimsonProbability;if(x<C.RoyalProbability)return 15;return Math.floor(rng()*13);}
 function finish(s){var n=count(s),extra=n===15?C.Mega:n>=12?C.Major:n>=9?C.Mini:0;s.Win=total(s)*s.Collector+extra*s.Bet*20;s.Jackpot=n===15?'Mega':n>=12?'Major':n>=9?'Mini':'';s.Phase='Result';return s.Win;}
 function step(s,requestedBet,rng){
  validate(s);if(!integer(requestedBet,1,100))throw Error('Invalid Mega Pot bet.');
  var win=0,debit=0,kind='Paid',won=[],trigger=false,free=s.Free>0;
  if(s.Phase==='Result')s.Phase='Base';
  s.Win=0;s.Jackpot='';
  if(s.Phase==='Hold'){
   kind='Respin';var fresh=0;for(var i=0;i<15;i++)if(!s.Locked[i]&&rng()<C.RespinCoinProbability){s.Locked[i]=s.Bet*20*(1+Math.floor(rng()*10));fresh++;}
   s.Respins=fresh?3:s.Respins-1;if(rng()<C.CollectorProbability)s.Collector=Math.min(10,s.Collector+1);
   if(s.Respins===0||count(s)===15)win=finish(s);
  }else{
   if(!s.Free&&!s.Royal)s.Bet=requestedBet;
   if(free){kind='Free';s.Free--;}else debit=s.Bet*20;
   var multiplier=s.Royal>0?2+Math.floor(rng()*9):1;if(s.Royal)s.Royal--;
   var hearts=0,p=s.Pots[s.Bet]||(s.Pots[s.Bet]=[0,0,0]);
   for(var j=0;j<15;j++){var symbol=roll(rng,free);s.Grid[j]=symbol;if(symbol===14)p[0]++;if(symbol===15)p[1]++;if(symbol===16){p[2]++;hearts++;}}
   var pay=evaluate(s.Grid,s.Bet);win=pay.Win*multiplier;won=pay.Lines;
   while(p[0]>=C.CrimsonGoal){p[0]-=C.CrimsonGoal;s.Free=Math.min(100,s.Free+C.FreeAward);}
   while(p[1]>=C.RoyalGoal){p[1]-=C.RoyalGoal;s.Royal=Math.min(100,s.Royal+C.RoyalAward);}
   if(hearts>=6||p[2]>=C.MegaGoal){p[2]=0;trigger=true;s.Phase='Hold';s.Locked=zeros();s.Collector=1;s.Respins=3;
    for(var q=0;q<15;q++)if(s.Grid[q]===16)s.Locked[q]=s.Bet*20*(1+Math.floor(rng()*5));
    // Bounded selection: fill from a shuffled list, including under deterministic test RNGs.
    var empty=[];for(var e=0;e<15;e++)if(!s.Locked[e])empty.push(e);
    while(count(s)<6){var z=Math.floor(rng()*empty.length),cell=empty.splice(z,1)[0];s.Locked[cell]=s.Bet*20*(1+Math.floor(rng()*5));}
    if(count(s)===15)win+=finish(s);
   }
  }
  s.Win=win;s.LastDebit=debit;s.Revision++;
  return {Version:C.Version,Kind:kind,Debit:debit,Win:win,Triggered:trigger,Lines:won,State:s};
 }
 return {Config:C,Paylines:lines,Paytable:pays,create:create,validate:validate,evaluate:evaluate,step:step,count:count,total:total};
})();
if(typeof module!=='undefined')module.exports=MegaPotRules;


