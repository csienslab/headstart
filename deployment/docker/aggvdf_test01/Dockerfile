FROM golang:1.19.3-buster As builder

WORKDIR /headstart

RUN groupadd -r headstart && \
    useradd -r -g headstart headstart && \
    mkdir -p /headstart/bin && \
    chown -R headstart:headstart /headstart

COPY cmd/  /headstart/cmd/
COPY pkg/chiavdf /headstart/pkg/chiavdf
COPY src/ /headstart/src

RUN apt-get update && apt-get -y install libgmp-dev

RUN g++ -O3 -std=c++1z ./cmd/aggvdf_test01/main.cpp \
    -I/headstart/pkg/chiavdf/src/ -I/headstart/src/ \
    -lgmp -pthread -o /headstart/bin/aggvdf_test01

FROM golang:1.19.3-buster

LABEL project=headstart
WORKDIR /headstart

COPY --from=builder /etc/passwd /etc/passwd
COPY --from=builder /headstart/bin/aggvdf_test01 /headstart/bin/aggvdf_test01

ENTRYPOINT /headstart/bin/aggvdf_test01
CMD []