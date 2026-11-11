'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {git,digest} = require('./source-control-baseline');
const {prepare,verify,publish,batchFiles,pointer,resumeIndex} = require('./chunked-source-upload');

function fixture() {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(),'lhc-source-chunks-'));
  const root = path.join(directory,'Project With Spaces'), remote = path.join(directory,'origin.git');
  fs.mkdirSync(root,{recursive:true});
  git(root,['init','--quiet','--initial-branch=master',root]);
  git(root,['config','user.name','Source Upload Test']);
  git(root,['config','user.email','source-upload-test@example.invalid']);
  git(root,['config','core.autocrlf','false']);
  git(root,['config','commit.gpgsign','false']);
  fs.mkdirSync(path.join(root,'Content/Art'),{recursive:true});
  fs.mkdirSync(path.join(root,'Source'),{recursive:true});
  fs.mkdirSync(path.join(root,'Docs'),{recursive:true});
  fs.writeFileSync(path.join(root,'.gitignore'),'/Saved/\n');
  fs.writeFileSync(path.join(root,'Source/Game.cpp'),'// source must survive unchanged\n');
  fs.writeFileSync(path.join(root,'Docs/upload.txt'),'Original documentation\n');
  fs.writeFileSync(path.join(root,'Content/Art/a.uasset'),Buffer.alloc(3072,0xa1));
  fs.writeFileSync(path.join(root,'Content/Art/b.UMAP'),Buffer.alloc(1800,0xb2));
  fs.writeFileSync(path.join(root,'Content/Art/c.uasset'),Buffer.alloc(1500,0xc3));
  fs.writeFileSync(path.join(root,'Content/Art/empty.uasset'),Buffer.alloc(0));
  git(root,['add','--all']);
  git(root,['commit','--quiet','-m','Original oversized-root stand-in']);
  const original = git(root,['rev-parse','HEAD']);
  git(root,['init','--quiet','--bare','--initial-branch=main',remote]);
  git(root,['remote','add','origin',remote]);
  const emptyTree = git(root,['mktree'],{input:''});
  const main = git(root,['commit-tree',emptyTree],{input:'Existing updater branch\n'});
  const pack=git(root,['pack-objects','--stdout','--revs'],{input:main+'\n',binary:true});
  git(root,['unpack-objects'],{gitDir:remote,input:pack});
  git(root,['update-ref','refs/heads/main',main],{gitDir:remote});
  fs.writeFileSync(path.join(root,'Docs/upload.txt'),'Reviewed source upload documentation\n');
  return {directory,root,remote,original,main,output:path.join(root,'Saved/chunked-upload')};
}
const options = {maintenance:['Docs/upload.txt'],batchBytes:2048,quiet:true};

test('batch boundaries preserve every file and isolate an oversized single asset', () => {
  const files = [{path:'a',bytes:3},{path:'b',bytes:8},{path:'c',bytes:2},{path:'d',bytes:2}];
  assert.deepEqual(batchFiles(files,5),[[files[0]],[files[1]],[files[2],files[3]]]);
  assert.throws(()=>batchFiles([{path:'too-large',bytes:2*1024**3}],512*1024**2),/ceiling/);
  assert.equal(pointer('0'.repeat(64),0),'');
});

test('prepare preserves active Git metadata and every byte of the original assets', () => {
  const f = fixture();
  const beforeIndex = digest(path.join(f.root,'.git/index'));
  const beforeConfig = digest(path.join(f.root,'.git/config'));
  const beforeAsset = digest(path.join(f.root,'Content/Art/a.uasset'));
  const result = prepare(f.root,f.output,options);
  assert.equal(result.commits,5);
  assert.equal(git(f.root,['rev-parse','HEAD']),f.original);
  assert.equal(digest(path.join(f.root,'.git/index')),beforeIndex);
  assert.equal(digest(path.join(f.root,'.git/config')),beforeConfig);
  assert.equal(digest(path.join(f.root,'Content/Art/a.uasset')),beforeAsset);
  assert.equal(fs.existsSync(path.join(f.root,'.gitattributes')),false);
  const plan=JSON.parse(fs.readFileSync(path.join(f.output,'plan.json'),'utf8'));
  assert.equal(plan.lfsFiles.length,4);
  assert.equal(plan.originalFiles.length,7);
  assert.equal(verify(f.root,f.output).status,'verified-prepared');
});

