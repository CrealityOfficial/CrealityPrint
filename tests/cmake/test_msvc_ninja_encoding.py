"""Run from a Visual Studio developer prompt: python -m unittest discover -s tests/cmake."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


@unittest.skipUnless(os.name == "nt", "MSVC/Ninja regression test requires Windows")
class MSVCNinjaEncodingTest(unittest.TestCase):
    def test_background_header_dependencies(self):
        for tool in ("cmake", "ninja", "cl"):
            if not shutil.which(tool):
                self.skipTest(f"{tool} is unavailable; use a VS developer prompt")
        module = Path(__file__).resolve().parents[2] / "cmake/MSVCNinjaEncoding.cmake"
        # A space in the path also checks command quoting. CREATE_NO_WINDOW
        # reproduces IDE builds whose compiler output falls back to GBK on a
        # Chinese Windows installation, despite Ninja using a UTF-8 prefix.
        with tempfile.TemporaryDirectory(prefix="msvc encoding ") as directory:
            source = Path(directory)
            local_module = source / "cmake/MSVCNinjaEncoding.cmake"
            local_module.parent.mkdir()
            shutil.copyfile(module, local_module)
            shutil.copyfile(module.with_name("msvc_utf8.cmd"), local_module.with_name("msvc_utf8.cmd"))

            def run(*args, success=True):
                result = subprocess.run(
                    args, cwd=source, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                    creationflags=subprocess.CREATE_NO_WINDOW, timeout=120,
                )
                output = result.stdout.decode("utf-8", errors="replace")
                if success:
                    self.assertEqual(result.returncode, 0, output)
                else:
                    self.assertNotEqual(result.returncode, 0, output)
                return output

            (source / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.16)\n"
                "project(encoding_probe LANGUAGES C CXX)\n"
                # Keep an existing launcher when applying the encoding fix.
                'set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_COMMAND};-E;time")\n'
                f'include("{local_module.as_posix()}")\n'
                "add_library(probe STATIC probe.c probe.cpp)\n"
                'target_precompile_headers(probe PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/probe.h>")\n',
                encoding="utf-8",
            )
            header = source / "probe.h"
            header.write_text("#pragma once\nstruct Probe { int value; };\n", encoding="utf-8")
            (source / "probe.c").write_text('#include "probe.h"\nstruct Probe c_probe;\n')
            (source / "probe.cpp").write_text('#include "probe.h"\nProbe cpp_probe;\n')
            run("cmake", "-S", ".", "-B", "build", "-G", "Ninja")
            run("cmake", "--build", "build")
            dependencies = run("ninja", "-C", "build", "-t", "deps")
            self.assertIn("probe.h", dependencies)
            objects = list((source / "build/CMakeFiles/probe.dir").rglob("*.obj"))
            self.assertGreaterEqual(len(objects), 3)
            before = {obj: obj.stat().st_mtime_ns for obj in objects}

            header.write_text("#pragma once\nstruct Probe { int value; int added; };\n", encoding="utf-8")
            scheduled = run("ninja", "-C", "build", "-n")
            self.assertIn("probe.c.obj", scheduled)
            self.assertIn("probe.cpp.obj", scheduled)
            self.assertIn("cmake_pch", scheduled)
            run("cmake", "--build", "build")
            for obj in objects:
                self.assertGreater(obj.stat().st_mtime_ns, before[obj], str(obj))
            self.assertIn("no work to do", run("cmake", "--build", "build"))

            # A compiler error must propagate through the .cmd launcher.
            header.write_text("#error intentional_dependency_test_failure\n", encoding="utf-8")
            failed = run("cmake", "--build", "build", success=False)
            self.assertIn("intentional_dependency_test_failure", failed)


if __name__ == "__main__":
    unittest.main()
