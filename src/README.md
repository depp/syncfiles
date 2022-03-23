# Converter

## Debugging

Tests can be debugged with GDB:

```shell
bazel build -c dbg //src:convert_test
gdb -ex 'dir .' -ex 'cd bazel-bin' bazel-bin/src/convert_test
```
