[![Travis Build Status](https://travis-ci.org/Dyalog/rscproxy.svg?branch=master)](https://travis-ci.org/Dyalog/rscproxy)
[![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/github/Dyalog/rscproxy?branch=master&svg=true)](https://ci.appveyor.com/project/JasonDyalog/rscproxy)

# rscproxy

This is a clone of the R package rscproxy, built and packaged for use with Dyalog APL's [R interface](http://docs.dyalog.com/17.0/R%20Interface%20Guide.pdf).

## Installation

1. Go to this repository's [releases](https://github.com/dyalog/rscproxy/releases/latest) page and choose an appropriate package:
   1. For Windows choose the Windows binary package `rscproxy*.zip`.
   2. For macOS choose the macOS binary package `rscproxy*.tgz`.
   3. For Linux and other operating systems choose the source package `rscproxy*.tar.gz`.
2. Right click on the package file name and copy its URL.
3. At the R command prompt type: `install.packages("\<URL>")`

## Information for developers

We use continuous integration: [AppVeyor](https://www.appveyor.com/) builds the source package and Windows binary package, and [Travis](https://travis-ci.org/) builds the macOS binary package. Every commit to `master` triggers a build, but only tagged builds are deployed to the [releases](https://github.com/dyalog/rscproxy/releases/latest) page.

The usual workflow for creating a new release is:
1. Use the github web UI to "Draft a new release". You can copy most of the information from the previous release. Increment the build number in the tag and the title. When you click "Publish release" the new git tag will be created. At first this new release will only have the standard "Source code" assets.
2. Travis and AppVeyor will spring into action. A few minutes later, if the builds succeed, they will add the packages they have built as new assets to the release created in step 1.
