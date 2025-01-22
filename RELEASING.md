## Release process

We are using timed releases with a new release every 6 months (2nd April/2nd
October).

To document the progress it is easiest to copy everything below into an issue.

If the automatic windows binary upload does not work, or you need to do it manually for other reasons, the
following snippet uploads all zip and msi files from the current folder:

```sh
#!/bin/sh

set -e

TOKEN= # adapt me
PROJECT_ID=24006041
PACKAGE_NAME=windows
PACKAGE_VERSION= # adapt me

for file in `ls *zip *msi`
do
  echo "Uploading $file"
  curl --header "PRIVATE-TOKEN: ${TOKEN}" --upload-file $file                \
       https://gitlab.com/api/v4/projects/${PROJECT_ID}/packages/generic/${PACKAGE_NAME}/${PACKAGE_VERSION}/$file
done
```

<!-- start of issue -->

### When the previous release branch is created:

- [ ] Raise the library version (`MAJOR_VERSION`/`MINOR_VERSION`/`PATCH_VERSION`
in CMakeLists.txt) on `main`
- [ ] Tag the commit where the library version is raised as `$major.$minor.$patch-dev`

### At the beginning of the 6 month release cycle:

- [ ] Appoint a release manager who is responsible for the release process
      and create the release issue with the contents of this document between
      the `start of issue`/`end of issue` comments
- [ ] Create a milestone with due date
- [ ] Fill the milestone with issues
- [ ] If the next release is a major release and will break API/ABI make the
      responsible CI job non-fatal, see fd8cbddf (.gitlab-ci.yml: Allow failures
      for the ABI-API check, 2023-05-25) for an example

### 6 weeks before the release date:

- [ ] Create an issue to update RELEASE_NOTES.md and assign it to someone else
- [ ] Announce (slack: `kernel`) that there are two weeks left before the first release candidate
- [ ] \(TSD maintainer\) Update TSD/distribution.properties to see if it works
      with cppTango
- [ ] \(TSD maintainer\) Create issues for all dependent subprojects and ask for an update

### 4 weeks before the release date:

- [ ] Determine if we have broken API/ABI compatibility for this release and
      make sure that the SO_VERSION is correct in the base CMakeLists.txt.
- [ ] Create a release branch named `release/$major.$minor.$patch` off `main`. From
      now on all merge requests for this release have to be done against this branch.
- [ ] Change the target branch of all MRs which should be part of this release to the new
      release branch
- [ ] Disable the `pages` job on the `main` branch

#### RC phase:

- [ ] Tag the repo (`$major.$minor.$patch-rc$X`), this automatically creates a
      release and uploads the windows packages
- [ ] \(TSD maintainer\) Update TSD to use the new tag, merge it and tag as well
- [ ] Advertise the new release candidate on the `tango-info` mailing list and slack (channel: `general`)

During the RC phase it is advisable to only merge non-intrusive MRs to avoid
the possibility of last-minute regressions.

Creating a new release candidate should be redone every week when more MRs have been merged.

### At the release date:

All steps from the release candidate phase above:

- [ ] Tag the release (`$major.$minor.$patch`)
- [ ] Update for TSD
- [ ] Advertise new release

and finally

- [ ] Delete release branch
- [ ] Revert the API/ABI non-fatal CI change if required
- [ ] Allow the `pages` job to run on `main` again

<!-- end of issue -->
