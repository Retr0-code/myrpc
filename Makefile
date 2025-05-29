RM := /usr/bin/rm
CMAKE := /usr/bin/cmake
CHMOD := /usr/bin/chmod
DOCKER := /usr/bin/docker

PRESET = release

debug: PRESET = debug
debug: all

all:
	$(CMAKE) --preset=$(PRESET)
	$(CMAKE) --build --preset=$(PRESET)

clean:
	$(RM) -rf build mysyslog/build deb

.PHONY: deb
deb:
	$(CHMOD) +x misc/deb-build-setup.sh
	$(CHMOD) +x mysyslog/misc/generate_apt_repo_release.sh

	@if ! $(DOCKER) compose start repo; then \
		$(DOCKER) compose up repo -d; \
	fi
