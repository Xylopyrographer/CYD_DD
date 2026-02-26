# Development Notes

Project learnings, workflows, and processes for maintaining and releasing **CYD DataDisplay**.

---

## Version Numbering

Two independent version numbers coexist in this project:

| Version | Where | Meaning |
|---|---|---|
| `v1.0.x` | Git tags / GitHub releases | **This refactor's** release tag |
| `1.4.x` | `FIRMWARE_VERSION` in `main.cpp` | Upstream OTA firmware version — **do not change** |

Never bump `FIRMWARE_VERSION` as part of a refactor release.

---

## Branch Strategy

```
main                   ← always production-ready; version marker commits land here
feature/<name>         ← work branch; branched off main
release/v1.0.x         ← cumulative history branch used for GitHub releases
```

---

## Feature Development Workflow

```bash
# 1. Branch off main
git checkout main && git pull
git checkout -b feature/<name>

# 2. Develop, commit frequently
git add <files> && git commit -m "<message>"

# 3. Update Release Notes.md on the feature branch (included in the squash)

# 4. Squash-merge back to main
git checkout main
git merge --squash feature/<name>
git commit -m "v1.0.X: <short description>"

# 5. Clean up feature branch (optional)
git branch -d feature/<name>
```

---

## Release Process

Each GitHub release uses a **cumulative release branch** built by cherry-picking
the individual feature commits on top of the previous release.

### Step 1 — Identify commits to cherry-pick

```bash
git log --oneline feature/<name>
# Note the individual commit SHAs (not the squash commit on main)
```

### Step 2 — Create the release branch from the tip of the previous release

```bash
git checkout -b release/v1.0.X remotes/origin/release/v1.0.<X-1>
```

### Step 3 — Cherry-pick the feature commits (in order)

```bash
git cherry-pick <sha1> <sha2> <sha3> ...
```

### Step 4 — Rebase onto origin/main

`origin/main` has a version marker commit the local branch doesn't have yet.
Rebase so the release history sits on top of it cleanly:

```bash
git fetch origin
git rebase --onto origin/main origin/release/v1.0.<X-1> release/v1.0.X
```

### Step 5 — Add the version marker commit

```bash
git commit --allow-empty -m "v1.0.X"
```

### Step 6 — Push the release branch, fast-forward main, push the tag

```bash
git push --force origin release/v1.0.X       # force needed after rebase
git push origin release/v1.0.X:main          # fast-forward origin/main
git tag v1.0.X <marker-commit-sha>
git push origin v1.0.X
```

### Step 7 — Sync local main

```bash
git checkout main && git reset --hard origin/main
```

### Step 8 — Create the GitHub release

```bash
gh release create v1.0.X \
    --title "v1.0.X" \
    --notes "<release notes text>" \
    --target release/v1.0.X
```

> The `--notes` text should match the corresponding entry in `Release Notes.md`.

---

## Device / Build Notes

| Item | Value |
|---|---|
| Device port | `/dev/cu.wchusbserial14330` |
| Normal build env | `release` |
| Build + flash + monitor | `pio run -e release -t upload -t monitor` |
| Full flash erase (stale NVS) | `pio run -e release -t erase && pio run -e release -t upload -t monitor` |

### When to erase flash

Erase flash when:
- NVS key layout has changed (e.g. new keys added, key names changed)
- Touch calibration NVS is suspected to be from a previous incompatible version
- Device is behaving unexpectedly after a change to `prefs` reads/writes

---

## NVS Key Reference

### Touch Calibration

| Key | Orientation | Default |
|---|---|---|
| `calXMin` | Normal | 200 |
| `calXMax` | Normal | 3900 |
| `calYMin` | Normal | 200 |
| `calYMax` | Normal | 3900 |
| `calXMinF` | Flipped | 3900 |
| `calXMaxF` | Flipped | 200 |
| `calYMinF` | Flipped | 3900 |
| `calYMaxF` | Flipped | 200 |

Min/max values encode axis direction — the touch loop uses unconditional `map()`.
`ts.setRotation()` is always 1; only `tft.setRotation()` changes on flip.

### Display

| Key | Type | Default | Notes |
|---|---|---|---|
| `dispFlip` | bool | false | Saved display orientation (NRM=false, FLP=true) |
| `invertCol` | bool | false | Invert display colours |

---

## Remote

```
origin  https://github.com/Xylopyrographer/CYD_DD.git
```
