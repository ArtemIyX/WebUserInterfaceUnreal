# CefProtobuf

`CefProtobuf` is the Unreal module that exposes the bundled Protocol Buffers C++ runtime used by the Web UI / CEF messaging path.

We currently use [**Protocol Buffers v36.0-rc1**](https://github.com/protocolbuffers/protobuf/releases/tag/v36.0-rc1).

## Layout

- Headers are expected under:
  - `Plugins/WebUserInterface/Source/ThirdParty/Protobuf/include`
- Static libraries are expected under:
  - `Plugins/WebUserInterface/Source/ThirdParty/Protobuf/lib/Win64`
- Unreal module rules:
  - [CefProtobuf.Build.cs](C:/Users/xeuse/repos/ScpRiftborn/Plugins/WebUserInterface/Source/CefProtobuf/CefProtobuf.Build.cs)

## Important integration details

- `CefProtobuf` builds as `C++20`.
- We force:
  - `PROTOBUF_FORCE_EMPTY_STRING_DYNAMIC_INIT=1`
- Generated `.pb.cc` files are post-processed by [proto.bat](C:/Users/xeuse/repos/ScpRiftborn/proto.bat).
- `proto.bat` recursively generates all `.proto` files under:
  - `Source/ScpProtobuf/Public/Proto`

## Generated source patching

Unreal may define `verify` as a macro, which can break protobuf / Abseil generated code.

To avoid that, rewrite each generated `.pb.cc` so the self-include:

```cpp
#include "some_file.pb.h"
```

becomes:

```cpp
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4702)
#endif

#pragma push_macro("verify")
#undef verify
#include "some_file.pb.h"
#pragma pop_macro("verify")

#ifdef _MSC_VER
#pragma warning(pop)
#endif
```

This protects against:

- Unreal `verify` macro collisions
- MSVC `C4702` codegen warnings emitted from protobuf / Abseil template instantiations

## Rebuilding protobuf for a specific machine

Prebuilt protobuf / Abseil static libraries must match the local Unreal toolchain closely.

At minimum, match:

- compiler toolset
- MSVC runtime model
- architecture
- C++ standard expectations
- build flavor used by Unreal

If the machine-specific toolchain changes, rebuild protobuf instead of reusing old `.lib` files.

Configures protobuf build with:

- `Visual Studio 18 2026`
- `x64`
- `C++20`
- `protobuf_MSVC_STATIC_RUNTIME=OFF`
- dynamic MSVC runtime (`/MD`) for Development-style UE builds
- static protobuf libraries
- install output enabled

## Manual rebuild steps

Example source root:

```text
C:\protobuf-36.0-rc1
```

Recommended commands:

```powershell
$src = "C:\protobuf-36.0-rc1"
$build = "$src\build-ue-md"
$install = "$src\install-ue-md"

cmake -S $src -B $build `
  -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_INSTALL_PREFIX="$install" `
  -DCMAKE_CXX_STANDARD=20 `
  -Dprotobuf_MSVC_STATIC_RUNTIME=OFF `
  -Dprotobuf_BUILD_SHARED_LIBS=OFF `
  -Dprotobuf_BUILD_TESTS=OFF `
  -Dprotobuf_BUILD_EXAMPLES=OFF `
  -Dprotobuf_BUILD_PROTOBUF_BINARIES=ON `
  -Dprotobuf_INSTALL=ON `
  -Dprotobuf_ABSL_PROVIDER=module `
  -Dprotobuf_JSONCPP_PROVIDER=module `
  -Dprotobuf_UTF8_RANGE_PROVIDER=module

cmake --build $build --config Release --target install --parallel
```

## Updating bundled protobuf in the repo

After a successful rebuild, copy:

- `install-ue-md/include` -> `Plugins/WebUserInterface/Source/ThirdParty/Protobuf/include`
- `install-ue-md/lib` -> `Plugins/WebUserInterface/Source/ThirdParty/Protobuf/lib/Win64`

Then:

1. Regenerate `.pb.h` / `.pb.cc`
2. Rebuild the Unreal target
3. Verify there are no:
   - `verify` macro include failures
   - `C4702` generated-code failures
   - MSVC runtime mismatches like `/MT` vs `/MD`
   - unresolved Abseil / protobuf linker errors

## Notes

- Do not reuse an old protobuf build directory if the runtime mode changed.
- If you previously built with `protobuf_MSVC_STATIC_RUNTIME=ON`, create a fresh build and install directory before rebuilding with `OFF`.
- If link errors mention Abseil logging internals or runtime-library mismatches, suspect a toolchain / runtime mismatch before changing protobuf headers or generated `.proto` schema.
