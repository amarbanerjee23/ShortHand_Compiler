import os
import shlex
import lit.formats

config.name = "ShortHandMLIR"
config.test_format = lit.formats.ShTest(True)
config.suffixes = [".mlir", ".test"]
config.test_source_root = os.path.join(config.shorthand_source, "test")
config.test_exec_root = os.path.join(config.shorthand_binary, "test")
for name in ("PATH", "LD_LIBRARY_PATH", "ASAN_OPTIONS", "UBSAN_OPTIONS"):
    if name in os.environ:
        config.environment[name] = os.environ[name]
config.substitutions += [
    ("%shorthand-opt", shlex.quote(os.path.join(config.shorthand_binary, "shorthand-opt"))),
    ("%FileCheck", shlex.quote(config.filecheck)),
    ("%python", shlex.quote(config.python)),
    ("%example", shlex.quote(os.path.join(config.shorthand_source, "examples", "ai_greenai_pipeline.mlir"))),
]
