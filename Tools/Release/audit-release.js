'use strict';
// Local verification only. Does not deploy, publish, create accounts, or submit live wagers.
const fs = require('node:fs'), path = require('node:path'), crypto = require('node:crypto');
const {spawnSync} = require('node:child_process');
const {validateCatalog, fingerprint, inspectAutomation, evaluate} = require('./release-policy');
const root = path.resolve(__dirname, '../..');
const args = process.argv.slice(2);
const option = key => {const i=args.indexOf(key);return i < 0 ? null : args[i+1];};
const readJson = file => JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));
const relative = file => path.relative(root,file).replaceAll('\\','/');
const catalog = validateCatalog(readJson(path.join(root,'Docs/Release/systems.json')));
const sourceFingerprint = fingerprint(root), checkedAt = new Date().toISOString();
const runDir = path.join(root,'Saved/ReleaseAudit','run-'+checkedAt.replace(/[:.]/g,'-'));
fs.mkdirSync(runDir,{recursive:true});
const checks = [];
function add(id,status,summary,extra={}) { const result={id,status,summary,checkedAt,sourceFingerprint,...extra}; checks.push(result); return result; }
function command(id,executable,parameters,options={}) {
  const log=path.join(runDir,id+'.log'), fd=fs.openSync(log,'w');
  let result;
  try {result=spawnSync(executable,parameters,{cwd:options.cwd || root,stdio:['ignore',fd,fd],timeout:options.timeout || 120000,windowsHide:true});}
  finally {fs.closeSync(fd);}
  const text=fs.readFileSync(log,'utf8');
  const denied=/WinError 10013|Access is denied|EACCES|EPERM|socket in a way forbidden/i.test(text);
  const status=result.error ? 'blocked' : result.status === 0 ? 'passed' : denied ? 'blocked' : 'failed';
  const tail=text.trim().split(/\r?\n/).slice(-3).join(' ').slice(0,500);
  return add(id,status,result.error?.message || tail || 'No check output; exit '+result.status,{exitCode:result.status,evidence:[relative(log)],...options.metadata});
}

const descriptor=readJson(path.join(root,'SugarLustUE.uproject'));
const gameIni=fs.readFileSync(path.join(root,'Config/DefaultGame.ini'),'utf8');
const engineIni=fs.readFileSync(path.join(root,'Config/DefaultEngine.ini'),'utf8');
const versions=[descriptor.ProjectVersion,...gameIni.matchAll(/^(?:CurrentVersion|ProjectVersion)=(.+)$/gm),...engineIni.matchAll(/^VersionDisplayName=(.+)$/gm)].map(v=>Array.isArray(v)?v[1].trim():v);
add('project-config',descriptor.EngineAssociation==='5.8' && versions.every(v=>v===descriptor.ProjectVersion)?'passed':'failed',`Engine ${descriptor.EngineAssociation}; configured versions ${versions.join(', ')}`,{evidence:['SugarLustUE.uproject','Config/DefaultGame.ini','Config/DefaultEngine.ini']});
const git=spawnSync('git',['rev-parse','--verify','HEAD'],{cwd:root,encoding:'utf8',windowsHide:true});
const sha=git.stdout?.trim();
add('git-head',git.status===0 && /^[a-f0-9]{40,64}$/i.test(sha)?'passed':'blocked',git.status===0 ? 'Resolved source commit '+sha : 'Git cannot resolve this checkout; preserve the source manifest and restore valid revision metadata.',{commit:git.status===0?sha:null,evidence:['Saved/ReleaseAudit/2026-11-11/baseline-source-manifest.json']});

if (args.includes('--reuse-checks')) {
  const previousPath=path.join(root,'Saved/ReleaseAudit/current.json');
  if (fs.existsSync(previousPath)) {
    for(const c of readJson(previousPath).checks) if(!['project-config','git-head','editor-build','editor-automation','windows-package'].includes(c.id)) checks.push(c);
  }
} else {
  command('runtime-bundle',process.execPath,['Tools/Backend/build-runtime.js','--check']);
  for(const [id,file] of [['settlement','Backend/Tests/settlement.test.js'],['free-spins','Backend/Tests/free-spins.test.js'],['mega-pot','Backend/Tests/mega-pot.test.js'],['setup-workflow','Backend/Tests/finish-settlement-setup.test.js'],['lustvend','Backend/NakamaRuntime/test_lustvend.js'],['release-policy','Tools/Release/release-policy.test.js']]) command(id,process.execPath,[file]);
  command('lustvend-python',process.platform==='win32'?'python':'python3',['-m','unittest','-v','test_server'],{cwd:path.join(root,'Backend/LustVend'),metadata:{required:false}});
}