test('publication resumes and a fresh clone hydrates LFS', t => {
  const f=fixture();
  const transport=git(f.root,['ls-remote','origin'],{allowFailure:true});
  if (typeof transport!=='string') {
    t.skip('Local Git transport is blocked in this Windows sandbox: '+transport.error.split('\n')[0]);
    return;
  }
  prepare(f.root,f.output,options);
  const beforeAsset=digest(path.join(f.root,'Content/Art/a.uasset'));
  const plan=JSON.parse(fs.readFileSync(path.join(f.output,'plan.json'),'utf8'));
  const gitDir=path.join(f.output,plan.gitDirectory);
  git(f.root,['push','--quiet','origin',plan.commits[0].sha+':refs/heads/master'],{gitDir});
  const installed=publish(f.root,f.output);
  assert.equal(installed.status,'published-and-installed');
  assert.equal(git(f.root,['rev-parse','HEAD']),plan.finalCommit);
  assert.equal(git(f.root,['rev-parse',installed.backupRef]),f.original);
  assert.equal(git(f.root,['ls-remote','origin','refs/heads/main']).split(/\s+/)[0],f.main);
  assert.equal(digest(path.join(f.root,'Content/Art/a.uasset')),beforeAsset);
  assert.equal(git(f.root,['status','--porcelain']),'');
  const clone=path.join(f.directory,'Clean Clone');
  git(f.root,['clone','--quiet','--no-checkout',f.remote,clone]);
  git(clone,['lfs','install','--local']);
  git(clone,['checkout','--quiet','master']);
  assert.equal(digest(path.join(clone,'Content/Art/a.uasset')),beforeAsset);
  assert.equal(fs.statSync(path.join(clone,'Content/Art/empty.uasset')).size,0);
  assert.equal(fs.readFileSync(path.join(clone,'Docs/upload.txt'),'utf8'),'Reviewed source upload documentation\n');
  assert.equal(git(clone,['status','--porcelain']),'');
  git(clone,['lfs','fsck','--objects','--pointers','HEAD']);
});

test('unreviewed work and changed files prevent history installation', () => {
  const f = fixture(), source=path.join(f.root,'Source/Game.cpp');
  const original=fs.readFileSync(source);
  fs.writeFileSync(source,'// new work\n');
  assert.throws(()=>prepare(f.root,f.output,options),/Unreviewed local changes/);
  assert.equal(fs.existsSync(f.output),false);
  fs.writeFileSync(source,original);
  prepare(f.root,f.output,options);
  fs.writeFileSync(source,'// work after preparation\n');
  assert.throws(()=>verify(f.root,f.output),/Working file changed/);
  assert.equal(git(f.root,['rev-parse','HEAD']),f.original);
  assert.equal(fs.readFileSync(source,'utf8'),'// work after preparation\n');
});

test('a second unpublished commit and local main rename are preserved on a separate source branch', () => {
  const f=fixture();
  fs.writeFileSync(path.join(f.root,'Source/NewWork.cpp'),'// second local commit\n');
  git(f.root,['add','Source/NewWork.cpp']);
  git(f.root,['commit','--quiet','-m','Additional unpublished work']);
  git(f.root,['branch','--move','main']);
  const latest=git(f.root,['rev-parse','HEAD']);
  fs.writeFileSync(path.join(f.root,'script.bat'),'Local user script; leave untouched.\n');
  prepare(f.root,f.output,options);
  const plan=JSON.parse(fs.readFileSync(path.join(f.output,'plan.json'),'utf8'));
  assert.equal(plan.originalCommit,latest);
  assert.equal(plan.originalHistory.length,2);
  assert.equal(plan.originalBranch,'main');
  assert.equal(plan.branch,'master');
  assert.equal(git(f.root,['rev-parse','HEAD']),latest);
  assert.equal(git(f.root,['symbolic-ref','--short','HEAD']),'main');
  assert.ok(plan.files.some(file=>file.path==='Source/NewWork.cpp'));
  assert.deepEqual(plan.untrackedPreserved,['script.bat']);
  assert.equal(fs.readFileSync(path.join(f.root,'script.bat'),'utf8'),'Local user script; leave untouched.\n');
});

test('resume accepts only a remote head from the prepared commit chain', () => {
  const commits=[{sha:'a'.repeat(40)},{sha:'b'.repeat(40)}];
  assert.equal(resumeIndex(commits,null),-1);
  assert.equal(resumeIndex(commits,commits[0].sha),0);
  assert.equal(resumeIndex(commits,commits[1].sha),1);
  assert.throws(()=>resumeIndex(commits,'c'.repeat(40)),/contains other work/);
});

test('official Git LFS integrity checks reject a corrupt prepared asset', () => {
  const f=fixture();
  prepare(f.root,f.output,options);
  const plan=JSON.parse(fs.readFileSync(path.join(f.output,'plan.json'),'utf8'));
  const asset=plan.lfsFiles.find(a=>a.bytes>0), oid=asset.sha256;
  const object=path.join(f.output,plan.gitDirectory,'lfs/objects',oid.slice(0,2),oid.slice(2,4),oid);
  fs.writeFileSync(object,Buffer.alloc(asset.bytes,0));
  assert.throws(()=>verify(f.root,f.output),/corrupt|expected OID|does not match|was not expected/i);
  assert.equal(git(f.root,['rev-parse','HEAD']),f.original);
});
