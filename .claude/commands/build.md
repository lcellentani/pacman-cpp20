---
model: claude-haiku-4-5-20251001
---
Run: cmake --build --preset clang-ninja-debug
If the Clang/Ninja build fails with "user-mapped section open" errors on `.pcm` files, the IDE has them locked — use `--clean-first` to recover.
Report all compiler errors and warnings with file name and line number.
On success, confirm with the binary path.