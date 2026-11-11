'use strict';
const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');
const STATUSES = new Set(['working', 'incomplete', 'planned', 'blocked']);
const COLUMNS = new Set(['Backlog', 'Ready', 'In Progress', 'Verify', 'Done']);
const CORE_CHECKS = ['project-config', 'git-head', 'runtime-bundle', 'settlement', 'free-spins', 'mega-pot', 'setup-workflow', 'lustvend', 'release-policy', 'editor-build', 'editor-automation', 'windows-package'];

function validateCatalog(catalog) {
  if (catalog.schemaVersion !== 1 || !Array.isArray(catalog.systems) || !catalog.systems.length) throw Error('Nonempty version-1 system inventory required.');
  const ids = new Set();
  for (const s of catalog.systems) {
    if (!/^REL-\d{2}$/.test(s.id) || ids.has(s.id)) throw Error('Duplicate or invalid system ID: ' + s.id);
    ids.add(s.id);
    if (!STATUSES.has(s.status) || !COLUMNS.has(s.column)) throw Error('Invalid status/column: ' + s.id);
    for (const field of ['name', 'priority', 'scope', 'verifiedScope', 'blocker', 'nextAction', 'acceptance']) if (typeof s[field] !== 'string' || !s[field].trim()) throw Error('Missing ' + field + ': ' + s.id);
    for (const field of ['sources', 'checks', 'dependsOn', 'plan', 'signoffs']) if (!Array.isArray(s[field])) throw Error('Missing array ' + field + ': ' + s.id);
    if (!s.sources.length || !s.plan.length || !s.signoffs.length) throw Error('Evidence, plan and acceptance record required: ' + s.id);
    if (s.column === 'Done' && s.status !== 'working') throw Error('Unfinished system cannot be Done: ' + s.id);
  }
  const byId = new Map(catalog.systems.map(s => [s.id, s]));
  const visiting = new Set(), visited = new Set();
  function visit(id) {
    if (!byId.has(id)) throw Error('Unknown dependency: ' + id);
    if (visiting.has(id)) throw Error('Cyclic dependency: ' + id);
    if (visited.has(id)) return;
    visiting.add(id);
    for (const dep of byId.get(id).dependsOn) visit(dep);
    visiting.delete(id); visited.add(id);
  }
  for (const id of ids) visit(id);
  return catalog;
}

function fingerprint(root) {
  const files = [];
  function walk(relative, includeAll = false) {
    const dir = path.join(root, relative);
    if (!fs.existsSync(dir)) return;
    for (const entry of fs.readdirSync(dir, {withFileTypes:true})) {
      const name = path.join(relative, entry.name);
      if (entry.isDirectory()) walk(name, includeAll);
      else if ((includeAll || /\.(cpp|h|cs|js|ini)$/.test(name)) && !/^User.*\.ini$/i.test(entry.name)) files.push(name);
    }
  }
  for (const dir of ['Source', 'Backend/NakamaRuntime', 'Backend/PlayFabCloudScript', 'Config']) walk(dir);
  for (const dir of ['Content', 'Media']) walk(dir, true);
  files.push('SugarLustUE.uproject');
  const hash = crypto.createHash('sha256');
  const buffer = Buffer.allocUnsafe(1024 * 1024);
  for (const file of files.sort()) {
    hash.update(file.replaceAll('\\', '/') + '\0');
    const fd=fs.openSync(path.join(root,file),'r');
    try { let bytes; while((bytes=fs.readSync(fd,buffer,0,buffer.length,null))>0) hash.update(buffer.subarray(0,bytes)); }
    finally {fs.closeSync(fd);}
    hash.update('\0');
  }
  return hash.digest('hex');
}

function inspectAutomation(report, expectedNames) {
  // A launched editor returning zero without this report is NOT a test pass.
  if (!report || !Array.isArray(report.tests) || !report.tests.length) return {status:'blocked', summary:'No nonempty Unreal automation report was produced.'};
  const tests = report.tests;
  const names = new Map(tests.map(t => [t.fullTestPath || t.testDisplayName, t]));
  const missing = expectedNames.filter(n => !names.has(n));
  const bad = tests.filter(t => !['Success', 'SuccessWithWarnings'].includes(t.state));
  if (missing.length || bad.length || Number(report.failed || 0) > 0 || Number(report.notRun || 0) > 0) return {status:'failed', summary:`Automation incomplete: ${bad.length} unsuccessful, ${missing.length} missing expected tests.`, missing};
  return {status:'passed', summary:`${tests.length} Unreal tests executed successfully.`, testNames:[...names.keys()]};
}

function evaluate(catalog, checks, manual, currentFingerprint, options = {}) {
  validateCatalog(catalog);
  const scope = options.scope || 'all';
  const now = options.now || Date.now();
  const maxAge = options.maxAgeMs || 24 * 60 * 60 * 1000;
  const byCheck = new Map(checks.map(c => [c.id, c]));
  const records = new Map((manual?.records || []).map(r => [r.id, r]));
  const blocked = [];
  for (const id of CORE_CHECKS) {
    const c = byCheck.get(id);
    if (!c || c.status !== 'passed') blocked.push(`${id}: ${c?.summary || 'not verified'}`);
    else if (c.sourceFingerprint !== currentFingerprint) blocked.push(`${id}: evidence belongs to different source`);
    else if (!Number.isFinite(Date.parse(c.checkedAt)) || now - Date.parse(c.checkedAt) > maxAge || Date.parse(c.checkedAt) > now + 60000) blocked.push(`${id}: evidence is stale or has an invalid timestamp`);
  }
  const artifactFingerprint = byCheck.get('windows-package')?.artifactFingerprint;
  const systems = catalog.systems.filter(s => scope === 'all' || s.scope === 'checkpoint').map(s => {
    const reasons = [];
    if (s.status !== 'working') reasons.push(`System is ${s.status}`);
    for (const id of s.checks) {
      const c = byCheck.get(id);
      if (c?.required === false) continue;
      if (!c || c.status !== 'passed' || c.sourceFingerprint !== currentFingerprint) reasons.push(`Check ${id} is not current and passed`);
    }
    for (const id of s.signoffs) {
      const r = records.get(id);
      if (!r || r.status !== 'passed' || !r.tester || !r.evidence?.length) { reasons.push(`Acceptance ${id} has no completed evidence`); continue; }
      const at = Date.parse(r.checkedAt);
      if (!Number.isFinite(at) || now - at > maxAge || at > now + 60000) reasons.push(`Acceptance ${id} is stale or has an invalid timestamp`);
      if (!artifactFingerprint || r.artifactFingerprint !== artifactFingerprint || manual.sourceFingerprint !== currentFingerprint) reasons.push(`Acceptance ${id} does not match the current source and package`);
      if (options.evidenceExists && !r.evidence.every(options.evidenceExists)) reasons.push(`Acceptance ${id} references missing or empty evidence`);
    }
    return {id:s.id, ready:reasons.length === 0, reasons};
  });
  return {ready:blocked.length === 0 && systems.every(s => s.ready), scope, sourceFingerprint:currentFingerprint, checkBlockers:blocked, systems};
}

module.exports = {validateCatalog, fingerprint, inspectAutomation, evaluate, CORE_CHECKS};
