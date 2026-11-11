'use strict';
// Split the unpublished source snapshot in isolation. Never reset working files.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const {spawnSync} = require('node:child_process');
const {git, digest, inside} = require('./source-control-baseline');

const MiB = 1024 * 1024;
const LFS_PATTERN = /\.(uasset|umap|ubulk|uexp|fbx|glb|blend|wav|ogg|mp3|mp4|webm|mov|psd|exr|tga)$/i;
const EXTENSIONS = ['uasset','umap','ubulk','uexp','fbx','glb','blend','wav','ogg','mp3','mp4','webm','mov','psd','exr','tga'];
const ATTRIBUTES = '# Unreal packages and production media belong in Git LFS.\n' +
  EXTENSIONS.flatMap(ext => [ext, ext.toUpperCase()].map(name => '*.' + name + ' filter=lfs diff=lfs merge=lfs -text')).join('\n') + '\n';
const MAINTENANCE = [
  'README.md', 'Docs/Release/MASTER_BOARD.md', 'Docs/Release/observations.json',
  'Docs/Release/systems.json', 'Docs/Release/SOURCE_UPLOAD.md',
  'Tools/Release/chunked-source-upload.js', 'Tools/Release/chunked-source-upload.test.js',
  'Tools/Release/Publish-SourceChunks.ps1'
];
const readJson = file => JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, ''));
const writeJson = (file, value) => fs.writeFileSync(file, JSON.stringify(value, null, 2) + '\n');
function tree(root, ref, gitDir) {
  return git(root, ['ls-tree','-rlz',ref], {gitDir, binary:true}).toString('utf8').split('\0').filter(Boolean).map(row => {
    const m = /^(\d+) (\w+) ([a-f0-9]+)\s+(\d+|-)\t([\s\S]+)$/.exec(row);
    if (!m || m[2] !== 'blob' || !['100644','100755'].includes(m[1])) throw Error('Only regular source files can be split.');
    return {path:m[5], mode:m[1], oid:m[3], bytes:Number(m[4])};
  });
}
function config(root, key) {
  const result = git(root, ['config','--get',key], {allowFailure:true});
  return typeof result === 'string' ? result : '';
}
function batchFiles(files, limit = 512 * MiB) {
  if (!Number.isSafeInteger(limit) || limit <= 0) throw Error('A positive batch size is required.');
  const batches = [];
  let batch = [], bytes = 0;
  for (const file of files) {
    if (file.bytes >= 2 * 1024 * MiB) throw Error('One file exceeds the conservative 2 GiB LFS ceiling: ' + file.path);
    if (batch.length && bytes + file.bytes > limit) { batches.push(batch); batch = []; bytes = 0; }
    batch.push(file); bytes += file.bytes;
  }
  if (batch.length) batches.push(batch);
  return batches;
}
function pointer(oid, bytes) {
  return bytes === 0 ? '' : 'version https://git-lfs.github.com/spec/v1\noid sha256:' + oid + '\nsize ' + bytes + '\n';
}
function blobOid(contents) {
  return crypto.createHash('sha1').update('blob '+Buffer.byteLength(contents)+'\0').update(contents).digest('hex');
}
function hashAsset(file, expected) {
  const before = fs.statSync(file), sha256 = crypto.createHash('sha256'), sha1 = crypto.createHash('sha1');
  sha1.update('blob ' + before.size + '\0');
  const fd = fs.openSync(file, 'r'), buffer = Buffer.allocUnsafe(MiB);
  try { let n; while ((n = fs.readSync(fd, buffer, 0, buffer.length, null))) { sha256.update(buffer.subarray(0,n)); sha1.update(buffer.subarray(0,n)); } }
  finally { fs.closeSync(fd); }
  const after = fs.statSync(file), blob = sha1.digest('hex');
  if (before.size !== expected.bytes || before.size !== after.size || before.mtimeMs !== after.mtimeMs || blob !== expected.oid)
    throw Error('Asset differs from the original committed bytes: ' + expected.path);
  return sha256.digest('hex');
}
function lfsPath(gitDir, oid) {
  if (!/^[a-f0-9]{64}$/.test(oid)) throw Error('Invalid LFS object identity.');
  return path.join(gitDir, 'lfs/objects', oid.slice(0,2), oid.slice(2,4), oid);
}
function copyObject(source, destination, oid) {
  if (fs.existsSync(destination)) {
    if (digest(destination) !== oid) throw Error('Existing LFS object is corrupt: ' + oid);
    return;
  }
  fs.mkdirSync(path.dirname(destination), {recursive:true});
  const temporary = destination + '.upload-' + process.pid;
  fs.copyFileSync(source, temporary, fs.constants.COPYFILE_EXCL);
  if (digest(temporary) !== oid) throw Error('Copied asset failed SHA-256 verification: ' + oid);
  fs.renameSync(temporary, destination);
}
function stage(root, gitDir, files) {
  if (files.length) git(root, ['update-index','--add','-z','--index-info'], {
    gitDir, input:files.map(f => f.mode + ' ' + f.oid + '\t' + f.path + '\0').join('')
  });
}
function commit(root, gitDir, branch, parent, message) {
  const currentTree = git(root, ['write-tree'], {gitDir});
  const sha = git(root, ['commit-tree',currentTree,...(parent ? ['-p',parent] : [])], {gitDir,input:message + '\n'});
  git(root, ['update-ref','refs/heads/' + branch,sha,...(parent ? [parent] : ['0'.repeat(40)])], {gitDir});
  return sha;
}
function changedPaths(root, includeUntracked = true) {
  return [...new Set([
    ...git(root, ['diff','--name-only','--no-renames','-z','HEAD']).split('\0'),
    ...(includeUntracked ? git(root, ['ls-files','--others','--exclude-standard','-z']).split('\0') : [])
  ].filter(Boolean))];
}
function prepare(root, output, options = {}) {
  root = path.resolve(root); output = inside(root, path.resolve(output));
  if (fs.existsSync(path.join(output, 'plan.json'))) return verify(root, output);
  if (fs.existsSync(path.join(output, 'snapshot'))) throw Error('Partial preparation exists. Preserve it and use a new --prepared directory.');
  const original = git(root, ['rev-parse','--verify','HEAD']);
  const originalHistory = git(root, ['rev-list',original]).split('\n');
  const originalBranch = git(root, ['symbolic-ref','--quiet','--short','HEAD']);
  const branch = options.branch || (originalBranch==='main' ? 'master' : originalBranch);
  if (branch==='main') throw Error('The updater main branch must not be the source upload destination.');
  git(root,['check-ref-format','refs/heads/'+branch]);
  const originalGitDir = git(root, ['rev-parse','--absolute-git-dir']);
  const originalIndexFingerprint = crypto.createHash('sha256').update(git(root,['ls-files','--stage','-z'],{binary:true})).digest('hex');
  const maintenance = (options.maintenance || MAINTENANCE).filter(name => fs.existsSync(inside(root,path.join(root,name))));
  const unexpected = changedPaths(root,false).filter(name => !maintenance.includes(name));
  if (unexpected.length) throw Error('Unreviewed local changes must be preserved separately: ' + unexpected.slice(0,8).join(', '));
  const untrackedPreserved = git(root,['ls-files','--others','--exclude-standard','-z']).split('\0').filter(name=>name&&!maintenance.includes(name));
  if (fs.existsSync(path.join(root, '.gitattributes'))) throw Error('Existing attributes require review; they will not be overwritten.');
  const originalFiles = tree(root, original);
  const assets = originalFiles.filter(f => LFS_PATTERN.test(f.path));
  const core = originalFiles.filter(f => !LFS_PATTERN.test(f.path));
  if (core.some(f => f.bytes > 100 * MiB)) throw Error('A large non-LFS file needs an explicit tracking rule.');
  const batches = batchFiles(assets, options.batchBytes);
  fs.mkdirSync(output, {recursive:true});
  const snapshot = path.join(output, 'snapshot'), gitDir = path.join(snapshot, '.git');
  git(root, ['init','--quiet','--initial-branch=' + branch,snapshot]);
  for (const [key,value] of [
    ['user.name',config(root,'user.name')], ['user.email',config(root,'user.email')],
    ['core.worktree',root], ['core.autocrlf',config(root,'core.autocrlf') || 'false'],
    ['core.filemode','false'], ['core.symlinks','false'], ['commit.gpgsign','false'],
    ['gc.auto','0'], ['maintenance.auto','false'], ['lfs.storage',path.join(gitDir,'lfs')]
  ]) { if (!value) throw Error('Git author configuration is missing.'); git(root, ['config',key,value], {gitDir}); }
  fs.writeFileSync(path.join(gitDir, 'objects/info/alternates'), path.join(originalGitDir,'objects').replaceAll('\\','/') + '\n');
  const remote = config(root, 'remote.origin.url');
  if (!remote) throw Error('The origin remote must be configured.');
  git(root, ['remote','add','origin',remote], {gitDir});
  git(root, ['lfs','install','--local'], {gitDir});
  git(root, ['read-tree','--empty'], {gitDir});
  stage(root, gitDir, core);
  const attributesOid = git(root, ['hash-object','-w','--stdin'], {gitDir,input:ATTRIBUTES});
  stage(root, gitDir, [{path:'.gitattributes',mode:'100644',oid:attributesOid}]);
  const commits = [], lfsFiles = [], assetHashes = new Map();
  let parent = commit(root,gitDir,branch,null,'Establish Lust House project source and Git LFS rules');
  commits.push({sha:parent,message:'Establish Lust House project source and Git LFS rules',files:core.length + 1,bytes:core.reduce((n,f)=>n+f.bytes,0),lfsOids:[]});
  for (const [number,batch] of batches.entries()) {
    const staged = [], oids = new Set(), bytes = batch.reduce((n,f)=>n+f.bytes,0);
    for (const asset of batch) {
      const source = inside(root,path.join(root,asset.path)), oid = hashAsset(source,asset);
      if (asset.bytes) { copyObject(source,lfsPath(gitDir,oid),oid); oids.add(oid); }
      const blob = git(root,['hash-object','-w','--stdin'],{gitDir,input:pointer(oid,asset.bytes)});
      staged.push({...asset,oid:blob});
      lfsFiles.push({...asset,sha256:oid,pointerOid:blob});
      assetHashes.set(asset.path,oid);
    }
    stage(root,gitDir,staged);
    const message = 'Add Unreal content batch ' + String(number+1).padStart(2,'0') + ' through Git LFS';
    parent = commit(root,gitDir,branch,parent,message);
    commits.push({sha:parent,message,files:batch.length,bytes,lfsOids:[...oids]});
    if (!options.quiet) console.log('Prepared batch ' + (number+1) + '/' + batches.length + ': ' + batch.length + ' files, ' + (bytes/MiB).toFixed(1) + ' MiB');
  }
  if (maintenance.length) {
    git(root, ['add','--',...maintenance], {gitDir});
    const message = 'Document source upload recovery and current REL-01 evidence';
    parent = commit(root,gitDir,branch,parent,message);
    commits.push({sha:parent,message,files:maintenance.length,bytes:maintenance.reduce((n,f)=>n+fs.statSync(path.join(root,f)).size,0),lfsOids:[]});
  }
  const files = tree(root,parent,gitDir);
  const worktreeFiles = files.filter(f=>f.path!=='.gitattributes').map(f => {
    const source = inside(root,path.join(root,f.path));
    return {path:f.path,bytes:fs.statSync(source).size,sha256:assetHashes.get(f.path)||digest(source)};
  });
  if (git(root,['rev-parse','HEAD']) !== original) throw Error('Original HEAD changed during preparation; nothing was installed.');
  const plan = {
    schemaVersion:1,preparedAt:new Date().toISOString(),projectRoot:root,originalGitDir,
    originalCommit:original,originalTree:git(root,['rev-parse',original+'^{tree}']),
    originalBranch,originalHistory,originalIndexFingerprint,branch,remote,untrackedPreserved,
    gitDirectory:'snapshot/.git',attributes:ATTRIBUTES,attributesOid,
    commits,finalCommit:parent,finalTree:git(root,['rev-parse',parent+'^{tree}'],{gitDir}),
    originalFiles,files,worktreeFiles,lfsFiles,maintenance,batchBytes:options.batchBytes||512*MiB,
    originalSnapshotPreserved:true,remoteVerified:false
  };
  writeJson(path.join(output,'plan.json'),plan);
  const summary = ['# Prepared source upload','',
    'Original local commit: '+original+'. It remains untouched until the publisher preserves it on a backup branch.',
    'Source destination: '+branch+'. Existing main is not updated.',
    'Run Tools/Release/Publish-SourceChunks.ps1 from a normal project terminal to publish and install.',
    '', '| Commit | Files | Payload MiB | SHA |', '| --- | ---: | ---: | --- |',
    ...commits.map(c=>'| '+c.message+' | '+c.files+' | '+(c.bytes/MiB).toFixed(1)+' | '+c.sha+' |'),
    '', 'Git stores small pointers for LFS files. Payload size includes the separate LFS data. No original files were removed.', ''
  ].join('\n');
  fs.writeFileSync(path.join(output,'UPLOAD_PLAN.md'),summary);
  return verify(root,output);
}
function load(root, output) {
  output = inside(root,path.resolve(output));
  const plan = readJson(path.join(output,'plan.json'));
  if (plan.schemaVersion!==1 || path.resolve(plan.projectRoot).toLowerCase()!==path.resolve(root).toLowerCase())
    throw Error('Upload plan belongs to a different project.');
  const gitDir = inside(output,path.resolve(output,plan.gitDirectory));
  git(root,['check-ref-format','refs/heads/'+plan.branch]);
  if (!/^[a-f0-9]{40}$/.test(plan.originalCommit) || !/^[a-f0-9]{40}$/.test(plan.finalCommit)) throw Error('Invalid commit identities.');
  return {plan,gitDir,output};
}
function verifyWorktree(root, plan) {
  for (const f of plan.worktreeFiles) {
    const file = inside(root,path.join(root,f.path));
    if (!fs.existsSync(file)||fs.statSync(file).size!==f.bytes||digest(file)!==f.sha256)
      throw Error('Working file changed since preparation; do not install over it: '+f.path);
  }
  const attrs = path.join(root,'.gitattributes');
  if (fs.existsSync(attrs) && fs.readFileSync(attrs,'utf8')!==plan.attributes) throw Error('Local Git attributes changed since preparation.');
}
function verify(root, output) {
  const {plan,gitDir} = load(root,output);
  if (git(root,['rev-parse','HEAD'],{gitDir})!==plan.finalCommit ||
      git(root,['rev-parse',plan.finalCommit+'^{tree}'],{gitDir})!==plan.finalTree)
    throw Error('Prepared source history changed.');
  if (JSON.stringify(tree(root,plan.finalCommit,gitDir))!==JSON.stringify(plan.files)) throw Error('Prepared file inventory changed.');
  const prepared = new Map(plan.files.map(f=>[f.path,f]));
  for (const original of plan.originalFiles) {
    const f = prepared.get(original.path), lfs = plan.lfsFiles.find(a=>a.path===original.path);
    if (!f || (lfs ? f.oid!==lfs.pointerOid : !plan.maintenance.includes(f.path)&&f.oid!==original.oid))
      throw Error('Original committed content was not preserved: '+original.path);
    if (lfs && (lfs.oid!==original.oid || lfs.bytes!==original.bytes || f.mode!==original.mode))
      throw Error('Original asset provenance changed: '+original.path);
    if (lfs && f.oid!==blobOid(pointer(lfs.sha256,lfs.bytes)))
      throw Error('Prepared LFS pointer differs from the verified asset: '+original.path);
  }
  const history = git(root,['rev-list','--reverse',plan.finalCommit],{gitDir}).split('\n');
  if (JSON.stringify(history)!==JSON.stringify(plan.commits.map(c=>c.sha))) throw Error('Commit chain differs from the upload order.');
  // The original large root must not be an ancestor of the new upload branch.
  if (history.includes(plan.originalCommit)) throw Error('Oversized original history entered the upload chain.');
  const reachable = git(root,['rev-list','--objects',plan.finalCommit],{gitDir}).split('\n').map(line=>line.split(' ')[0]).join('\n')+'\n';
  const objects = git(root,['cat-file','--batch-check=%(objecttype) %(objectsize)'],{gitDir,input:reachable});
  if (objects.split('\n').some(line=>{const [kind,size]=line.split(' ');return kind==='blob'&&Number(size)>100*MiB;}))
    throw Error('An oversized raw Git blob remains reachable.');
  verifyWorktree(root,plan);
  const gitCheck=git(root,['fsck','--connectivity-only','--no-dangling'],{gitDir});
  const lfsCheck=git(root,['lfs','fsck','--objects','--pointers',plan.finalCommit],{gitDir});
  fs.writeFileSync(path.join(output,'git-fsck.log'),(gitCheck||'Git object connectivity verified.')+'\n');
  fs.writeFileSync(path.join(output,'lfs-fsck.log'),lfsCheck+'\n');
  const result = {status:'verified-prepared',checkedAt:new Date().toISOString(),originalCommit:plan.originalCommit,
    finalCommit:plan.finalCommit,commits:plan.commits.length,files:plan.files.length,lfsFiles:plan.lfsFiles.length,
    assetBytes:plan.lfsFiles.reduce((n,f)=>n+f.bytes,0),largestBatchBytes:Math.max(...plan.commits.map(c=>c.bytes)),
    originalSnapshotPreserved:true,remoteVerified:false,preparedDirectory:output,
    evidence:['plan.json','git-fsck.log','lfs-fsck.log']};
  writeJson(path.join(output,'verification.json'),result);
  return result;
}
function remoteHead(root, gitDir, branch) {
  const result = git(root,['ls-remote','--heads','origin','refs/heads/'+branch],{gitDir,timeout:60000});
  if (!result) return null;
  const parts = result.split(/\s+/);
  if (parts.length!==2 || parts[1]!=='refs/heads/'+branch || !/^[a-f0-9]{40}$/.test(parts[0])) throw Error('Unexpected remote branch response.');
  return parts[0];
}
function resumeIndex(commits, remote) {
  if (!remote) return -1;
  const index = commits.findIndex(c=>c.sha===remote);
  if (index<0) throw Error('Remote source branch contains other work. No force push will be performed.');
  return index;
}
function live(root, gitDir, args, input) {
  const env = {...process.env};
  for (const key of ['GIT_DIR','GIT_WORK_TREE','GIT_INDEX_FILE','GIT_OBJECT_DIRECTORY','GIT_ALTERNATE_OBJECT_DIRECTORIES']) delete env[key];
  const result = spawnSync('git',['--git-dir='+gitDir,'--work-tree='+root,...args],{
    cwd:root,env,windowsHide:true,stdio:input===undefined?'inherit':['pipe','inherit','inherit'],input
  });
  if (result.error||result.status!==0) throw Error(result.error?.message||'Upload failed. Rerun the same command to resume after resolving the reported error.');
}
function publish(root, output, options = {}) {
  const {plan,gitDir} = load(root,output);
  verify(root,output);
  if (git(root,['remote','get-url','origin'],{gitDir})!==plan.remote) throw Error('Prepared origin changed.');
  const remote = remoteHead(root,gitDir,plan.branch), index = resumeIndex(plan.commits,remote);
  for (const [number,c] of plan.commits.entries()) {
    if (number<=index) continue;
    console.log('Uploading commit '+(number+1)+'/'+plan.commits.length+': '+c.message+' ('+(c.bytes/MiB).toFixed(1)+' MiB payload)');
    if (c.lfsOids.length) live(root,gitDir,['-c','lfs.concurrenttransfers=2','lfs','push','--object-id','--stdin','origin'],c.lfsOids.join('\n')+'\n');
    live(root,gitDir,['-c','lfs.concurrenttransfers=2','push','origin',c.sha+':refs/heads/'+plan.branch]);
    if (remoteHead(root,gitDir,plan.branch)!==c.sha) throw Error('Remote did not confirm the uploaded commit.');
    writeJson(path.join(output,'publish-progress.json'),{checkedAt:new Date().toISOString(),remote:plan.remote,branch:plan.branch,commit:c.sha,completedCommits:number+1,totalCommits:plan.commits.length});
  }
  if (remoteHead(root,gitDir,plan.branch)!==plan.finalCommit) throw Error('Final remote source revision was not verified.');
  writeJson(path.join(output,'published.json'),{checkedAt:new Date().toISOString(),commit:plan.finalCommit,branch:plan.branch,remote:plan.remote,remoteVerified:true});
  return options.install===false ? {status:'published',commit:plan.finalCommit} : install(root,output);
}
function install(root, output) {
  const {plan,gitDir} = load(root,output);
  verifyWorktree(root,plan);
  const current = git(root,['rev-parse','HEAD']), branch = git(root,['symbolic-ref','--quiet','--short','HEAD']);
  if (![plan.originalBranch,plan.branch].includes(branch) || ![plan.originalCommit,plan.finalCommit].includes(current))
    throw Error('The active branch changed. Prepared history was not installed.');
  const published = readJson(path.join(output,'published.json'));
  if (!published.remoteVerified || published.commit!==plan.finalCommit || remoteHead(root,gitDir,plan.branch)!==plan.finalCommit)
    throw Error('Verified remote publication is required before installing the split source history.');
  const activeDir = git(root,['rev-parse','--absolute-git-dir']);
  if (path.resolve(activeDir).toLowerCase()!==path.resolve(plan.originalGitDir).toLowerCase()) throw Error('Active Git directory changed.');
  for (const lock of ['index.lock','HEAD.lock','refs/heads/'+branch+'.lock'])
    if (fs.existsSync(path.join(activeDir,lock))) throw Error('Another Git operation holds '+lock+'. Let it finish.');
  const expectedChanges = new Set([...plan.originalFiles.map(f=>f.path),...plan.maintenance,'.gitattributes']);
  if (current===plan.originalCommit && changedPaths(root,false).some(p=>!expectedChanges.has(p))) throw Error('Additional tracked work appeared; installation stopped.');
  const indexFingerprint=crypto.createHash('sha256').update(git(root,['ls-files','--stage','-z'],{binary:true})).digest('hex');
  if (current===plan.originalCommit && indexFingerprint!==plan.originalIndexFingerprint) throw Error('Staged work changed since preparation; installation stopped.');
  git(root,['lfs','install','--local']);
  const backupRef = 'refs/heads/backup/pre-lfs-'+plan.originalCommit.slice(0,12);
  const prior = git(root,['rev-parse','--verify',backupRef],{allowFailure:true});
  if (typeof prior==='string'&&prior!==plan.originalCommit) throw Error('Backup branch already points to different work.');
  if (typeof prior!=='string') git(root,['update-ref',backupRef,plan.originalCommit,'0'.repeat(40)]);
  git(root,['fetch','--quiet','--no-tags',path.dirname(gitDir),plan.finalCommit]);
  // Read the effective LFS store after installing filters; preserve custom storage.
  const lfsEnvironment = git(root,['lfs','env']);
  const storage = /^LocalMediaDir=(.+)$/m.exec(lfsEnvironment)?.[1];
  if (!storage) throw Error('Cannot resolve the active Git LFS object store.');
  for (const asset of plan.lfsFiles) {
    if (!asset.bytes) continue;
    copyObject(lfsPath(gitDir,asset.sha256),path.join(storage,asset.sha256.slice(0,2),asset.sha256.slice(2,4),asset.sha256),asset.sha256);
  }
  const indexBackup = path.join(output,'active-index-before-upload');
  if (!fs.existsSync(indexBackup)) fs.copyFileSync(path.join(activeDir,'index'),indexBackup,fs.constants.COPYFILE_EXCL);
  writeJson(path.join(output,'install-journal.json'),{startedAt:new Date().toISOString(),originalCommit:plan.originalCommit,finalCommit:plan.finalCommit,backupRef});
  // Only metadata and the new attributes file change; no checkout or hard reset.
  const attributesPath = path.join(root,'.gitattributes');
  if (!fs.existsSync(attributesPath)) fs.writeFileSync(attributesPath,plan.attributes,{flag:'wx'});
  const destinationRef='refs/heads/'+plan.branch;
  if (branch!==plan.branch) {
    const destination=git(root,['rev-parse','--verify',destinationRef],{allowFailure:true});
    if (typeof destination==='string'&&destination!==plan.finalCommit) throw Error('The local source destination contains other work.');
    if (typeof destination!=='string') git(root,['update-ref',destinationRef,plan.finalCommit,'0'.repeat(40)]);
    git(root,['symbolic-ref','-m','Select verified LFS source baseline','HEAD',destinationRef]);
  } else if (current!==plan.finalCommit) git(root,['update-ref','-m','Install verified chunked LFS baseline',destinationRef,plan.finalCommit,plan.originalCommit]);
  git(root,['read-tree',plan.finalCommit]);
  git(root,['config','branch.'+plan.branch+'.remote','origin']);
  git(root,['config','branch.'+plan.branch+'.merge','refs/heads/'+plan.branch]);
  git(root,['update-ref','refs/remotes/origin/'+plan.branch,plan.finalCommit]);
  git(root,['lfs','fsck','--objects','--pointers','HEAD']);
  if (git(root,['diff','--name-only','HEAD'])) throw Error('Publication succeeded, but the installed checkout needs review. Original backup is preserved.');
  const result = {status:'published-and-installed',checkedAt:new Date().toISOString(),branch:plan.branch,commit:plan.finalCommit,backupRef,originalCommit:plan.originalCommit,remoteVerified:true,workingFilesPreserved:true};
  writeJson(path.join(output,'installed.json'),result);
  return result;
}
module.exports = {prepare,verify,publish,install,batchFiles,pointer,resumeIndex,ATTRIBUTES,MAINTENANCE,LFS_PATTERN};
if (require.main===module) {
  const args=process.argv.slice(2), option=key=>{const i=args.indexOf(key);return i<0?null:args[i+1];};
  const root=path.resolve(option('--project-root')||path.join(__dirname,'../..'));
  const output=path.resolve(option('--prepared')||path.join(root,'Saved/ReleaseAudit/REL01-2026-11-11/chunked-upload'));
  try {
    const command=args[0];
    const result=command==='prepare'?prepare(root,output):command==='verify'?verify(root,output):command==='publish'?publish(root,output):null;
    if (!result) throw Error('Use prepare, verify, or publish.');
    console.log(JSON.stringify(result,null,2));
  } catch(error) {console.error(error.message);process.exitCode=1;}
}
