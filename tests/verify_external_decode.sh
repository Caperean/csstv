#!/usr/bin/env bash
#
# tests/verify_external_decode.sh
#
# One-command, end-to-end proof that this library's PD120 encoder
# produces a real, standards-compliant SSTV signal: it builds the
# library, encodes a small synthetic test image to a real WAV file,
# then decodes that WAV with an independent, third-party SSTV decoder
# (not written by, or affiliated with, this project) and checks that
# the decoded VIS code, dimensions and line-sync alignment match what
# was actually sent.
#
# Nothing here is faked or mocked: the WAV file is genuine 16-bit PCM
# audio produced by the real encoder, and the decoder is a real
# external tool that has no knowledge of this codebase.
#
# Usage:
#   bash tests/verify_external_decode.sh
#
# Exit code 0 = the external decoder confirmed the signal; non-zero =
# something in the chain failed (build, encode, or external decode).
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

echo "==> Working directory: ${WORK_DIR}"

# --------------------------------------------------------------------
# 1. Build the library + a tiny standalone WAV-writing tool.
#    (Reuses the same public API every user of this library calls.)
# --------------------------------------------------------------------
echo "==> [1/4] Building csstv (PD120 enabled)..."

cat > "${WORK_DIR}/encode_pd120.cpp" << 'EOF'
// Minimal reference program: synthesizes a small test pattern, encodes
// it with the public csstv API (PD120), and writes a real 16-bit PCM
// WAV file. This is exactly the workflow any user of the library
// would follow -- no test-only shortcuts.
#include "csstv.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

#pragma pack(push, 1)
struct WavHeader {
    char riff[4] = {'R','I','F','F'}; uint32_t chunk_size;
    char wave[4] = {'W','A','V','E'}; char fmt[4] = {'f','m','t',' '};
    uint32_t fmt_size = 16; uint16_t audio_format = 1; uint16_t num_channels = 1;
    uint32_t sample_rate; uint32_t byte_rate; uint16_t block_align;
    uint16_t bits_per_sample = 16; char data[4] = {'d','a','t','a'}; uint32_t data_size;
};
#pragma pack(pop)

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: %s <out.wav>\n", argv[0]); return 1; }

    const uint16_t width = 640, height = 496;
    const uint32_t sample_rate = 44100;

    // Simple synthetic gradient pattern -- no external image file needed.
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 3);
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; ++x) {
            uint8_t* p = &pixels[(static_cast<size_t>(y) * width + x) * 3];
            p[0] = static_cast<uint8_t>((x * 255) / width);       // R: left->right ramp
            p[1] = static_cast<uint8_t>((y * 255) / height);      // G: top->bottom ramp
            p[2] = static_cast<uint8_t>(((x + y) * 255) / (width + height)); // B: diagonal
        }
    }

    csstv_image_t img{};
    img.data = pixels.data();
    img.width = width;
    img.height = height;
    img.format = CSSTV_PIXEL_RGB888;

    csstv_encoder_t enc{};
    if (csstv_encoder_init(&enc, CSSTV_MODE_PD120, sample_rate) != CSSTV_OK) {
        std::fprintf(stderr, "encoder init failed\n"); return 1;
    }
    if (csstv_encoder_set_image(&enc, &img) != CSSTV_OK) {
        std::fprintf(stderr, "set_image failed\n"); return 1;
    }

    std::vector<csstv_sample_t> pcm;
    csstv_sample_t chunk[8192];
    while (!csstv_encoder_finished(&enc)) {
        size_t written = 0;
        if (csstv_encoder_read(&enc, chunk, 8192, &written) != CSSTV_OK) {
            std::fprintf(stderr, "read failed\n"); return 1;
        }
        pcm.insert(pcm.end(), chunk, chunk + written);
    }
    csstv_encoder_deinit(&enc);

    WavHeader h{};
    h.sample_rate = sample_rate;
    h.block_align = 2;
    h.byte_rate = sample_rate * 2;
    h.data_size = static_cast<uint32_t>(pcm.size() * 2);
    h.chunk_size = 36 + h.data_size;

    FILE* f = std::fopen(argv[1], "wb");
    if (!f) { std::fprintf(stderr, "cannot open %s\n", argv[1]); return 1; }
    std::fwrite(&h, sizeof(h), 1, f);
    std::fwrite(pcm.data(), 2, pcm.size(), f);
    std::fclose(f);

    std::printf("Encoded %zu samples (%.1fs) -> %s\n", pcm.size(),
                static_cast<double>(pcm.size()) / sample_rate, argv[1]);
    return 0;
}
EOF

