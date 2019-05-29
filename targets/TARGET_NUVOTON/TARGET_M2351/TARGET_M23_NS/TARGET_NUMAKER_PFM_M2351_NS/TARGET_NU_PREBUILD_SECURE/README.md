# M2351 pre-built secure images for PSA

This directory tree contains secure images released under Permissive Binary License.

Built by mbed-cli using Arm Compiler - version 6.10

These images were compiled by the following command:

## tfm.hex

```sh
mbed compile -t ARMC6 -m NUMAKER_PFM_M2351_S --app-config tools/psa/tfm/mbed_app.json --profile release
```

## spm_smoke.hex

```sh
mbed test --compile -t ARMC6 -m NUMAKER_PFM_M2351_S --app-config tools/psa/tfm/mbed_app.json -n tests-psa-spm_smoke -DUSE_PSA_TEST_PARTITIONS -DUSE_SMOKE_TESTS_PART1 --profile release
```

## spm_client.hex

```sh
mbed test --compile -t ARMC6 -m NUMAKER_PFM_M2351_S --app-config tools/psa/tfm/mbed_app.json -n tests-psa-spm_client -DUSE_PSA_TEST_PARTITIONS -DUSE_CLIENT_TESTS_PART1 --profile release
```

## spm_server.hex

```sh
mbed test --compile -t ARMC6 -m NUMAKER_PFM_M2351_S --app-config tools/psa/tfm/mbed_app.json -n tests-psa-spm_server -DUSE_PSA_TEST_PARTITIONS -DUSE_SERVER_TESTS_PART1 -DUSE_SERVER_TESTS_PART2 --profile release
```

To update the prebuilt images, run the previous commands.
