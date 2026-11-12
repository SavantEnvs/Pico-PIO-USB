#!/usr/bin/env bash
#
# mayhem/build.sh — builds the Pico-PIO-USB fuzz harnesses, their standalone reproducers and the
# known-answer oracle probe. The library is hardware-bound (RP2040 PIO), so it is compiled UNMODIFIED
# against a host-side stand-in for the Pico SDK headers in mayhem/mocksdk/.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

OUT=/mayhem
W="$(mktemp -d /tmp/pio-usb-build.XXXXXX)"
trap 'rm -rf "$W"' EXIT
INC="-I$SRC/mayhem/mocksdk -I$SRC/src"
LIBSRC="usb_crc pio_usb pio_usb_device"

# 1) The library (+ mock SDK) with sanitizers, DWARF 3 and SanCov on the fuzzed code.
#    nonnull-attribute and pointer-overflow are relaxed on the library only: the status stage of every
#    control transfer runs transfer_start(ep, NULL, 0), then memcpy(NULL, ..., 0) and app_buf + 0 on the
#    NULL buffer (pio_usb.c, pio_usb_device.c). That benign UB would otherwise halt every well-formed
#    control request. ASan and the rest of UBSan stay on.
mkdir -p "$W/fuzz"
for f in $LIBSRC; do
  $CC $SANITIZER_FLAGS -fno-sanitize=nonnull-attribute,pointer-overflow $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O1 $INC -c "src/$f.c" -o "$W/fuzz/$f.o"
done
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -O1 $INC -c mayhem/mocksdk/mocksdk.c -o "$W/fuzz/mocksdk.o"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$W/fuzz/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -x c -c "$STANDALONE_FUZZ_MAIN" -o "$W/fuzz/standalone_main.o"
LIBOBJS="$W/fuzz/usb_crc.o $W/fuzz/pio_usb.o $W/fuzz/pio_usb_device.o $W/fuzz/mocksdk.o $W/fuzz/lsan_off.o"

# 2) One libFuzzer binary + one standalone reproducer per harness.
for t in crc ll_encode ll_endpoint device; do
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O1 $INC -c "mayhem/harness_$t.c" -o "$W/fuzz/harness_$t.o"
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$W/fuzz/harness_$t.o" $LIBOBJS -o "$OUT/$t"
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS "$W/fuzz/harness_$t.o" "$W/fuzz/standalone_main.o" $LIBOBJS -o "$OUT/$t-standalone"
done

# Reproducer for mayhem/ll_endpoint/known-findings: the harness without its max-packet-size precondition.
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O1 -DLL_ENDPOINT_ALLOW_OVERSIZE $INC -c mayhem/harness_ll_endpoint.c -o "$W/fuzz/harness_ll_endpoint_oversize.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS "$W/fuzz/harness_ll_endpoint_oversize.o" "$W/fuzz/standalone_main.o" $LIBOBJS -o "$OUT/ll_endpoint-oversize-standalone"

# 3) The oracle probe, built with NORMAL flags (no sanitizers, no coverage on the fuzz path).
mkdir -p "$W/kat"
for f in $LIBSRC; do
  $CC -O1 $COVERAGE_FLAGS $INC -c "src/$f.c" -o "$W/kat/$f.o"
done
$CC -O1 $COVERAGE_FLAGS $INC -c mayhem/mocksdk/mocksdk.c -o "$W/kat/mocksdk.o"
$CC -O1 $COVERAGE_FLAGS $INC -c mayhem/kat/kat_probe.c -o "$W/kat/kat_probe.o"
$CC $COVERAGE_FLAGS "$W/kat"/*.o -o "$OUT/kat_probe"
