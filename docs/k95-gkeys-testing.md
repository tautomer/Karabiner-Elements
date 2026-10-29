# Experimental Corsair K95 RGB G-key support

This branch is based on Karabiner-Elements **v16.3.0**. It is an experimental
self-build, not an official Karabiner release. The GitHub Actions workflow builds
on a hosted Mac, runs the test suite, and uploads a DMG plus a SHA-256 checksum.
No signing credentials are stored in the repository or used by the workflow.
The resulting package is **not signed with the official Developer ID or
notarized**. A green build does not prove macOS will allow it to run.

## What the patch does

- It recognizes only physical Corsair vendor `0x1b1c` / product `0x1b11`,
  and only the keyboard interface with the expected report-3 descriptor.
- It converts the G-key bitmap to 18 distinct internal, input-only key codes:
  `corsair_k95_g1` through `corsair_k95_g18`. These do not alias F-keys or mouse
  buttons. Unmapped G keys produce no ordinary output.
- It makes those input names available in **Simple Modifications**, in the
  “Corsair K95 RGB G keys” category, and in complex-modification JSON.
- Other K95 models and unknown report layouts are not handled. No keyboard
  initialization commands, RGB controls, or iCUE features are implemented.

G1–G3 report layouts were verified against real captures. G4–G18 positions
come from the ckb-next key map and have synthetic tests only. Real G-key
behavior, reconnects, and event grabbing remain untested on the patched build.

## Before installing any self-build

A CI artifact alone does not establish that the package is safe or usable on a
particular Mac. Review the CI status, artifact checksum, and signing state first.
The upstream `DEVELOPMENT.md` explains why replacing only one component of an
official installation can break inter-process communication. Switching signers
may require re-approving macOS Accessibility, Input Monitoring, and background
services. The build skips a team-identity check when there is no verified signing
team, so an ad-hoc/unsigned build has different security properties than the
upstream-signed release.

Before testing, keep a known-good official installer and a copy of the existing
Karabiner configuration, and have a separate working keyboard or other input
method available. This branch does not install itself or change the configuration.
**Do not disable macOS security protections to make it work.**

The physical K95 was previously ignored by Karabiner to avoid conflicts with
iCUE. The new handler runs in Karabiner's device-grabbing pipeline; using it may
require enabling **Modify events** for the physical K95. Test ordinary keys and
G-keys cautiously, with the iCUE virtual keyboard still excluded as appropriate.
A build or unit test cannot prove that the real device-grabbing conflict is fixed.

## Example complex-modification rule

After a compatible self-build is installed and the physical device is enabled,
this example maps G1 to F13, scoped to the original K95 RGB:

```json
{
  "description": "K95 G1 to F13",
  "manipulators": [
    {
      "type": "basic",
      "from": { "key_code": "corsair_k95_g1", "modifiers": { "optional": ["any"] } },
      "to": [ { "key_code": "f13" } ],
      "conditions": [
        { "type": "device_if", "identifiers": [ { "vendor_id": 6940, "product_id": 6929 } ] }
      ]
    }
  ]
}
```

This is an example, not an automatically installed mapping. Text expansion,
launching programs, and other assignments can be configured with standard
Karabiner complex-modification actions once the input is working.

## Technical details

The raw input report is 64 bytes, including report ID `03` at byte zero. G1–G8
use byte 16; G9–G10 use bits 0–1 of byte 17; G11–G18 use byte 18. Other bits
are ignored. The internal logical usage page is deliberately outside the 16-bit
physical HID-page range to avoid colliding with physical keyboard or mouse
inputs. It is consumed by Karabiner rules and is not posted to the virtual HID
output device as a real page.

The test suite covers the real descriptor, G1–G3 capture-derived reports,
18 synthetic key positions, malformed reports and descriptors, reset, distinct
JSON names, event-queue conversion, and rule matching. It is not a substitute for
on-device testing.

### Local sanitizer note

The repository's test configuration enables AddressSanitizer. In an additional
local experiment with UndefinedBehaviorSanitizer, an existing packed DriverKit
HID-report structure produced alignment warnings during the generic output tests.
The same warning was reproduced using the unmodified v16.3.0 source and the pinned
DriverKit submodule. This branch does not change that vendored code; the warning is
not evidence that the K95-specific decoder is responsible, nor is it proof that
the wider codebase is free of undefined behavior.
