#!/usr/bin/env bash
set -euo pipefail

mode="${1:---all}"
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

# Python 解析:依次试探 python3/python,取第一个能真实运行的
# (Windows 商店的 python3 存根在 PATH 里存在但执行即失败,须探测而非只看存在)。
resolve_python() {
    local cand
    for cand in "${PYTHON:-}" python3 python; do
        [[ -z "${cand}" ]] && continue
        if command -v "${cand}" >/dev/null 2>&1 &&
            "${cand}" --version >/dev/null 2>&1; then
            echo "${cand}"
            return 0
        fi
    done
    return 1
}
PY_BIN="$(resolve_python)" || {
    echo "ERROR: no runnable python3/python found in PATH." >&2
    exit 1
}

# C 编译器解析:CC 优先,其次 cc,再退 gcc(MinGW/Git Bash 环境常见)。
resolve_cc() {
    local cand
    for cand in "${CC:-}" cc gcc; do
        [[ -z "${cand}" ]] && continue
        if command -v "${cand}" >/dev/null 2>&1; then
            echo "${cand}"
            return 0
        fi
    done
    return 1
}
CC_BIN="$(resolve_cc)" || {
    echo "ERROR: no runnable cc/gcc found in PATH." >&2
    exit 1
}

usage() {
    echo "Usage: $0 [--all|--static|--firmware]" >&2
}

run_static_checks() {
    local actionlint_bin
    local test_dir

    "${PY_BIN}" tools/check_repo.py

    actionlint_bin="${ACTIONLINT_BIN:-}"
    if [[ -z "${actionlint_bin}" ]]; then
        actionlint_bin="$(command -v actionlint || true)"
    fi
    if [[ -z "${actionlint_bin}" || ! -x "${actionlint_bin}" ]]; then
        actionlint_bin="$(./tools/install-actionlint.sh)"
    fi
    "${actionlint_bin}" -color .github/workflows/*.yml

    test_dir="$(mktemp -d /tmp/ai-passport-host-tests.XXXXXX)"
    "${CC_BIN}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_tarot_deck.c main/tarot_deck.c \
        -o "${test_dir}/test_tarot_deck"
    "${test_dir}/test_tarot_deck"
    "${CC_BIN}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_tarot_engine.c main/tarot_engine.c main/tarot_deck.c \
        -o "${test_dir}/test_tarot_engine"
    "${test_dir}/test_tarot_engine"
    "${CC_BIN}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_tarot_nav.c main/tarot_nav.c \
        -o "${test_dir}/test_tarot_nav"
    "${test_dir}/test_tarot_nav"
    "${CC_BIN}" -std=c11 -Wall -Wextra -Werror -Imain \
        tests/test_tarot_emblem_math.c main/tarot_emblem_math.c \
        -o "${test_dir}/test_tarot_emblem_math"
    "${test_dir}/test_tarot_emblem_math"
    "${CC_BIN}" -std=c11 -Wall -Wextra -Werror -Icomponents/bsp/src \
        tests/test_bsp_display_rounding.c components/bsp/src/bsp_display_rounding.c \
        -o "${test_dir}/test_bsp_display_rounding"
    "${test_dir}/test_bsp_display_rounding"
    "${CC_BIN}" -std=c11 -Wall -Wextra -Werror -Icomponents/bsp/src \
        tests/test_bsp_es8311_sleep_check.c components/bsp/src/bsp_es8311_sleep_check.c \
        -o "${test_dir}/test_bsp_es8311_sleep_check"
    "${test_dir}/test_bsp_es8311_sleep_check"
    "${CC_BIN}" -std=c11 -Wall -Wextra -Werror \
        -Itests/bsp_stubs -Icomponents/bsp/include \
        tests/test_bsp_button.c -o "${test_dir}/test_bsp_button"
    "${test_dir}/test_bsp_button"
    "${CC_BIN}" -std=c11 -Wall -Wextra -Werror \
        -Itests/bsp_stubs -Icomponents/bsp/include \
        tests/test_bsp_lvgl_init.c components/bsp/src/bsp_display_rounding.c \
        -o "${test_dir}/test_bsp_lvgl_init"
    "${test_dir}/test_bsp_lvgl_init"
    "${CC_BIN}" -std=c11 -Wall -Wextra -Werror \
        -Itests/audio_stubs -Icomponents/bsp/include -Icomponents/bsp/src \
        tests/test_bsp_audio_recovery.c components/bsp/src/bsp_es8311_sleep_check.c \
        -o "${test_dir}/test_bsp_audio_recovery"
    "${test_dir}/test_bsp_audio_recovery"
    PYTHONDONTWRITEBYTECODE=1 "${PY_BIN}" tests/test_deep_sleep_contract.py
    PYTHONDONTWRITEBYTECODE=1 "${PY_BIN}" tests/test_tarot_font_contract.py
    PYTHONDONTWRITEBYTECODE=1 "${PY_BIN}" tests/test_tarot_art_contract.py
    PYTHONDONTWRITEBYTECODE=1 "${PY_BIN}" tests/test_check_repo.py
    PYTHONDONTWRITEBYTECODE=1 "${PY_BIN}" tests/test_verify_firmware.py
    PYTHONDONTWRITEBYTECODE=1 "${PY_BIN}" tests/test_archive_firmware.py
    PYTHONDONTWRITEBYTECODE=1 "${PY_BIN}" tests/test_install_passport_skills.py
    rm -rf "${test_dir}"
    echo "Host tests: PASS"
}

run_firmware_checks() (
    local validation_build_dir

    if ! command -v idf.py >/dev/null 2>&1; then
        echo "ERROR: idf.py is not available; activate ESP-IDF 5.5.3 first." >&2
        return 1
    fi

    validation_build_dir="$(mktemp -d /tmp/ai-passport-firmware.XXXXXX)"
    trap 'case "${validation_build_dir}" in /tmp/ai-passport-firmware.*) rm -rf -- "${validation_build_dir}" ;; esac' EXIT

    SDKCONFIG_DEFAULTS="${repo_root}/sdkconfig.defaults" \
        idf.py -B "${validation_build_dir}" \
        -D "SDKCONFIG=${validation_build_dir}/sdkconfig" build
    idf.py -B "${validation_build_dir}" merge-bin \
        -o "${validation_build_dir}/FoloToy-AI-Passport-full.bin"
    "${PY_BIN}" tools/verify_firmware.py "${validation_build_dir}"
    PYTHONDONTWRITEBYTECODE=1 "${PY_BIN}" tools/archive_firmware.py create \
        "${validation_build_dir}" --archive-root "${repo_root}/build/firmware"
    mkdir -p "${repo_root}/build"
    install -m 0644 \
        "${validation_build_dir}/FoloToy-AI-Passport-full.bin" \
        "${repo_root}/build/FoloToy-AI-Passport-full.bin"
    echo "Firmware build: PASS"
)

cd "${repo_root}"
case "${mode}" in
    --all)
        run_static_checks
        run_firmware_checks
        ;;
    --static)
        run_static_checks
        ;;
    --firmware)
        run_firmware_checks
        ;;
    *)
        usage
        exit 2
        ;;
esac
