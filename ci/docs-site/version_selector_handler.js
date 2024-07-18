// Adapted from https://oleksandrkvl.github.io/2024/06/05/multi-version-doxygen.html#multi-version-docs

$(function () {
    // We assume that doxygen has generated the site with a single depth of html
    // files, so that the version_selector.html is in the directory above this
    // one.  The ci/docs-site/patch-in-version-selector.sh script already
    // assumes that this file is going to be at
    // '../version_selector_handler.js', so this is not an additional
    // constraint.
    var splitPath = window.location.pathname.split('/');
    var currentVersion = splitPath[splitPath.length - 2];
    $.get('../version_selector.html', function (data) {
        // Inject version selector HTML into the page
        $('#projectnumber').html(data);

        // Event listener to handle version selection
        document.getElementById('versionSelector').addEventListener('change', function () {
            var selectedVersion = this.value;
            // TODO: It would be nice to stay in the same relative location here
            // and only go to index.html on a 404
            window.location.href = '../' + selectedVersion + '/index.html';
        });

        // Set the selected option based on the current version
        $('#versionSelector').val(currentVersion);
    });
});
