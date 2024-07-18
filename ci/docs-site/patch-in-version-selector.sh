#! /usr/bin/env bash

# Adapted from https://oleksandrkvl.github.io/2024/06/05/multi-version-doxygen.html#multi-version-docs
#
# This script patches the doxygen output so that it includes a
# version selector to be included in the cppTango documentation website.
# Example:
#
#   make -Cbuild doc
#   mv build/doc_html $CI_COMMIT_TAG
#   ci/docs-site/patch-in-version-selector.sh $CI_COMMIT_TAG

if [ $# -ne 1 ]; then
    echo "usage: $0 <dir>"
    exit 1
fi

dir=$1
version_selector_line='<script type="text/javascript" src="../version_selector_handler.js"></script>'

for f in $(find $dir -name "*.html" -type f); do
    # Add a line to include the script that loads the version selector
    sed -i "/<\/head>/i $version_selector_line" $f

    # Remove the contents of the project number, so that it can be replace by
    # the version selector
    sed -i 's/<span id="projectnumber">.*<\/span>/<span id="projectnumber"\/>/' $f
done
