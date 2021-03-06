SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILDDIR := .
JULIAHOME := $(abspath $(SRCDIR)/..)
include $(JULIAHOME)/deps/Versions.make
include $(JULIAHOME)/Make.inc

TAGGED_RELEASE_BANNER := ""

ifneq ($(USEMSVC), 1)
CPP_STDOUT := $(CPP) -P
else
CPP_STDOUT := $(CPP) -E
endif

all: $(addprefix $(BUILDDIR)/,pcre_h.jl errno_h.jl build_h.jl.phony file_constants.jl uv_constants.jl version_git.jl.phony)

PCRE_CONST := 0x[0-9a-fA-F]+|[0-9]+
ifeq ($(USE_SYSTEM_PCRE), 1)
  PCRE_INCL_PATH := $(shell $(PCRE_CONFIG) --prefix)/include/pcre2.h
else
  PCRE_INCL_PATH := $(build_includedir)/pcre2.h
endif

$(BUILDDIR)/pcre_h.jl: $(PCRE_INCL_PATH)
	@$(call PRINT_PERL, $(CPP) -D PCRE2_CODE_UNIT_WIDTH=8 -dM $< | perl -nle '/^\s*#define\s+PCRE2_(\w*)\s*\(?($(PCRE_CONST))\)?u?\s*$$/ and print "const $$1 = UInt32($$2)"' | LC_ALL=C sort > $@)

$(BUILDDIR)/errno_h.jl:
	@$(call PRINT_PERL, echo '#include <errno.h>' | $(CPP) -dM - | perl -nle 'print "const $$1 = Int32($$2)" if /^#define\s+(E\w+)\s+(\d+)\s*$$/' | LC_ALL=C sort > $@)

$(BUILDDIR)/file_constants.jl: $(SRCDIR)/../src/file_constants.h
	@$(call PRINT_PERL, $(CPP_STDOUT) -DJULIA $< | perl -nle 'print "$$1 0o$$2" if /^(\s*const\s+[A-z_]+\s+=)\s+(0[0-9]*)\s*$$/; print "$$1" if /^\s*(const\s+[A-z_]+\s+=\s+([1-9]|0x)[0-9A-z]*)\s*$$/' > $@)

$(BUILDDIR)/uv_constants.jl: $(SRCDIR)/../src/uv_constants.h $(build_includedir)/uv-errno.h
	@$(call PRINT_PERL, $(CPP_STDOUT) "-I$(LIBUV_INC)" -DJULIA $< | tail -n 16 > $@)

$(BUILDDIR)/build_h.jl.phony:
	@echo "# This file is automatically generated in base/Makefile" > $@
ifeq ($(XC_HOST),)
	@echo "const MACHINE = \"$(BUILD_MACHINE)\"" >> $@
else
	@echo "const MACHINE = \"$(XC_HOST)\"" >> $@
endif
	@echo "const libm_name = \"$(LIBMNAME)\"" >> $@
	@echo "const libblas_name = \"$(LIBBLASNAME)\"" >> $@
	@echo "const liblapack_name = \"$(LIBLAPACKNAME)\"" >> $@
ifeq ($(USE_BLAS64), 1)
	@echo "const USE_BLAS64 = true" >> $@
else
	@echo "const USE_BLAS64 = false" >> $@
endif
ifeq ($(USE_GPL_LIBS), 1)
	@echo "const USE_GPL_LIBS = true" >> $@
else
	@echo "const USE_GPL_LIBS = false" >> $@
endif
	@echo "const libfftw_name = \"$(LIBFFTWNAME)\"" >> $@
	@echo "const libfftwf_name = \"$(LIBFFTWFNAME)\"" >> $@
	@echo "const libllvm_version_string = \"$$($(LLVM_CONFIG_HOST) --version)\"" >> $@
	@echo "const VERSION_STRING = \"$(JULIA_VERSION)\"" >> $@
	@echo "const TAGGED_RELEASE_BANNER = \"$(TAGGED_RELEASE_BANNER)\"" >> $@
	@echo "const SYSCONFDIR = \"$(sysconfdir_rel)\"" >> $@
	@echo "const DATAROOTDIR = \"$(datarootdir_rel)\"" >> $@
	@echo "const DOCDIR = \"$(docdir_rel)\"" >> $@
	@echo "const LIBDIR = \"$(libdir_rel)\"" >> $@
	@echo "const PRIVATE_LIBDIR = \"$(private_libdir_rel)\"" >> $@
	@echo "const INCLUDEDIR = \"$(includedir_rel)\"" >> $@

	@# This to ensure that we always rebuild this file, but only when it is modified do we touch build_h.jl,
	@# ensuring we rebuild the system image as infrequently as possible
	@if ! cmp -s $@ build_h.jl; then \
		$(call PRINT_PERL,) \
		mv $@ build_h.jl; \
	else \
		rm -f $@; \
	fi

$(BUILDDIR)/version_git.jl.phony: $(SRCDIR)/version_git.sh
ifneq ($(NO_GIT), 1)
	sh $< $(SRCDIR) > $@
	@# This to avoid touching git_version.jl when it is not modified,
	@# so that the system image does not need to be rebuilt.
	@if ! cmp -s $@ version_git.jl; then \
	    $(call PRINT_PERL,) \
	    mv $@ version_git.jl; \
	else \
	    rm -f $@; \
	fi
else
ifeq ($(shell [ -f $(BUILDDIR)/version_git.jl ] && echo "true"), true)
	@# Give warning if boilerplate git is used
	@if grep -q "Default output if git is not available" $(BUILDDIR)/version_git.jl; then \
	    echo "WARNING: Using boilerplate git version info" >&2; \
	fi
else
	$(warning "WARNING: Generating boilerplate git version info")
	@sh $(SRCDIR)/version_git.sh $(SRCDIR) NO_GIT > $(BUILDDIR)/version_git.jl
endif
endif

.PHONY: $(BUILDDIR)/build_h.jl.phony $(BUILDDIR)/version_git.jl.phony clean all

clean:
	rm -f $(BUILDDIR)/pcre_h.jl
	rm -f $(BUILDDIR)/errno_h.jl
	rm -f $(BUILDDIR)/build_h.jl
	rm -f $(BUILDDIR)/build_h.jl.phony
	rm -f $(BUILDDIR)/fenv_constants.jl # To be removed
	rm -f $(BUILDDIR)/uv_constants.jl
	rm -f $(BUILDDIR)/file_constants.jl
	rm -f $(BUILDDIR)/version_git.jl
	rm -f $(BUILDDIR)/version_git.jl.phony
