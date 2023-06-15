#!/usr/bin/env bash
# vim: syntax=sh

if ! hash docker 2>/dev/null; then
  echo "Can not run the tests as docker is missing"
  exit 1
fi

mysql_container="${1:-mysql_db}"
tango_container="${2:-tango_cs}"

docker run \
    --name "$mysql_container" \
    -e MYSQL_ROOT_PASSWORD=root \
    -e MYSQL_INITDB_SKIP_TZINFO=1 \
    -d \
    registry.gitlab.com/tango-controls/docker/mysql:5.16-mysql-5 \
    --sql-mode="" \
    --innodb=OFF \
    --default-storage-engine=MyISAM \
    > /dev/null

docker run \
    --name "$tango_container" \
    -e TANGO_HOST=127.0.0.1:10000 \
    -e MYSQL_HOST=mysql_db:3306 \
    -e MYSQL_USER=tango \
    -e MYSQL_PASSWORD=tango \
    -e MYSQL_DATABASE=tango \
    --link "$mysql_container":mysql_db \
    -d \
    registry.gitlab.com/tango-controls/docker/tango-db:5.16 \
    > /dev/null

tango_ipaddr="$(docker inspect \
    -f '{{ .NetworkSettings.IPAddress }}' \
    "$tango_container")"

export TANGO_HOST="${tango_ipaddr}:10000"

if ! "@CMAKE_CURRENT_BINARY_DIR@/tango_admin.sh" --ping-database 30; then
    echo "Failed to ping database"
    exit 1
fi
