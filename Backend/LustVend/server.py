"""LUSTVEND single-node reference service. Python 3.11+, no dependencies."""
import argparse
import contextlib
import datetime as dt
import hmac
import json
import os
from pathlib import Path
import re
import sqlite3
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import urllib.request
import uuid

ROOT = Path(__file__).parent
CATALOG = [
    dict(sku='vid-chip', name='Vid Chip', price=60, sprite=0),
    dict(sku='holo-disk', name='Holo Disk', price=90, sprite=1),
    dict(sku='pinup-pack', name='Pinup Pack', price=120, sprite=3),
    dict(sku='avatar-item', name='Avatar Item', price=180, sprite=4),
    dict(sku='theme-pack', name='Theme Pack', price=300, sprite=5),
    dict(sku='special-item', name='Special Item', price=600, sprite=10),
]
ITEMS = {p['sku']: p for p in CATALOG}
VERSION = 'lustvend-1'

class Rejected(Exception):
    def __init__(self, code, status=409):
        self.code, self.status = code, status

class Store:
    def __init__(self, path):
        self.path = str(path)
        with contextlib.closing(self.connect()) as db:
            db.executescript('''
            PRAGMA journal_mode=WAL;
            CREATE TABLE IF NOT EXISTS wallets(player TEXT PRIMARY KEY, gems INTEGER NOT NULL CHECK(gems>=0));
            CREATE TABLE IF NOT EXISTS ledger(id TEXT PRIMARY KEY, player TEXT NOT NULL, delta INTEGER NOT NULL,
                reason TEXT NOT NULL, source TEXT NOT NULL, at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(player,source));
            CREATE TABLE IF NOT EXISTS purchases(player TEXT NOT NULL, request TEXT NOT NULL, sku TEXT NOT NULL,
                version TEXT NOT NULL, receipt TEXT NOT NULL UNIQUE, price INTEGER NOT NULL, balance INTEGER NOT NULL,
                collected INTEGER NOT NULL DEFAULT 0, PRIMARY KEY(player,request));
            CREATE TABLE IF NOT EXISTS inventory(player TEXT NOT NULL, sku TEXT NOT NULL, receipt TEXT NOT NULL UNIQUE,
                PRIMARY KEY(player,sku));
            ''')

    def connect(self):
        db = sqlite3.connect(self.path, timeout=10, isolation_level=None)
        db.row_factory = sqlite3.Row
        return db

    @contextlib.contextmanager
    def transaction(self):
        with contextlib.closing(self.connect()) as db:
            db.execute('BEGIN IMMEDIATE')
            try:
                yield db
                db.commit()
            except BaseException:
                db.rollback()
                raise

    @staticmethod
    def wallet(db, player):
        db.execute('INSERT OR IGNORE INTO wallets VALUES (?,0)', (player,))
        return db.execute('SELECT gems FROM wallets WHERE player=?', (player,)).fetchone()[0]

    def snapshot(self, player):
        with self.transaction() as db:
            balance = self.wallet(db, player)
            return dict(player=player, balance=balance, catalog=CATALOG, version=VERSION,
                owned=[r[0] for r in db.execute('SELECT sku FROM inventory WHERE player=?', (player,))],
                pending=[dict(r) for r in db.execute('SELECT receipt,sku FROM purchases WHERE player=? AND collected=0', (player,))])

    def earn(self, player, source, amount, reason):
        # Trusted server/CLI only. Never accept arbitrary client earning events.
        if not isinstance(amount, int) or not 0 < amount <= 1000 or not source:
            raise Rejected('invalid_earning_event', 400)
        with self.transaction() as db:
            balance = self.wallet(db, player)
            old = db.execute('SELECT delta,reason FROM ledger WHERE player=? AND source=?', (player, source)).fetchone()
            if old:
                if (old['delta'], old['reason']) != (amount, reason):
                    raise Rejected('earning_event_conflict')
                return balance
            db.execute('UPDATE wallets SET gems=gems+? WHERE player=?', (amount, player))
            db.execute('INSERT INTO ledger(id,player,delta,reason,source) VALUES (?,?,?,?,?)',
                (str(uuid.uuid4()), player, amount, reason, source))
            return balance + amount

    def purchase(self, player, request, sku, version):
        if not isinstance(request, str) or not re.fullmatch(r'[a-zA-Z0-9-]{16,80}', request):
            raise Rejected('invalid_request_id', 400)
        with self.transaction() as db:
            old = db.execute('SELECT * FROM purchases WHERE player=? AND request=?', (player, request)).fetchone()
            if old:
                if old['sku'] != sku or old['version'] != version:
                    raise Rejected('idempotency_conflict')
                return dict(old)
            if version != VERSION:
                raise Rejected('catalog_changed')
            if sku not in ITEMS:
                raise Rejected('unknown_product', 400)
            balance = self.wallet(db, player)
            if db.execute('SELECT 1 FROM inventory WHERE player=? AND sku=?', (player, sku)).fetchone():
                raise Rejected('already_owned')
            price = ITEMS[sku]['price']
            if balance < price:
                raise Rejected('insufficient_gems')
            receipt = str(uuid.uuid4())
            db.execute('UPDATE wallets SET gems=gems-? WHERE player=? AND gems>=?', (price, player, price))
            db.execute('INSERT INTO ledger(id,player,delta,reason,source) VALUES (?,?,?,?,?)',
                (receipt, player, -price, 'vending_purchase', 'purchase:' + request))
            db.execute('INSERT INTO inventory VALUES (?,?,?)', (player, sku, receipt))
            db.execute('INSERT INTO purchases(player,request,sku,version,receipt,price,balance) VALUES (?,?,?,?,?,?,?)',
                (player, request, sku, version, receipt, price, balance-price))
            return dict(db.execute('SELECT * FROM purchases WHERE receipt=?', (receipt,)).fetchone())

    def collect(self, player, receipt):
        with self.transaction() as db:
            row = db.execute('SELECT * FROM purchases WHERE player=? AND receipt=?', (player, receipt)).fetchone()
            if not row:
                raise Rejected('receipt_not_found', 404)
            db.execute('UPDATE purchases SET collected=1 WHERE receipt=?', (receipt,))
            return dict(receipt=receipt, collected=True, sku=row['sku'])

