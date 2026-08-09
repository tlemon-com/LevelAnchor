#pragma once

/*
    BuildInfo -- the version strings the About panel shows.

    Release builds: installer\build-release.bat writes BuildInfo.generated.h
    next to this file (gitignored) just before it calls MSBuild. It stamps the
    hand-edited semver from the repo-root VERSION file plus an automatic build
    ID, so two builds of the same version are still told apart.

    Any other build -- a plain Projucer/VS build, or a fresh clone that has
    never run the release script -- has no generated header and falls back to
    the "dev" values below. That fallback is why this file exists: the tree has
    to compile without the script.

    Note this is deliberately NOT JucePlugin_VersionString. That one comes from
    the .jucer and can only change by re-exporting from the Projucer, so it
    cannot carry a per-build number.
*/

#if __has_include("BuildInfo.generated.h")
  #include "BuildInfo.generated.h"
#endif

#ifndef LEVELANCHOR_VERSION
  #define LEVELANCHOR_VERSION  "dev"
  #define LEVELANCHOR_BUILD_ID "local"
#endif

// Composed at compile time (adjacent string literals), so painting it costs
// nothing: "v1.0.0  b260805.2" for a release, "vdev  blocal" otherwise.
#define LEVELANCHOR_VERSION_LABEL "v" LEVELANCHOR_VERSION "  b" LEVELANCHOR_BUILD_ID
