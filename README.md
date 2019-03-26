# lb-gps-linux / liblbgpsdo  [![Build Status](https://travis-ci.org/philcrump/lb-gps-linux.svg?branch=master)](https://travis-ci.org/philcrump/lb-gps-linux)

A library for communication with LeoBodnar GPSDOs, compatible with the dual-output, and single-output 'mini', models.

This work is derived from [github.com/simontheu/lb-gps-linux](https://github.com/simontheu/lb-gps-linux)

## Prerequisites

gcc libudev-dev

make is optional depending on how you want to build it.

## Examples

#### /examples/cli-status/

Simple app that reads and prints the current configuration and lock status.

```cd examples/cli-status/ && make && ./lbgpsdo```

## License

A license with the original author has not yet been negotiated, and so this work is currently unlicensed, use at your own legal risk.