def authenticate(ticket, demo):
    if demo:
        if not hmac.compare_digest(ticket, 'local-demo'):
            raise Rejected('unauthorized', 401)
        return 'demo-player'
    title, secret = os.environ['PLAYFAB_TITLE_ID'], os.environ['PLAYFAB_SECRET_KEY']
    if not ticket or len(ticket) > 8192:
        raise Rejected('unauthorized', 401)
    req = urllib.request.Request(f'https://{title}.playfabapi.com/Server/AuthenticateSessionTicket',
        data=json.dumps(dict(SessionTicket=ticket)).encode(),
        headers={'Content-Type':'application/json', 'X-SecretKey':secret})
    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            result = json.load(response)['data']
        if result.get('IsSessionTicketExpired'):
            raise ValueError('expired')
        return result['UserInfo']['PlayFabId']
    except Exception:
        raise Rejected('unauthorized', 401) from None

def handler(store, demo):
    class Handler(SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=str(ROOT / 'web'), **kwargs)

        def reply(self, body, status=200):
            raw = json.dumps(body).encode()
            self.send_response(status)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Cache-Control', 'no-store')
            self.send_header('Content-Length', str(len(raw)))
            self.end_headers()
            self.wfile.write(raw)

        def do_GET(self):
            if self.path != '/api/state':
                return super().do_GET()
            self.api(False)

        def do_POST(self):
            self.api(True)

        def api(self, post):
            try:
                ticket = self.headers.get('Authorization', '').removeprefix('Bearer ')
                player = authenticate(ticket, demo)
                if not post:
                    return self.reply(store.snapshot(player))
                length = int(self.headers.get('Content-Length', '0'))
                if not 0 < length <= 4096:
                    raise Rejected('invalid_body', 400)
                body = json.loads(self.rfile.read(length))
                if not isinstance(body, dict):
                    raise Rejected('invalid_body', 400)
                if self.path == '/api/purchase':
                    result = store.purchase(player, body.get('requestId'), body.get('sku'), body.get('version'))
                elif self.path == '/api/collect':
                    result = store.collect(player, body.get('receipt'))
                elif self.path == '/api/daily':
                    # Login reward is server-clock gated; no client progress claims.
                    day = dt.datetime.now(dt.timezone.utc).date().isoformat()
                    result = dict(balance=store.earn(player, 'daily:' + day, 10, 'daily_login'))
                else:
                    raise Rejected('not_found', 404)
                self.reply(result)
            except Rejected as err:
                self.reply(dict(error=err.code), err.status)
            except (ValueError, TypeError, sqlite3.IntegrityError):
                self.reply(dict(error='invalid_request'), 400)
            except Exception:
                self.reply(dict(error='service_unavailable_retry_same_request'), 503)

        def log_message(self, format, *args):
            pass  # Never log tickets, POST bodies, or query parameters.
    return Handler

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--demo', action='store_true')
    parser.add_argument('--db', default=str(ROOT / 'lustvend.sqlite3'))
    parser.add_argument('--port', type=int, default=8787)
    args = parser.parse_args()
    if not args.demo and not all(os.environ.get(k) for k in ('PLAYFAB_TITLE_ID', 'PLAYFAB_SECRET_KEY')):
        parser.error('Set PLAYFAB_TITLE_ID and PLAYFAB_SECRET_KEY on the server, or use --demo locally.')
    store = Store(args.db)
    if args.demo:
        store.earn('demo-player', 'demo-seed-v1', 420, 'demo_seed')
    print(f'LUSTVEND {"LOCAL DEMO" if args.demo else "PlayFab authenticated"}: http://127.0.0.1:{args.port}')
    ThreadingHTTPServer(('127.0.0.1', args.port), handler(store, args.demo)).serve_forever()
