#!/bin/sh
# Run once on the Pi: sh init-repo.sh [path].  Needs git and OpenSSH >= 8.2 (ssh-keygen -Y).
# Bare repo, fast-forward only, no deletes, signature check on every push.  No forge, no hooks beyond this.
set -eu
repo=${1:-$HOME/aios.git}
here=$(cd "$(dirname "$0")" && pwd)
git init --bare --initial-branch=main "$repo"
git -C "$repo" config receive.denyNonFastForwards true
git -C "$repo" config receive.denyDeletes true
git -C "$repo" config receive.fsckObjects true
install -m 755 "$here/hooks/pre-receive" "$repo/hooks/pre-receive"
install -m 644 "$here/allowed_signers.seed" "$repo/allowed_signers.seed"
echo "bare repo at $repo.  On the Windows box: git remote add pi $(whoami)@$(hostname):${repo#$HOME/} && git push pi main"
