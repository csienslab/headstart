.PHONY: help aggvdf_test01 testU01 submodule

all: help

INCLUDE_HEADERS = -I${HOME}/opt/include/ -I./pkg/chiavdf/src/ -I./src/
BIN_PATH = ./bin
TEST_U01_INCLUDE = -I./cmd/testU01/include -L ./cmd/testU01/lib -ltestu01 -lprobdist -lmylib -lm

ifeq ($(shell uname), Darwin) # macOS
	GMP = $$(brew --prefix gmp)/lib/libgmp.a $$(brew --prefix gmp)/lib/libgmpxx.a
else # ubuntu
	GMP = $$(find /usr/local -name 'libgmp.a') $$(find /usr/local -name 'libgmpxx.a')
endif

## Build aggvdf_worker (Mac)
aggvdf_test01:
	g++ -O3 -std=c++1z ./cmd/$@/main.cpp $(GMP) \
	$(INCLUDE_HEADERS) -pthread -o $(BIN_PATH)/$@

## Build aggvdf_test01 in docker
build_aggvdf_test01_docker:
	docker build \
		-f deployment/docker/aggvdf_test01/Dockerfile \
		-t local/aggvdf_test01 \
		. && \
	echo "docker run local/aggvdf_test01"

## TestU01 (Ubuntu)
# testU01:
# 	g++ -O3 -std=c++1z ./cmd/$@/main.cpp $(GMP) \
# 	$(INCLUDE_HEADERS) $(TEST_U01_INCLUDE) -pthread -o $(BIN_PATH)/$@

## Initialize submodule
submodule:
	git submodule update --init --recursive
	git submodule set-branch --branch tag/v1.1.0 pkg/chiavdf

clean_submodule:
	git rm --cached pkg/chiavdf
	rm -rf pkg/chiavdf

###########################################################
# Help
# COLORS
GREEN  := $(shell tput -Txterm setaf 2)
YELLOW := $(shell tput -Txterm setaf 3)
WHITE  := $(shell tput -Txterm setaf 7)
RESET  := $(shell tput -Txterm sgr0)

TARGET_MAX_CHAR_NUM=26
## Show help
help:
	@echo ''
	@echo 'Usage:'
	@echo '  ${YELLOW}make${RESET} ${GREEN}<target>${RESET}'
	@echo ''
	@echo 'Targets:'
	@awk '/^[a-zA-Z\-\_0-9]+:/ { \
		helpMessage = match(lastLine, /^## (.*)/); \
		if (helpMessage) { \
			helpCommand = substr($$1, 0, index($$1, ":")-1); \
			helpMessage = substr(lastLine, RSTART + 3, RLENGTH); \
			printf "  ${YELLOW}%-$(TARGET_MAX_CHAR_NUM)s${RESET} ${GREEN}%s${RESET}\n", helpCommand, helpMessage; \
		} \
	} \
	{ lastLine = $$0 }' $(MAKEFILE_LIST)
