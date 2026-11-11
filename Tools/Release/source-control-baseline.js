'use strict';
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const {spawnSync}=require('node:child_process');
const {fingerprint}=require('./release-policy');
const SOURCE_PATHS=['.gitignore','.vsconfig','README.md','ITCH_DEVLOG.md','SugarLustUE.uproject','Source','Backend','Config','Content','Media','Build','Docs','Tools'];
const readJson=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const digest=p=>{const h=crypto.createHash('sha256'),fd=fs.openSync(p,'r'),b=Buffer.allocUnsafe(1024*1024);try{let n;while((n=fs.readSync(fd,b,0,b.length,null)))h.update(b.subarray(0,n));}finally{fs.closeSync(fd);}return h.digest('hex');};
const norm=p=>path.resolve(p).replaceAll('\\','/').toLowerCase();
function inside(root,file){const rel=path.relative(path.resolve(root),path.resolve(file));if(rel==='..'||rel.startsWith('..'+path.sep)||path.isAbsolute(rel))throw Error('Path escaped its intended root: '+file);return file;}
function git(root,args,options={}){
 const env={...process.env};for(const key of ['GIT_DIR','GIT_WORK_TREE','GIT_INDEX_FILE','GIT_OBJECT_DIRECTORY','GIT_ALTERNATE_OBJECT_DIRECTORIES'])delete env[key];
 const prefix=options.gitDir?['--git-dir='+options.gitDir,'--work-tree='+root]:['-C',root];
 const r=spawnSync('git',['--no-optional-locks',...prefix,...args],{cwd:root,env,encoding:options.binary?null:'utf8',input:options.input,windowsHide:true,maxBuffer:64*1024*1024,timeout:options.timeout||600000});
 if(r.error||r.status!==0){if(options.allowFailure)return {ok:false,status:r.status,error:r.error?.message||String(r.stderr||r.stdout)};throw Error(r.error?.message||String(r.stderr||r.stdout).trim()||'Git failed: '+args[0]);}
 return options.binary?r.stdout:String(r.stdout).trim();
}
function inspect(root){
 const head=git(root,['rev-parse','--verify','HEAD'],{allowFailure:true});
 if(typeof head!=='string')return {status:'blocked',summary:'The project has no resolvable local source commit.',error:head.error};
 const top=git(root,['rev-parse','--show-toplevel']);
 if(norm(top)!==norm(root))return {status:'blocked',summary:'Git resolved an ancestor or different project rather than this Unreal root.'};
 const branch=git(root,['symbolic-ref','--quiet','--short','HEAD'],{allowFailure:true});
 if(typeof branch!=='string')return {status:'blocked',summary:'The source baseline is detached from a named branch.'};
 const indexPath=path.resolve(root,git(root,['rev-parse','--git-path','index']));
 if(!fs.existsSync(indexPath))return {status:'blocked',summary:'HEAD exists but the working index is missing.'};
 return {status:'passed',summary:`${branch} at ${head}; repository root matches the Unreal project.`,commit:head,branch,tree:git(root,['rev-parse','HEAD^{tree}']),root:path.resolve(top)};
}
function tracked(root,dir){
 const data=git(root,['ls-files','--stage','-z'],{gitDir:dir,binary:true}).toString('utf8');
 return data.split('\0').filter(Boolean).map(line=>{const m=/^(\d+) ([a-f0-9]{40}) 0\t([\s\S]+)$/.exec(line);if(!m)throw Error('Unexpected index entry.');const file=inside(root,path.join(root,m[3]));if(!fs.statSync(file).isFile())throw Error('Baseline entry is not a regular file: '+m[3]);return {path:m[3].replaceAll('\\','/'),mode:m[1],blob:m[2],bytes:fs.statSync(file).size,sha256:digest(file)};});
}
function verifyFiles(root,files){
 const failures=[];for(const f of files){const target=inside(root,path.resolve(root,f.path));if(!fs.existsSync(target)||fs.statSync(target).size!==f.bytes||digest(target)!==f.sha256)failures.push(f.path);}
 if(failures.length)throw Error('Baseline file verification failed: '+failures.slice(0,8).join(', '));
 return files.length;
}
function looseObjects(dir){
 const result=[];for(const e of fs.readdirSync(path.join(dir,'objects'),{withFileTypes:true})){if(!e.isDirectory()||!/^[a-f0-9]{2}$/.test(e.name))continue;for(const f of fs.readdirSync(path.join(dir,'objects',e.name),{withFileTypes:true})){if(f.isFile()&&/^[a-f0-9]{38}$/.test(f.name)){const relative='objects/'+e.name+'/'+f.name;result.push({path:relative,sha256:digest(path.join(dir,relative))});}}}return result;
}
function prepare(root,output){
 root=path.resolve(root);output=inside(root,path.resolve(output));
 if(fs.existsSync(path.join(output,'prepared.json')))throw Error('Prepared baseline already exists; retain it rather than replacing its commit.');
 fs.mkdirSync(output,{recursive:true});
 const original=path.join(root,'.git');
 const expectedHead=fs.readFileSync(path.join(original,'HEAD'),'utf8').trim();
 if(!/^ref: refs\/heads\/[A-Za-z0-9_./-]+$/.test(expectedHead))throw Error('A normal named local branch is required for this recovery.');
 const existing=git(root,['rev-parse','--verify','HEAD'],{allowFailure:true});if(typeof existing==='string')throw Error('The existing repository already has a commit; do not replace its history.');
 if(fs.existsSync(path.join(original,'index')))throw Error('An existing index needs separate reconciliation; it will not be replaced.');
 const name=git(root,['config','--file',path.join(original,'config'),'--get','user.name']);
 const email=git(root,['config','--file',path.join(original,'config'),'--get','user.email']);
 const repo=path.join(output,'snapshot'),dir=path.join(repo,'.git'),branch=expectedHead.slice('ref: refs/heads/'.length);
 fs.mkdirSync(repo,{recursive:true});git(root,['init','--quiet','--initial-branch='+branch,repo]);
 for(const [key,value]of [['user.name',name],['user.email',email],['core.autocrlf','false'],['core.filemode','false'],['core.symlinks','false'],['commit.gpgsign','false'],['gc.auto','0'],['maintenance.auto','false']])git(root,['config',key,value],{gitDir:dir});
 // The original object database is read only. Only NEW objects are written here.
 fs.writeFileSync(path.join(dir,'objects/info/alternates'),path.join(original,'objects').replaceAll('\\','/')+'\n');
 const selected=SOURCE_PATHS.filter(p=>fs.existsSync(path.join(root,p)));
 git(root,['add','--all','--',...selected],{gitDir:dir});
 const files=tracked(root,dir);
 if(!files.some(f=>f.path==='SugarLustUE.uproject')||!files.some(f=>f.path.startsWith('Source/')))throw Error('The Unreal project and native source must be in the baseline.');
 if(files.some(f=>/^(Saved|Binaries|Intermediate|DerivedDataCache|DemoBuilds|AndroidBuild)\//i.test(f.path)||/UserEngine\.ini$|\.keystore$|\.clixml$|__pycache__|\/tmp[^/]*\//i.test(f.path)))throw Error('Generated or private data entered the source baseline.');
 git(root,['commit','--quiet','-m','Establish verified Lust House Unreal source baseline'],{gitDir:dir});
 const commit=git(root,['rev-parse','HEAD'],{gitDir:dir}),tree=git(root,['rev-parse','HEAD^{tree}'],{gitDir:dir});
 git(root,['fsck','--connectivity-only','--no-dangling'],{gitDir:dir});
 const index=path.join(dir,'index');
 const result={schemaVersion:1,preparedAt:new Date().toISOString(),projectRoot:root,expectedHead,branch,commit,tree,sourceFingerprint:fingerprint(root),gitDirectory:'snapshot/.git',indexSha256:digest(index),objects:looseObjects(dir),files,bytes:files.reduce((n,f)=>n+f.bytes,0),remoteNote:'The configured GitHub main is updater distribution metadata, not this Unreal source history.'};
 fs.writeFileSync(path.join(output,'prepared.json'),JSON.stringify(result,null,2)+'\n');
 return {status:'prepared',commit,tree,files:files.length,bytes:result.bytes,newObjects:result.objects.length,sourceFingerprint:result.sourceFingerprint,preparedDirectory:output};
}
function verifyPrepared(root,output){
 const m=readJson(path.join(output,'prepared.json'));
 if(m.schemaVersion!==1||norm(m.projectRoot)!==norm(root)||!/^[a-f0-9]{40}$/.test(m.commit)||m.expectedHead!=='ref: refs/heads/'+m.branch)throw Error('Prepared metadata belongs to a different or invalid project.');
 const dir=inside(output,path.join(output,m.gitDirectory));
 if(git(root,['rev-parse','HEAD'],{gitDir:dir})!==m.commit||git(root,['rev-parse','HEAD^{tree}'],{gitDir:dir})!==m.tree)throw Error('Prepared commit or tree changed.');
 if(digest(path.join(dir,'index'))!==m.indexSha256)throw Error('Prepared index changed.');
 for(const o of m.objects){if(!/^objects\/[a-f0-9]{2}\/[a-f0-9]{38}$/.test(o.path)||digest(inside(dir,path.join(dir,o.path)))!==o.sha256)throw Error('Prepared object changed: '+o.path);}
 return {manifest:m,dir};
}
function materialize(root,output,destination){
 const {manifest:m}=verifyPrepared(root,output);destination=inside(root,path.resolve(destination));
 if(fs.existsSync(destination))throw Error('Verification checkout must be a new empty destination.');
 git(root,['clone','--quiet','--shared','--no-checkout','--no-hardlinks',path.join(output,'snapshot'),destination]);
 git(destination,['config','core.autocrlf','false']);git(destination,['config','core.filemode','false']);
 git(destination,['checkout','--quiet',m.branch]);
 verifyFiles(destination,m.files);
 if(fingerprint(destination)!==m.sourceFingerprint)throw Error('Clean checkout source/asset fingerprint differs.');
 if(git(destination,['status','--porcelain']).length)throw Error('The verification checkout is not clean.');
 const result={status:'verified',checkedAt:new Date().toISOString(),commit:m.commit,tree:m.tree,sourceFingerprint:m.sourceFingerprint,files:m.files.length,checkout:destination};
 fs.writeFileSync(path.join(output,'checkout-verification.json'),JSON.stringify(result,null,2)+'\n');return result;
}
function atomicNew(file,data){
 fs.mkdirSync(path.dirname(file),{recursive:true});
 if(fs.existsSync(file)){if(Buffer.compare(fs.readFileSync(file),data)===0)return;throw Error('Refusing to overwrite existing metadata: '+file);}
 const lock=file+'.rel01-lock';let created=false;
 try{const fd=fs.openSync(lock,'wx');created=true;try{fs.writeFileSync(fd,data);}finally{fs.closeSync(fd);}if(fs.existsSync(file))throw Error('Metadata appeared during repair: '+file);fs.renameSync(lock,file);created=false;}
 finally{if(created&&fs.existsSync(lock))fs.unlinkSync(lock);}
}
function apply(root,output,options={}){
 root=path.resolve(root);output=inside(root,path.resolve(output));
 const {manifest:m,dir}=verifyPrepared(root,output),original=path.join(root,'.git');
 const before=fingerprint(root),health=inspect(root),journalPath=path.join(output,'apply-journal.json');
 if(health.status==='passed'){
  if(health.commit!==m.commit||health.branch!==m.branch)throw Error('A different valid source history now exists. Repair refused.');
  const result={status:'already_repaired',checkedAt:new Date().toISOString(),...health,sourceFingerprint:before};
  fs.writeFileSync(path.join(output,'applied.json'),JSON.stringify(result,null,2)+'\n');return result;
 }
 if(fs.readFileSync(path.join(original,'HEAD'),'utf8').trim()!==m.expectedHead)throw Error('HEAD changed since preparation. Repair refused.');
 const presentHead=git(root,['rev-parse','--verify','HEAD'],{allowFailure:true});
 if(typeof presentHead==='string'&&presentHead!==m.commit)throw Error('A different commit appeared. Repair refused.');
 const refs=path.join(original,'refs');
 function refFiles(p){return fs.existsSync(p)?fs.readdirSync(p,{withFileTypes:true}).flatMap(e=>e.isDirectory()?refFiles(path.join(p,e.name)):[path.join(p,e.name)]):[];}
 const otherRefs=refFiles(refs).filter(p=>p!==path.join(original,'refs/heads',m.branch));
 if(otherRefs.length||fs.existsSync(path.join(original,'packed-refs')))throw Error('Unreconciled branch/tag references appeared. Repair refused.');
 if(fs.existsSync(path.join(original,'index')))throw Error('An existing index will not be overwritten.');
 const backup=path.join(output,'metadata-backup');fs.mkdirSync(backup,{recursive:true});
 for(const file of ['HEAD','config','description','GITGUI_MSG']){const p=path.join(original,file);if(fs.existsSync(p))atomicNew(path.join(backup,file),fs.readFileSync(p));}
 fs.writeFileSync(journalPath,JSON.stringify({commit:m.commit,projectRoot:root,startedAt:new Date().toISOString(),sourceFingerprint:before},null,2)+'\n');
 for(const o of m.objects)atomicNew(inside(original,path.join(original,o.path)),fs.readFileSync(path.join(dir,o.path)));
 fs.mkdirSync(path.join(original,'refs/heads'),{recursive:true});
 if(typeof presentHead!=='string')git(root,['update-ref','--create-reflog','-m','REL-01 verified source baseline','refs/heads/'+m.branch,m.commit,'0'.repeat(40)]);
 git(root,['config','core.autocrlf','false']);
 atomicNew(path.join(original,'index'),fs.readFileSync(path.join(dir,'index')));
 git(root,['fsck','--connectivity-only','--no-dangling']);
 const after=fingerprint(root);if(after!==before)throw Error('Working source changed during metadata repair; preserve state for reconciliation.');
 const final=inspect(root);if(final.status!=='passed'||final.commit!==m.commit)throw Error('Repaired repository failed verification.');
 const result={status:'repaired',checkedAt:new Date().toISOString(),...final,sourceFingerprint:after,workingFilesPreserved:true,backupDirectory:backup};
 fs.writeFileSync(path.join(output,'applied.json'),JSON.stringify(result,null,2)+'\n');return result;
}

module.exports={prepare,materialize,apply,inspect,verifyPrepared,verifyFiles,digest,inside,git};
if(require.main===module){
 const args=process.argv.slice(2),command=args[0],value=k=>{const i=args.indexOf(k);return i<0?null:args[i+1];};
 const root=path.resolve(value('--project-root')||path.join(__dirname,'../..'));
 const output=path.resolve(value('--prepared')||path.join(root,'Saved/ReleaseAudit/REL01-2026-11-11/prepared-baseline'));
 try{let result;if(command==='prepare')result=prepare(root,output);else if(command==='materialize')result=materialize(root,output,path.resolve(value('--destination')||path.join(output,'clean-checkout')));else if(command==='apply')result=apply(root,output);else if(command==='inspect')result=inspect(root);else throw Error('Use prepare, materialize, apply, or inspect.');console.log(JSON.stringify(result,null,2));if(result.status==='blocked')process.exitCode=2;}
 catch(e){console.error(e.message);process.exitCode=1;}
}
