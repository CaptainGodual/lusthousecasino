'use strict';
const fs = require('node:fs');
const path = require('node:path');
const {inspectAutomation} = require('./release-policy');
function expectedTests(root) {
  const directory = path.join(root, 'Source/SugarLustUE/Tests');
  const names = fs.readdirSync(directory).filter(name=>name.endsWith('.cpp')).flatMap(name =>
    [...fs.readFileSync(path.join(directory,name),'utf8').matchAll(/IMPLEMENT_SIMPLE_AUTOMATION_TEST\(\s*\w+\s*,\s*"([^"]+)"/g)].map(match=>match[1]));
  if (!names.length || new Set(names).size!==names.length) throw Error('A nonempty, unique project automation inventory is required.');
  if (names.some(name=>!name.startsWith('SugarLust.'))) throw Error('A project test falls outside the SugarLust automation filter.');
  return names.sort();
}
function inRange(version, range) {
  const compare=(a,b)=>{const left=a.split('.').map(Number),right=b.split('.').map(Number);
    for(let i=0;i<Math.max(left.length,right.length);i++){const delta=(left[i]||0)-(right[i]||0);if(delta)return Math.sign(delta);}return 0;};
  const [minimum,maximum]=range.split('-');
  return compare(version,minimum)>=0 && (!maximum || compare(version,maximum)<=0);
}
function inspectBuild(log, exitCode, sdk) {
  if (exitCode!==0) return {status:/access.*denied|unauthorizedaccess|permission|EACCES|EPERM/i.test(log)?'blocked':'failed',summary:'Native compilation did not complete successfully.'};
  if (!/Result: Succeeded|Target is up to date/.test(log)) return {status:'blocked',summary:'No explicit successful build result was produced.'};
  const compiler=/Using Visual Studio ([\d.]+) toolchain \((.+?)\) and Windows ([\d.]+) SDK/.exec(log);
  const selectedVersion=compiler?.[1]||/Visual Studio compiler version ([\d.]+) is newer than/.exec(log)?.[1];
  const warnings=[...new Set(log.split(/\r?\n/).filter(line=>/\bwarning\b|newer than.*preferred|result store tasks did not succeed/i.test(line)))];
  let toolchain=null;
  if(selectedVersion){
    if(!sdk||!Array.isArray(sdk.PreferredVisualCppVersions)||!Array.isArray(sdk.BannedVisualCppVersions)||!sdk.MinimumVisualCppVersion)
      return {status:'blocked',summary:'Engine toolchain policy could not be verified.'};
    toolchain={compilerVersion:selectedVersion,compilerPath:compiler?.[2]||null,windowsSdkVersion:compiler?.[3]||null,
      versionEvidence:compiler?'Compiler and SDK selection in the build log':'Selected compiler warning emitted by UnrealBuildTool for an up-to-date target',
      preferred:sdk.PreferredVisualCppVersions.some(range=>inRange(selectedVersion,range)),
      banned:sdk.BannedVisualCppVersions.some(range=>inRange(selectedVersion,range))||!inRange(selectedVersion,sdk.MinimumVisualCppVersion)};
    if(toolchain.banned)return {status:'failed',summary:'The selected compiler is rejected by the installed engine policy.',toolchain,warnings};
  }
  return {status:'passed',summary:'Native target build succeeded'+(toolchain&&!toolchain.preferred?'; selected compiler is allowed but not preferred.':'.'),
    toolchain,warnings,toolchainNote:toolchain?'Versions were extracted from this build log.':'An up-to-date target may omit toolchain selection; retain the preflight inventory and previous compile evidence.'};
}
function inspectRun({report,expected,exitCode,fresh}) {
  if(!fresh||!report)return {status:'blocked',summary:'Unreal did not produce a fresh automation report; an exit code of zero does not verify test execution.',expectedTests:expected};
  if(exitCode!==0)return {status:'failed',summary:'Unreal automation exited unsuccessfully.',expectedTests:expected};
  if(!Array.isArray(expected)||!expected.length) return {status:'failed',summary:'The required automation inventory is empty.'};
  const result=inspectAutomation(report,expected);
  const names=(report.tests||[]).map(test=>test.fullTestPath||test.testDisplayName);
  if(new Set(names).size!==names.length)return {status:'failed',summary:'The automation report contains duplicate test records.',expectedTests:expected};
  return {...result,expectedTests:expected};
}
module.exports={expectedTests,inRange,inspectBuild,inspectRun};
if(require.main===module){
  const args=process.argv.slice(2),value=key=>{const index=args.indexOf(key);return index<0?null:args[index+1];};
  try{
    const mode=value('--mode'),root=path.resolve(value('--root'));
    const rawExitCode=value('--exit-code');
    if(rawExitCode===null||!/^[-]?\d+$/.test(rawExitCode)) throw Error('A recorded native process exit code is required.');
    const nativeExitCode=Number(rawExitCode);
    let result;
    if(mode==='EditorTests'){
      const file=value('--report'),started=Date.parse(value('--started'));
      const fresh=fs.existsSync(file)&&Number.isFinite(started)&&fs.statSync(file).mtimeMs>=started;
      let report=null;
      if(fresh){try{report=JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));}catch{throw Error('Automation report is malformed JSON.');}}
      result=inspectRun({report,expected:expectedTests(root),exitCode:nativeExitCode,fresh});
    }else{
      const log=fs.readFileSync(value('--log'),'utf8').replace(/^\uFEFF/,'');
      const sdk=JSON.parse(fs.readFileSync(path.join(value('--engine'),'Engine/Config/Windows/Windows_SDK.json'),'utf8'));
      result=inspectBuild(log,nativeExitCode,sdk);
    }
    process.stdout.write(JSON.stringify(result));
    process.exitCode=result.status==='passed'?0:result.status==='failed'?1:2;
  }catch(error){process.stdout.write(JSON.stringify({status:'failed',summary:error.message}));process.exitCode=1;}
}
