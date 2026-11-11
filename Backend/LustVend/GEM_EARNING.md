# Gem earning implementation

**Live activation verified:** after the user's Nakama restart, the HTTPS healthcheck returned 200 and PlayFab revision **19 is Live**. The updated game loaded real daily/weekly progress for the existing account. The three tutorial lessons saved sequentially and the final lesson awarded +30 gems, changing the authoritative wallet from 10 to 40. The tutorial-complete button is disabled afterward. Screenshot: `Saved/LustVendDeployment/tutorial-earned-live.png`. Paid-spin milestones and weekly limits pass the isolated authoritative-handler tests; the agent did not conduct live wagering to test them.

## Player rewards

| Activity | Gems | Limit |
|---|---:|---|
| First signed-in login each UTC day | 10 | Once/day |
| Complete 10 regular paid spins | 5 | Once/day |
| Complete 25 regular paid spins | 5 | Once/day |
| Complete 50 regular paid spins | 5 | Once/day |
| Complete all three milestones | 5 extra | Once/day |
| Complete five daily sets in one UTC week | 30 extra | Once/week |
| Finish the three-lesson vending tutorial | 30 | Once/account |

Sugar Lust and Neon Nights both contribute to the same three milestones. Any accepted bet size counts equally; wins, losses and wager amounts do not increase the gem award. Bonus picks and Neon heat respins do not count as regular paid spins. The milestones overlap: the 50th spin finishes the third milestone and daily set. Daily ceiling: 30 gems; weekly ceiling: 240 gems, plus the one-time tutorial reward. Weeks start Monday 00:00 UTC.

The existing five **coin** missions remain available; their coin payouts are unchanged. Gem missions are separate and automatic, without a claim button. A coin mission involving bet size never grants extra gems.

## Server ownership and transaction behavior

`vendActivityWrites` runs inside the existing Nakama authoritative spin transaction, alongside reel receipt and jackpot updates. The storage read set includes the player's gem account and a durable `played_<player>_<game>_<spinId>` marker. The write set commits the spin result, deduplication marker, progress, wallet credit and milestone ledger entries together. A storage failure rolls back all of them; a version conflict rereads and recomputes progress. Reusing an old spin ID cannot count again even after the slot's rolling receipt list expires.

Clients have no arbitrary-earning RPC and cannot submit gem amounts or mission progress. The trusted bridge still derives the PlayFab player ID from `currentPlayerId`. Milestone, daily-set and weekly ledger entries use stable source IDs and server UTC dates. Preserve play markers and reward ledger entries; retention must not allow old accepted spin IDs to count again. Estimate storage needs for permanent markers before large-scale rollout.

Tutorial progress is persisted in Nakama as steps 0–3. `LustVendTutorial` validates a sequential next step; steps 1 and 2 save progress, and step 3 atomically grants 30 gems with a unique `tutorial_v1` ledger record. Retrying any step is harmless. This is a short acknowledgment-based introduction to gems, selection/confirmation and server-approved collection, not a time-gated tutorial or a trust signal for other rewards.

`LustVendState` returns an `earning` object with daily spin progress, targets, set completion, weekly completed days, weekly bonus status and tutorial step. It continues to claim the once-daily 10-gem login reward. The lobby's **Earn Gems** button opens mission progress; the machine's **Earn Gems / Tutorial** button shows rules, live progress and the three lessons. Refresh or reopen these panels after play to fetch updated gem progress. Existing coin mission information remains in the lobby mission panel.

## Validation and activation

- `node Backend/NakamaRuntime/test_lustvend.js`: 19 tests pass, including real authoritative spin handlers for both games, mixed-game progress, milestone limits, daily/weekly rollover, permanent old-spin deduplication, weekly award once, sequential tutorial completion once, and failed spin transaction rollback.
- Unreal Editor Development build succeeded: `Saved/LustVend-earning-build.log`.
- Prior runtime preserved at `Saved/LustVendDeployment/before/earning-index.js`; prior CloudScript preserved at `Saved/LustVendDeployment/before/earning-cloudscript.js`. Existing PlayFab revision 18 remains the rollback revision.
- New Nakama runtime uploaded to `/opt/nakama/data/modules/index.js` (23,355 bytes), followed by a user-confirmed restart and successful live account verification.
- Matching CloudScript **revision 19 is Live**. Revision 18 remains available for rollback. Earlier staging proof is retained at `Saved/LustVendDeployment/gem-earning-revision19-staged.png`.

Before declaring live end-to-end verification complete: confirm the new `earning` fields load for an existing account, finish the three lessons once (+30), reopen/refresh and confirm no duplicate reward, check 10 accepted regular spins grant +5 once, and verify an interrupted request resumes safely. Weekly rollover and five-day rewards are tested with an isolated simulated server clock; do not change the production clock to test them.
