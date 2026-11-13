#!/bin/bash
set -e

REMOTE="origin"
BRANCH="main"
COMMIT_LIMIT=50000   # commits per push

# Total commits
TOTAL_COMMITS=$(git rev-list --count HEAD)
CHUNK_COUNT=$(( (TOTAL_COMMITS + COMMIT_LIMIT - 1) / COMMIT_LIMIT ))

echo "Total commits: $TOTAL_COMMITS"
echo "Commits per push: $COMMIT_LIMIT"
echo "Number of chunks: $CHUNK_COUNT"

for ((i=0; i<CHUNK_COUNT; i++)); do
  START=$(( i * COMMIT_LIMIT + 1 ))
  END=$(( START + COMMIT_LIMIT - 1 ))
  [ "$END" -gt "$TOTAL_COMMITS" ] && END=$TOTAL_COMMITS

  TMP_BRANCH="temp-chunk-$i"
  git branch -D "$TMP_BRANCH" 2>/dev/null || true
  git branch "$TMP_BRANCH" "$(git rev-list HEAD --reverse | sed -n "${END}p")"

  echo "Pushing $TMP_BRANCH to $REMOTE:$BRANCH..."
  git push "$REMOTE" "$TMP_BRANCH:$BRANCH" --force

  git branch -D "$TMP_BRANCH"
done

echo "All chunks pushed successfully to $REMOTE:$BRANCH."
