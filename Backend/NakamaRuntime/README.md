# Authoritative slot economy runtime

index.js is the generated single-file ES5 Nakama runtime. It registers the server-to-server sugarlust_progressive gateway and now owns coin/XP settlement, feature state, missions, daily claims, permanent request receipts, reel results and shared progressive pools. LUSTVEND gem/ownership transactions share the existing private storage model; spin-earned gems commit in the same batch as the spin.

Build from index.core.js, economy-domain.js and economy-settlement.js with `node Tools/Backend/build-runtime.js` from the project root. Upload only index.js, not its three build inputs alongside it.

Read [SETTLEMENT_DEPLOYMENT.md](../SETTLEMENT_DEPLOYMENT.md) for the required maintenance, frozen-profile import, deployment order, tests and rollback restrictions. Do not deploy this runtime independently of the matching PlayFab bridge. Old raw Spin/NeonSpin RPC mutations fail closed. Existing pool and gem records remain intact; the new authoritative profile and permanent receipts live in private slot_economy rows.

The HTTP runtime key remains server-only. Use a valid HTTPS gateway, keep the Nakama administrative console private, and back up the entire database. No VPS credentials or service configuration are included here.
