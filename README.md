[![Gitlab (main)](https://img.shields.io/gitlab/pipeline-status/tango-controls/cppTango?branch=main&label=main)](https://gitlab.com/tango-controls/cppTango/-/pipelines?page=1&scope=branches&ref=main)
[![Gitlab (9.3-backports)](https://img.shields.io/gitlab/pipeline-status/tango-controls/cppTango?branch=9.3-backports&label=9.3-backports)](https://gitlab.com/tango-controls/cppTango/-/pipelines?page=1&scope=branches&ref=9.3-backports)
[![AppVeyor (9.3-backports)](https://img.shields.io/appveyor/build/bourtemb/cpptango-nh0mp/9.3-backports?label=9.3-backports%20%28Windows%29)](https://ci.appveyor.com/project/bourtemb/cpptango-nh0mp/branch/9.3-backports)
[![GitLab tag (latest by date)](https://img.shields.io/gitlab/v/tag/tango-controls/cppTango?sort=date)](https://gitlab.com/tango-controls/cppTango/-/releases)
[![Conda](https://img.shields.io/conda/v/conda-forge/cppTango)](https://anaconda.org/conda-forge/cpptango)
[![Gitlab code coverage](https://img.shields.io/gitlab/coverage/tango-controls/cppTango/main)](https://gitlab.com/tango-controls/cppTango/-/pipelines?page=1&scope=branches&ref=main)
[![CLion](https://img.shields.io/badge/-Developed%20using%20CLion-blue.svg?style=flat-square&logo=clion)](https://www.jetbrains.com/clion)
[![Neovim](https://img.shields.io/badge/-Developed%20using%20Neovim.svg?style=flat-square&logo=neovim)](https://neovim.io)
[![License](https://img.shields.io/badge/license-LGPL--3.0-orange.svg)](https://gitlab.com/tango-controls/cppTango/-/blob/main/LICENSE)

[![logo](http://www.tango-controls.org/static/tango/img/logo_tangocontrols.png)](http://www.tango-controls.org)

# Distributed Control System - C++ library

TANGO is an object-oriented distributed control system. In TANGO all objects
are representations of devices, which can be on the same computer or
distributed over a network. Communication between devices can be synchronous,
asynchronous or event driven.

The object model in TANGO supports methods, attributes and properties. TANGO
provides an API which hides all the details of network access and provides
object browsing, discovery, and security features.

For more information please visit [www.tango-controls.org](http://www.tango-controls.org).

# Important branches

- main: Tango development version
- 9.3-backports: Tango 9.3 stable version
- tango-v10: Future Tango version without CORBA (not yet functional)

# Documentation

On readthedocs: [tango-controls.readthedocs.io](http://tango-controls.readthedocs.io)

API reference: [tango-controls.gitlab.io/cppTango](https://tango-controls.gitlab.io/cppTango)

Old Manuals: [tango-controls.org/documentation/kernel](http://www.tango-controls.org/documentation/kernel/)

# How to build and install

See [INSTALL file](INSTALL.md).

# How to contribute

See [CONTRIBUTING file](CONTRIBUTING.md).
