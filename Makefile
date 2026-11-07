# VaveHero: the everyday commands. The firmware itself is built by ./build.sh (docs/DINGHY_README.md: the toolchain).
#   make test       VaveHero's host tests and the app in Dinghy's host mode (seconds, no FM-1, no toolchain)
#   make host       the app on the host with the default script: build/host/vavehero.wav and its screens
#   make reference  the browser reference in Chromium (node, Playwright): tests/reference.cjs
#   make songs      songs/*.mid and the reference's songs block, from tools/songs.py
#   make fw         the FM-1 package build/vavehero.fwsc (JieLi toolchain in ~/.jieli, SDK in ~/fw-AC79_AIoT_SDK)
#   make check      what CI runs after the package: VaveHero's tests, then Dinghy's platform tests on it
PY ?= python3

.PHONY: test host reference songs fw check clean
test:
	sh tests/run_vavehero.sh
host:
	sh tests/run_host.sh
reference:
	node tests/reference.cjs
songs:
	$(PY) tools/songs.py
fw:
	PYTHON=$(PY) sh build.sh
	sha256sum build/vavehero.fwsc
check: fw
	sh tests/run_vavehero.sh
	sh tests/run_tests.sh
clean:
	rm -rf build/host build/gen build/loader build/vavehero*
