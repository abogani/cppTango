#! /usr/bin/env bash

# Adapted from https://oleksandrkvl.github.io/2024/06/05/multi-version-doxygen.html#multi-version-docs

# This script will generate the cppTango documentation website from all the
# versioned docs tarballs in the cppTango package registry and, optionally, from
# build/doc_html.
#
# It is expecting to be run from the cppTango git root directory.
#
# To view the site run the following:
#
#   ci/docs-site/build.sh
#   python -m http.server
#
# Then navigate to 0.0.0.0:8000/site in your browser.
#
# Note, you must run `python -m http.server` from the cppTango
# root directory as the web site is expecting to be hosted at
# <hostname>/<some_dir> to match the gitlab pages address.

set -e

CI_API_V4_URL="${CI_API_V4_URL:-https://gitlab.com/api/v4}"
CI_PROJECT_ID="${CI_PROJECT_ID:-24006041}"

docs_packages=$(curl -s "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages?package_name=doc" | jq 'sort_by(.version|split(".")|map(tonumber))|reverse')

rm -rf site
rm -rf build/doc-site

mkdir -p build/doc-site
mkdir site

cp ci/docs-site/version_selector_handler.js site

echo '&nbsp;<select id="versionSelector">' > site/version_selector.html

if [ -e build/doc_html ]; then
    git_tag=$(git describe --tags --abbrev=0 --exclude=*test* --always HEAD)
    git_desc=$(git describe --tags --exclude=*test* --always HEAD)
    echo "Preparing documentation for HEAD (${git_desc})"
    cp -r build/doc_html site/${git_tag}
    ci/docs-site/patch-in-version-selector.sh site/${git_tag}
    echo "  <option value=\"${git_tag}\">${git_desc}</option>" >> site/version_selector.html
fi

echo $docs_packages | jq -r '.[].version' | while read -r version; do
  echo "Preparing documentation for tag $version"
  curl -o build/doc-site/${version}.tar.bz2 ${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/docs/${version}/docs.tar.bz2
  tar xjf build/doc-site/${version}.tar.bz2 -C site
  echo "  <option value=\"${version}\">${version}</option>" >> site/version_selector.html
done

echo '</select>' >> site/version_selector.html

latest=$(echo ${docs_packages} | jq '.[0].version' | tr -d '"')

cat << EOF > site/index.html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="refresh" content="0; url=${latest}/index.html">
    <title>Redirecting...</title>
</head>
<body>
    <p>If you are not redirected automatically, <a href="${latest}/index.html">click here</a>.</p>
</body>
</html>
EOF
