# HeadStart AggVDF

This is the source code of the AggVDF presented in our paper, [HeadStart: Efficiently Verifiable and Low-Latency Participatory Randomness Generation at Scale](https://www.ndss-symposium.org/wp-content/uploads/2022-234-paper.pdf), published at [Network and Distributed System Security (NDSS) Symposium 2022](https://www.ndss-symposium.org/ndss-paper/auto-draft-184/).

## Installation

### MacOS

```sh
make submodule
brew install gmp
```

### Docker

```sh
make build_aggvdf_test01_docker
docker run local/aggvdf_test01
```

# TODO
cmd/testU01 has not been updated since v1.0.8.
