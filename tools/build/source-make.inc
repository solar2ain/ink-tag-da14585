.DEFAULT_GOAL := all
.PHONY: all build full host-tests
POWERSHELL ?= pwsh
SDK_ROOT ?= $(CURDIR)/.sdk

all: build
build:
	"$(POWERSHELL)" -NoProfile -File build.ps1 -SdkRoot "$(SDK_ROOT)" -OtaSignaturePolicy "$(INKTAG_DA_SIGNATURE_POLICY)" $(if $(strip $(SIGNED_APP)),-SignedApp "$(SIGNED_APP)")

full:
	"$(POWERSHELL)" -NoProfile -File build.ps1 -SdkRoot "$(SDK_ROOT)" -OtaSignaturePolicy "$(INKTAG_DA_SIGNATURE_POLICY)" -RequireFull $(if $(strip $(SIGNED_APP)),-SignedApp "$(SIGNED_APP)")

host-tests:
	python tools/build/run_host_tests.py --sanitize
