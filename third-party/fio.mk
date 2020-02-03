#
# fio
#
.PHONY: third-party-fio
third-party-fio:
	@echo "## xNVMe: make third-party-fio"
	@echo "# Preparing third-party (fio)"
	@if [ ! -f "third-party/fio/fio.c" ]; then	\
		$(MAKE) third-party-update;		\
	fi
	@$(MAKE) third-party-fio-clean
	@$(MAKE) third-party-fio-build

.PHONY: third-party-fio-clean
third-party-fio-clean:
	@echo "## xNVMe: make third-party-fio-clean"
	cd third-party/fio && $(MAKE) clean || true

.PHONY: third-party-fio-clobber
third-party-fio-clobber: third-party-fio-clean
	@echo "## xNVMe: make third-party-fio-clobber"
	cd third-party/fio && git clean -dfx || true
	cd third-party/fio && git clean -dfX || true
	cd third-party/fio && git checkout . || true

.PHONY: third-party-fio-build
third-party-fio-build:
	@echo "## xNVMe: make third-party-fio-build"
	cd third-party/fio && $(MAKE)
