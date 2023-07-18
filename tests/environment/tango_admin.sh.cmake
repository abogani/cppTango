#!/usr/bin/env bash
# vim: syntax=sh

exec docker run --rm \
    --read-only \
    --network=host \
    -e TANGO_HOST="$TANGO_HOST" \
    --entrypoint /tango/bin/tango_admin \
    registry.gitlab.com/tango-controls/docker/tango-db:5.16-1 "$@"