const evidenceDir=option('--unreal-evidence');
for(const [id,mode] of [['editor-build','EditorBuild'],['editor-automation','EditorTests'],['windows-package','WindowsCheckpoint']]) {
  const evidenceFile=evidenceDir?path.resolve(root,evidenceDir,mode+'-evidence.json'):null;
  if(evidenceFile && fs.existsSync(evidenceFile)) {
    const e=readJson(evidenceFile);
    let status=e.Status, summary=e.Summary;
    const refs=(e.Evidence||[]).map(p=>path.resolve(p));
    const checked=Date.parse(e.CheckedAt), started=Date.parse(e.StartedAt);
    if(e.SourceFingerprint!==sourceFingerprint) {status='blocked';summary='Unreal evidence belongs to different source.';}
    else if(!Number.isFinite(checked)||!Number.isFinite(started)||checked<started||checked>Date.now()+60000) {status='blocked';summary='Unreal evidence timestamps are invalid.';}
    else if(status==='passed' && (e.ExitCode!==0 || !refs.length || refs.some(p=>!fs.existsSync(p)||fs.statSync(p).size===0))) {status='blocked';summary='Unreal evidence is incomplete, missing, or has no successful exit.';}
    if(id==='editor-automation' && status==='passed') {
      const reportPath=refs.find(p=>path.basename(p)==='index.json');
      const expected=fs.readdirSync(path.join(root,'Source/SugarLustUE/Tests')).filter(n=>n.endsWith('.cpp')).flatMap(n=>[...fs.readFileSync(path.join(root,'Source/SugarLustUE/Tests',n),'utf8').matchAll(/IMPLEMENT_SIMPLE_AUTOMATION_TEST\(\s*\w+\s*,\s*"([^"]+)"/g)].map(m=>m[1]));
      const assessment=inspectAutomation(reportPath?readJson(reportPath):null,expected);
      status=assessment.status; summary=assessment.summary;
    }
    const existingRefs=refs.filter(p=>fs.existsSync(p)&&fs.statSync(p).isFile()&&fs.statSync(p).size>0);
    add(id,status,summary,{checkedAt:e.CheckedAt||checkedAt,sourceFingerprint:e.SourceFingerprint,evidence:[relative(evidenceFile),...existingRefs.map(relative)],artifactFingerprint:e.ArtifactFingerprint});
  } else add(id,'not_run','Run Verify-Project.ps1 -Mode '+mode+' and import its evidence.');
}

const manualPath=path.join(root,'Saved/ReleaseAudit/acceptance.json');
const manual=fs.existsSync(manualPath)?readJson(manualPath):null;
function evidenceExists(name) {
  if(typeof name!=='string'||path.isAbsolute(name)) return false;
  const resolved=path.resolve(root,name);
  const rel=path.relative(root,resolved);
  return !rel.startsWith('..')&&!path.isAbsolute(rel)&&fs.existsSync(resolved)&&fs.statSync(resolved).isFile()&&fs.statSync(resolved).size>0;
}
const readiness=evaluate(catalog,checks,manual,sourceFingerprint,{evidenceExists});
const checkpoint=evaluate(catalog,checks,manual,sourceFingerprint,{scope:'checkpoint',evidenceExists});
const report={schemaVersion:1,checkedAt,sourceFingerprint,project:relative(path.join(root,'SugarLustUE.uproject')),engineAssociation:descriptor.EngineAssociation,version:descriptor.ProjectVersion,checks,readiness,checkpoint};
fs.writeFileSync(path.join(runDir,'audit.json'),JSON.stringify(report,null,2)+'\n');
fs.writeFileSync(path.join(root,'Saved/ReleaseAudit/current.json'),JSON.stringify(report,null,2)+'\n');
const template={schemaVersion:1,sourceFingerprint,records:catalog.systems.flatMap(s=>s.signoffs.map(id=>({id,status:'pending',tester:'',checkedAt:'',artifactFingerprint:'',evidence:[],acceptance:s.acceptance})))};
fs.writeFileSync(path.join(root,'Saved/ReleaseAudit/acceptance.template.json'),JSON.stringify(template,null,2)+'\n');

const cell=s=>String(s).replaceAll('|','/').replace(/\r?\n/g,' ');
const link=p=>`[${p}](../../${p.replaceAll(' ','%20')})`;
const counts=Object.fromEntries(['working','incomplete','planned','blocked'].map(s=>[s,catalog.systems.filter(x=>x.status===s).length]));
const lines=[
  '# Lust House Casino master release board','',
  `Reviewed ${checkedAt}. This is the single game implementation board. Edit systems.json and regenerate this view; keep Character Studio and Character Production as linked specialist records.`,'',
  `**Full release: ${readiness.ready?'READY':'NOT READY'}. Focused checkpoint: ${checkpoint.ready?'READY':'NOT READY'}.**`,'',
  `Inventory: ${catalog.systems.length} systems; ${Object.entries(counts).map(([s,n])=>n+' '+s).join(', ')}. Working always includes an evidence level and does not automatically mean release accepted.`,'',
  '## Active baseline','',
  `- Project: ${catalog.project}; Unreal ${descriptor.EngineAssociation}; application ${descriptor.ProjectVersion}.`,
  '- Current runtime source fingerprint: '+sourceFingerprint+'.',
  `- Source-control identity: ${checks.find(c=>c.id==='git-head').summary}`,
  `- Preserved pre-change manifest: ${link('Saved/ReleaseAudit/2026-11-11/baseline-source-manifest.json')}.`,
  `- Machine-readable current checks: ${link('Saved/ReleaseAudit/current.json')}.`,'',
  '## Status meanings',''
];
for(const [status,meaning] of Object.entries(catalog.statusDefinitions)) lines.push(`- **${status}**: ${meaning}`);
lines.push('','## Current verification','', '| Check | Result | Evidence |','| --- | --- | --- |');
for(const c of checks) lines.push(`| ${c.id}${c.required===false?' (optional demo)':''} | ${c.status} | ${cell(c.summary)} ${(c.evidence||[]).map(link).join(' ')} |`);
lines.push('','An Editor process exiting with code zero without a nonempty test report remains unverified. Isolated backend tests use fake accounts and storage. No live wagers or deployment changes are performed by this audit.','', '## Master task queue','', '| ID | System | Status | Column | Priority | Scope | Next action |','| --- | --- | --- | --- | --- | --- | --- |');
for(const s of catalog.systems) lines.push(`| [${s.id}](#${s.id.toLowerCase()}) | ${cell(s.name)} | ${s.status} | ${s.column} | ${s.priority} | ${s.scope} | ${cell(s.nextAction)} |`);
lines.push('','## Implementation order','', '1. P0 baseline: REL-01, REL-02, REL-03, REL-35. Resolve source identity, retain the baseline, run actual automation, and produce a new package.', '2. P0 gameplay: REL-05, REL-34, REL-04, REL-06, REL-08, REL-09, REL-10, REL-11, REL-12, REL-13. Prove money, requests, persistence and the complete loop.', '3. P1 presentation: REL-16, REL-17, REL-20, REL-23, then REL-21. Connect one approved character before expanding the roster.', '4. P1 event slice: REL-14 and REL-22. Freeze the rules, deliver one supported Mild reward, and verify earning and claims in staging and the package.', '5. Checkpoint acceptance: REL-03, REL-24, REL-26, REL-27, REL-33. Fix observed defects, verify operations/content, and review the exact build.', '6. Full product completion: REL-07, REL-15, REL-18, REL-19, REL-25, REL-28, REL-29, REL-30, REL-31, REL-32. Each remains in this board until its real acceptance is working; none is silently removed from full release readiness.', '', 'Only one implementation task and one bounded character/art task should be In Progress. The focused checkpoint is an intermediate milestone; it does not convert planned expansion systems to working.', '', '## Completion checks by system','');
for(const s of catalog.systems) {
  const r=readiness.systems.find(r=>r.id===s.id);
  lines.push(`## ${s.id}`,'',`**${s.name}** — ${s.status}; ${s.priority}; ${s.column}.`,'',`Evidence level: ${s.verifiedScope}.`,'',`Source evidence: ${s.sources.map(link).join(', ')}.`,'',`Dependencies: ${s.dependsOn.join(', ')||'none'}.`,'',`Current constraint: ${s.blocker}`,'',`Next action: ${s.nextAction}`,'');
  for(const task of s.plan) lines.push('- [ ] '+task);
  lines.push('',`Acceptance: ${s.acceptance}`,'',`Required acceptance record: ${s.signoffs.join(', ')}. Release acceptance: ${r.ready?'verified':'pending'}.`,'');
}
lines.push('## Updating this board','', 'Run `node Tools/Release/audit-release.js` to execute local checks and regenerate this board. Exit 2 means release evidence is still blocked; exit 1 means a required automated check failed. Use `--unreal-evidence Saved/ReleaseAudit/<run>` to import native build, automation and checkpoint evidence. `--reuse-checks` reuses recorded checks without refreshing their timestamps; changed source or stale evidence still blocks readiness.', '', 'Use `Tools/Release/Verify-Project.ps1 -Mode EditorBuild`, `-Mode EditorTests`, and `-Mode WindowsCheckpoint` with the same output directory and the audit source fingerprint. Copy acceptance.template.json to acceptance.json only when recording real tests. Each passed acceptance needs a tester, UTC timestamp, matching package fingerprint and retained evidence. A template is never acceptance.', '', 'Historical deployment notes remain reference evidence. They are not parallel game task boards. The original November 11 advancement plan and November 9 Pepper design remain design inputs.', '');
fs.writeFileSync(path.join(root,'Docs/Release/MASTER_BOARD.md'),lines.join('\n'));
console.log(JSON.stringify({systems:catalog.systems.length,counts,ready:readiness.ready,checkpointReady:checkpoint.ready,sourceFingerprint,report:relative(path.join(runDir,'audit.json')),board:'Docs/Release/MASTER_BOARD.md',checks:checks.map(c=>({id:c.id,status:c.status}))},null,2));
const failed=checks.some(c=>c.required!==false && c.status==='failed');
process.exitCode=failed?1:readiness.ready?0:2;
