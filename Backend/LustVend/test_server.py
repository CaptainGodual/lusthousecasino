import concurrent.futures
import contextlib
import json
from pathlib import Path
import sqlite3
import tempfile
import unittest
import uuid
import threading
import urllib.request
import urllib.error
from http.server import ThreadingHTTPServer
from server import Store, Rejected, VERSION, authenticate, handler

class Transactions(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(dir=Path(__file__).parent)
        self.store = Store(Path(self.tmp.name)/'test.sqlite3')
        self.store.earn('p', 'seed', 300, 'test')
    def tearDown(self):
        self.tmp.cleanup()
    def buy(self, sku='vid-chip', key=None):
        return self.store.purchase('p', key or str(uuid.uuid4()), sku, VERSION)
    def test_replay_survives_restart(self):
        key = str(uuid.uuid4())
        first = self.buy(key=key)
        other = Store(self.store.path)
        self.assertEqual(first, other.purchase('p', key, 'vid-chip', VERSION))
        self.assertEqual(240, other.snapshot('p')['balance'])
    def test_key_reuse_rejected(self):
        key = str(uuid.uuid4()); self.buy(key=key)
        with self.assertRaises(Rejected): self.buy('avatar-item', key)
    def test_concurrent_same_request(self):
        key = str(uuid.uuid4())
        with concurrent.futures.ThreadPoolExecutor(8) as pool:
            results = list(pool.map(lambda _: self.buy(key=key), range(16)))
        self.assertEqual(1, len({r['receipt'] for r in results}))
        self.assertEqual(240, self.store.snapshot('p')['balance'])
    def test_concurrent_distinct_no_overspend(self):
        def attempt(sku):
            try: return self.buy(sku)
            except Rejected: return None
        with concurrent.futures.ThreadPoolExecutor() as pool:
            results = list(pool.map(attempt, ['avatar-item','theme-pack']))
        self.assertEqual(1, sum(r is not None for r in results))
        self.assertGreaterEqual(self.store.snapshot('p')['balance'], 0)
    def test_rollback_injected_failure(self):
        with contextlib.closing(self.store.connect()) as db:
            db.execute("CREATE TRIGGER fail BEFORE INSERT ON inventory BEGIN SELECT RAISE(ABORT,'injected'); END")
        with self.assertRaises(sqlite3.IntegrityError): self.buy()
        snap = self.store.snapshot('p')
        self.assertEqual(300, snap['balance']); self.assertEqual([], snap['owned'])
        with contextlib.closing(self.store.connect()) as db:
            self.assertEqual(1, db.execute('SELECT count(*) FROM ledger').fetchone()[0])
            self.assertEqual(0, db.execute('SELECT count(*) FROM purchases').fetchone()[0])
    def test_earning_idempotent_and_conflict(self):
        self.assertEqual(300, self.store.earn('p','seed',300,'test'))
        with self.assertRaises(Rejected): self.store.earn('p','seed',301,'test')
    def test_price_and_catalog_owned_funds(self):
        with self.assertRaises(Rejected): self.buy('missing')
        with self.assertRaises(Rejected): self.buy('special-item')
        with self.assertRaises(Rejected): self.store.purchase('p',str(uuid.uuid4()),'vid-chip','stale')
        self.buy()
        with self.assertRaises(Rejected): self.buy()
    def test_collect_is_owned_and_replayable(self):
        r=self.buy()
        with self.assertRaises(Rejected): self.store.collect('attacker',r['receipt'])
        self.assertEqual(self.store.collect('p',r['receipt']),self.store.collect('p',r['receipt']))
        self.assertEqual([],self.store.snapshot('p')['pending'])
        self.assertEqual(['vid-chip'],self.store.snapshot('p')['owned'])
    def test_ledger_reconciles(self):
        self.buy(); self.buy('avatar-item')
        with contextlib.closing(self.store.connect()) as db:
            total=db.execute("SELECT sum(delta) FROM ledger WHERE player='p'").fetchone()[0]
        self.assertEqual(total,self.store.snapshot('p')['balance'])
    def test_demo_auth(self):
        self.assertEqual('demo-player',authenticate('local-demo',True))
        with self.assertRaises(Rejected): authenticate('other',True)

    def test_http_roundtrip_and_auth(self):
        host=ThreadingHTTPServer(('127.0.0.1',0),handler(self.store,True))
        thread=threading.Thread(target=host.serve_forever,daemon=True);thread.start()
        base=f'http://127.0.0.1:{host.server_port}'
        def call(route,body=None,token='local-demo'):
            req=urllib.request.Request(base+route,data=json.dumps(body).encode() if body is not None else None,
                headers={'Authorization':'Bearer '+token,'Content-Type':'application/json'})
            with urllib.request.urlopen(req,timeout=5) as response: return json.load(response)
        try:
            with self.assertRaises(urllib.error.HTTPError) as caught: call('/api/state',token='bad')
            self.assertEqual(401,caught.exception.code)
            first=call('/api/daily',{});again=call('/api/daily',{})
            self.assertEqual(first,again)
            self.store.earn('demo-player','http-seed',100,'test')
            body=dict(requestId=str(uuid.uuid4()),sku='vid-chip',version=VERSION,price=0,player='attacker')
            result=call('/api/purchase',body)
            self.assertEqual(60,result['price']);self.assertEqual(50,result['balance'])
            self.assertEqual(result,call('/api/purchase',body))
            self.assertEqual(1,len(call('/api/state')['pending']))
            call('/api/collect',dict(receipt=result['receipt']))
            self.assertEqual([],call('/api/state')['pending'])
            with urllib.request.urlopen(base+'/',timeout=5) as response:
                self.assertIn(b'LUSTVEND',response.read())
            with urllib.request.urlopen(base+'/assets/machine_0.png',timeout=5) as response:
                self.assertEqual(b'\x89PNG\r\n\x1a\n',response.read(8))
        finally:
            host.shutdown();host.server_close();thread.join()

if __name__ == '__main__': unittest.main()
