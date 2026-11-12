#!/usr/bin/env bash
#
# mayhem/test.sh — known-answer oracle for Pico-PIO-USB. Runs /mayhem/kat_probe (built by build.sh
# with normal flags), which drives the real CRC5/CRC16, NRZI/bit-stuffing encoder and the device
# stack's GET_DESCRIPTOR handling, and prints the computed values. Every output line is compared with
# its exact expected value (cross-checked offline against an independent CRC16 + NRZI decoder).
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

PROBE=/mayhem/kat_probe
if [ ! -x "$PROBE" ]; then
  echo "missing $PROBE (mayhem/build.sh should have built it)" >&2
  emit_ctrf "pico-pio-usb-kat" 0 1 0
  exit $?
fi

mapfile -t ACTUAL < <("$PROBE" 2>&1)

EXPECTED=(
  'crc16(123456789)=b4c8'
  'crc5(addr=0x15,ep=0xe)=1d'
  'encode(sync,ack) dddf5d7f25'
  'after reset: ep0.size=64 ep1.ep_num=80 dev_addr=0'
  'get_device has_transfer=1 total_len=18 data_id=1'
  '  encoded dddff5d75d77ddddddddf777777777777777777dd7f75d77755757d77777dddd77775ddddddd77777d7d577725'
  'get_config9 has_transfer=1 total_len=9 data_id=1'
  '  encoded dddff5d7df775dddf75ddddd7777ddddddddddf5f7f7dfd57f5725'
  'get_string1 has_transfer=1 total_len=10 data_id=1'
  '  encoded dddff5d75f77f777ddd77777f7577777d5fddddd7dfdddddddd7f55f25'
)

pass=0; fail=0
n=${#EXPECTED[@]}
[ "${#ACTUAL[@]}" -eq "$n" ] || { echo "FAIL: expected $n output lines, got ${#ACTUAL[@]}" >&2; fail=$((fail+1)); }
for ((i = 0; i < n; i++)); do
  if [ "${ACTUAL[i]:-}" = "${EXPECTED[i]}" ]; then
    pass=$((pass+1))
  else
    echo "FAIL line $((i+1)):" >&2
    echo "  expected: ${EXPECTED[i]}" >&2
    echo "  actual:   ${ACTUAL[i]:-<missing>}" >&2
    fail=$((fail+1))
  fi
done

emit_ctrf "pico-pio-usb-kat" "$pass" "$fail" 0
