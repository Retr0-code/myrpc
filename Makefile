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
	$(RM) -rf build mysyslog/build

.PHONY: deb
deb:
	$(CHMOD) +x misc/deb-build.sh
	$(CHMOD) +x mysyslog/misc/generate_apt_repo_release.sh

	@if ! $(DOCKER) compose start repo-build; then \
		$(DOCKER) compose up repo-build -d; \
	fi

deb-clean:
	$(RM) -rf deb

repo-deploy:
	$(CHMOD) +x misc/deb-deploy.sh
	@if ! $(DOCKER) compose start repo-deploy; then \
		$(DOCKER) compose up repo-deploy -d; \
	fi

repo-stop:
	$(DOCKER) compose exec repo-deploy /usr/sbin/nginx -s stop
