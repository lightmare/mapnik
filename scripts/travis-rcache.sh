#! /bin/bash
#
# Given the following values set by Travis:
#   TRAVIS_REPO_SLUG = lightmare/mapnik
#   TRAVIS_BRANCH = test-cache
#   TRAVIS_JOB_NUMBER = 89.3
#
# And these set in repository settings on Travis:
#   RCACHE_REMOTE = host::prefix
#   RCACHE_PASSWORD = puzzles
#
# rcache variables will default to:
#   RCACHE_MODULE = host::prefix:lightmare
#   RCACHE_USER = lightmare
#   RCACHE_REPO = mapnik
#   RCACHE_TAG = 3
#
# ccache directory will be rsynced with:
#   host::prefix:lightmare/mapnik/test-cache/3
#
# If RCACHE_PASSWORD is empty, cache won't be uploaded.
# If RCACHE_REMOTE is empty, RCACHE_MODULE must be supplied.
# RCACHE_TAG may be used to make several jobs share a cache.
#

: ${RCACHE_DIR:=$HOME/.rcache}
: ${RCACHE_TAG:=${TRAVIS_JOB_NUMBER##*.}}
: ${RCACHE_REPO:=${TRAVIS_REPO_SLUG#*/}}
: ${RCACHE_USER:=${TRAVIS_REPO_SLUG%%/*}}
: ${RCACHE_MODULE:=${RCACHE_REMOTE:+$RCACHE_REMOTE:$RCACHE_USER}}

rcache_download () {
    test -n "$RCACHE_MODULE" || return
    export CCACHE_COMPRESS=1
    export CCACHE_DIR="$RCACHE_DIR/$RCACHE_REPO/$TRAVIS_BRANCH/$RCACHE_TAG"

    # try to download cache for the current branch (or merge-base)
    rcache_download_branch "$TRAVIS_BRANCH" && return

    # if that fails, try to download cache for master
    test "$TRAVIS_BRANCH" != "master" &&
        rcache_download_branch "master" &&
        ( cd "$RCACHE_DIR/$RCACHE_REPO" && mv "master" "$TRAVIS_BRANCH" )
}

rcache_download_branch () {
    local branch="$1"
    shift
    mkdir -p "$RCACHE_DIR"

    # RCACHE_PASSWORD, being an encrypted environment variable, is not
    # available to pull requests from forks. That's why there is rsync
    # user "travis-ci" with un-encrypted password and read-only access.
    RSYNC_PASSWORD="ic+sivart" \
    rsync -auz "$@" --human-readable --stats \
        --relative --no-implied-dirs \
        "travis-ci@$RCACHE_MODULE/./$RCACHE_REPO/$branch/$RCACHE_TAG/" \
        "$RCACHE_DIR/"
}

rcache_upload () {
    test -n "$RCACHE_MODULE" || return
    test -n "$RCACHE_PASSWORD" || return

    RSYNC_PASSWORD="$RCACHE_PASSWORD" \
    rsync -auz "$@" --human-readable --stats \
        --relative --no-implied-dirs \
        --del --delete-excluded \
        --exclude="tmp" \
        --link-dest="$RCACHE_REPO/master/$RCACHE_TAG/" \
        "$RCACHE_DIR/./$RCACHE_REPO/$TRAVIS_BRANCH/$RCACHE_TAG/" \
        "$RCACHE_USER@$RCACHE_MODULE/"
}
