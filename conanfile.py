import os
import shutil

from conan import ConanFile


class MldsaConan(ConanFile):
    name = "qrs-mldsa-native"
    version = "0.1.0"
    license = "Apache-2.0 OR ISC OR MIT"
    url = "https://github.com/pq-code-package/mldsa-native"
    description = "ML-DSA post-quantum signature implementation (native C)"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    exports_sources = "*"

    def build(self):
        """Build the project using the repository Makefile.

        We pass BUILD_DIR to keep build artifacts inside Conan's build folder
        so packaging can find them reliably.
        """
        jobs = os.cpu_count() or 1
        build_dir = os.path.join(self.build_folder, "build")

        # Ensure build directory exists (Make will create subdirs but ensure root)
        os.makedirs(build_dir, exist_ok=True)

        cwd = os.getcwd()
        try:
            os.chdir(self.source_folder)
            # Invoke the project's Makefile. The default Make target 'build' mirrors
            # the repository's intention (it builds tests and libraries).
            self.run(f"make -j{jobs} BUILD_DIR={build_dir} build")
        finally:
            os.chdir(cwd)

    def package(self):
        """Collect headers, static libraries, binaries and license into the Conan package."""
        build_dir = os.path.join(self.build_folder, "build")

        # Copy the whole mldsa/ dir into include/mldsa
        src_include = os.path.join(self.source_folder, "mldsa")
        dst_include = os.path.join(self.package_folder, "include", "mldsa")
        if os.path.isdir(src_include):
            shutil.copytree(src_include, dst_include, dirs_exist_ok=True)
        else:
            self.output.warning(f"Source include directory not found: {src_include}")

        # Copy top-level headers (if any) as a fallback
        for hdr in ("README.md", "LICENSE", "SECURITY.md"):
            src = os.path.join(self.source_folder, hdr)
            if os.path.exists(src):
                dst = os.path.join(self.package_folder, "licenses")
                os.makedirs(dst, exist_ok=True)
                shutil.copy(src, dst)

        # Copy static libraries (if produced)
        lib_names = ["libmldsa.a", "libmldsa44.a", "libmldsa65.a", "libmldsa87.a"]
        dst_lib = os.path.join(self.package_folder, "lib")
        os.makedirs(dst_lib, exist_ok=True)
        for lib in lib_names:
            src_lib = os.path.join(build_dir, lib)
            if os.path.exists(src_lib):
                shutil.copy(src_lib, dst_lib)
            else:
                # Some builds may put per-target libs in subdirs; look for them
                matches = []
                for root, _dirs, files in os.walk(build_dir):
                    if lib in files:
                        matches.append(os.path.join(root, lib))
                if matches:
                    for m in matches:
                        shutil.copy(m, dst_lib)
                else:
                    self.output.warning(f"Library not found: {lib}")

        # Copy any produced command-line tools into bin/
        dst_bin = os.path.join(self.package_folder, "bin")
        os.makedirs(dst_bin, exist_ok=True)
        # Binaries are typically under build/mldsa*/bin/
        if os.path.isdir(build_dir):
            for root, _dirs, files in os.walk(build_dir):
                if root.endswith(os.path.join("bin")):
                    for f in files:
                        srcf = os.path.join(root, f)
                        shutil.copy(srcf, dst_bin)

    def package_info(self):
        # The repository produces static libraries named libmldsa*.a. Expose the
        # linker names without the 'lib' prefix.
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.bindirs = ["bin"]
        self.cpp_info.includedirs = ["include"]

        # List libs in a conservative order. Consumers can pick the ones they need.
        self.cpp_info.libs = ["mldsa", "mldsa87", "mldsa65", "mldsa44"]