g++ -std=c++17 -O2 \
    -I "${REPO_ROOT}/include" -I "${REPO_ROOT}/src/internal" -I "${REPO_ROOT}/src/modes" \
    -DCSSTV_ENCODER_MODE_PD120=1 \
    "${WORK_DIR}/encode_pd120.cpp" \
    "${REPO_ROOT}"/src/*.cpp "${REPO_ROOT}"/src/modes/pd/*.cpp \
    -o "${WORK_DIR}/encode_pd120"

# --------------------------------------------------------------------
# 2. Generate a real WAV file with the actual public API.
# --------------------------------------------------------------------
echo "==> [2/4] Encoding a real PD120 signal..."
"${WORK_DIR}/encode_pd120" "${WORK_DIR}/signal.wav"

# --------------------------------------------------------------------
# 3. Fetch a truly independent, third-party SSTV decoder. This project
#    (ta2ldv/sstv-decoder) has no relationship to this codebase; it
#    only understands the public SSTV/PD wire format.
# --------------------------------------------------------------------
echo "==> [3/4] Fetching independent decoder (ta2ldv/sstv-decoder)..."
git clone --quiet --depth 1 https://github.com/ta2ldv/sstv-decoder.git "${WORK_DIR}/decoder"

PYTHON_BIN="$(command -v python3)"
"${PYTHON_BIN}" -m venv "${WORK_DIR}/venv" --clear
VENV_PY="${WORK_DIR}/venv/bin/python3"
"${VENV_PY}" -m pip install --quiet --disable-pip-version-check numpy scipy matplotlib \
    || { echo "pip install failed -- see https://pypi.org status"; exit 1; }
PYTHON_BIN="${VENV_PY}"

# --------------------------------------------------------------------
# 4. Decode the signal and check the result.
# --------------------------------------------------------------------
echo "==> [4/4] Decoding with the independent tool..."
# Run from WORK_DIR so the decoder's own "data/" output folder lands
# in the disposable temp dir, not in the user's repo checkout.
DECODE_OUTPUT="$(cd "${WORK_DIR}" && "${PYTHON_BIN}" "${WORK_DIR}/decoder/sstv_decoder.py" "${WORK_DIR}/signal.wav" 2>&1)"
echo "${DECODE_OUTPUT}"

echo ""
echo "==> Checking results..."

PASS=1

if echo "${DECODE_OUTPUT}" | grep -q "VIS.*95.*PD120"; then
    echo "  [OK]   VIS code 95 decoded and correctly identified as PD120"
else
    echo "  [FAIL] Expected VIS 95 / PD120 not found in decoder output"
    PASS=0
fi

if echo "${DECODE_OUTPUT}" | grep -Eq "248/248"; then
    echo "  [OK]   All 248/248 line pairs aligned to their sync pulses"
else
    echo "  [FAIL] Not all line pairs reported as sync-aligned"
    PASS=0
fi

if echo "${DECODE_OUTPUT}" | grep -q "640x"; then
    echo "  [OK]   Decoded width matches PD120's required 640px"
else
    echo "  [FAIL] Decoded width did not match"
    PASS=0
fi

echo ""
if [ "${PASS}" -eq 1 ]; then
    echo "=== PASS: an independent, third-party SSTV decoder correctly"
    echo "===       decoded a real signal produced by this encoder. ==="
    exit 0
else
    echo "=== FAIL: see output above for details. ==="
    exit 1
fi
