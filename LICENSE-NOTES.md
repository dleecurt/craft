# License notes

CRAFT is under the **Artistic License** (the original 1.0 version; see
[`LICENSE`](LICENSE)). This file records the evidence and the older,
conflicting statements.

## 1. The author's 3.5 source page (primary source)

The "Source Code of CRAFT" page for version 3.5, from which
`craftcc35.tar.Z` was downloaded, ends with a "Finally" section that
gives the full Artistic License, clauses 1 to 10:

- http://borneo.gmd.de/AS/janus/craft/craftcc35.html
- archived copy:
  https://web.archive.org/web/20030324085855/http://borneo.gmd.de/AS/janus/craft/craftcc35.html

This is the author's own statement for the release this repository
is based on, so it is the one this fork follows.

## 2. Debian (agrees)

Debian packaged Craft 3.5 from 1999 (packaged by Falk Hueffner, from
`craftcc35.tar.gz`). Its copyright file (craft 3.5-9) says the
copyright belongs to Uwe Beyer and that the game may be redistributed
under the terms of the **Artistic License**.

- https://sources.debian.org/copyright/license/craft/3.5-9

## 3. Older wording in the source archive (superseded)

The `README` in the 3.5 source archive (kept as
[`doc/README.original`](doc/README.original)) and an older copy of the
source page ([`doc/craftcc.html`](doc/craftcc.html)) say instead:

> This source code is free, everybody may do everything with it, if it is
> not used commercially. ...

The published 3.5 page replaced this paragraph with the Artistic
License, so this wording appears to be older text left in the archive.

## 4. Wikis and project listings (unsupported)

- **libregamewiki** lists the code and media as **GPL**:
  https://libregamewiki.org/CRAFT:_The_Vicious_Vikings
- **SourceForge** (project `craft-vikings`) lists **GPLv2**:
  https://sourceforge.net/projects/craft-vikings/

No statement by the author putting Craft under the GPL has been found.

## How this fork meets the license

The Artistic License lets you distribute a modified version if every
changed file has a prominent notice of how and when it was changed, and
the changes are made freely available (clause 3), and if binaries come
with the source or instructions for getting it (clause 4). In this fork:

- every file changed from the 3.5 Standard Version starts with a
  "Changed from the CRAFT 3.5 Standard Version" notice that lists who
  changed it, when, and how, and points to its git history;
- all changes are public at https://github.com/dleecurt/craft;
- [`README.md`](README.md) lists the differences from the Standard
  Version, and the art tools install under `craft-` names that don't
  clash with standard programs;
- the author's name is not used to promote the fork (clause 9).

## Third-party images

Images whose rights are unclear are left out of this fork: a scanned
cartoon (`ccc.gif`) and a photo (`pic/hua.gif`) from the original web
pages, and the cartoon images on the author's other pages (the main
page, release notes and source page). The title-screen picture
(`hcraft/m1`, also `doc/pic/craft.gif`) appears to be a photo of Eilean
Donan castle in Scotland. Where it came from is unknown.
