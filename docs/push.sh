#!/bin/sh
# Build the site and push it to GitHub pages.
set -e

root=$(git rev-parse --show-toplevel)
cd "$root"

if ! command -v frum >/dev/null ; then
  echo >&2 "Error: frum is not installed"
  exit 1
fi

if ! git diff-index --quiet --cached HEAD -- docs ; then
  echo >&2 "Error: uncommitted changes"
  exit 1
fi
if ! git diff-files --quiet docs ; then
  echo >&2 "Error: uncommitted changes"
  exit 1
fi
branch=$(git symbolic-ref HEAD)
if test "$branch" != refs/heads/main ; then
  echo >&2 "Error: branch is not main"
  exit 1
fi
commit="$(git rev-parse HEAD)"

echo >&2 "Checking out gh-pages..."
if test -d gh-pages ; then
  branch=$(git -C gh-pages symbolic-ref HEAD)
  if test "$branch" != refs/heads/gh-pages ; then
    echo >&2 "Error: gh-pages dir does not have gh-pages branch"
    exit 1
  fi
else
  git worktree add gh-pages
fi

echo >&2 "Building site..."
(
  cd docs
  eval "$(sh -c 'frum init')"
  bundle exec jekyll build --destination ../gh-pages
)

echo >&2 "Committing..."
cd gh-pages
git add .
git commit -m "Generated from commit ${commit}"
