#!/usr/bin/env bash

set -uo pipefail

project_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
hwc=${HWC:-"$project_root/INSTALL/bin/hwc"}

if [[ ! -x "$hwc" ]]; then
    echo "error: HWC executable not found: $hwc" >&2
    echo "set HWC=/path/to/hwc or build the project first" >&2
    exit 2
fi

is_fault_flow() {
    local flow=$1 file_list
    file_list=$(sed -nE 's/^[[:space:]]*read_file[[:space:]]+-f[[:space:]]+([^[:space:]#]+).*$/\1/p' "$flow" | head -n1)
    [[ -n "$file_list" ]] || return 1
    [[ "$file_list" = /* ]] || file_list="$(dirname -- "$flow")/$file_list"
    [[ -f "$file_list" ]] || return 1
    rg -q '^\+define\+(INJECT_FAULT|KILL_)' "$file_list"
}

dump_flow() {
    local flow=$1 flow_dir dump_root command name output errors dump_script formal=0
    flow=$(cd -- "$(dirname -- "$flow")" && pwd)/$(basename -- "$flow")
    [[ -f "$flow" ]] || { echo "error: no such flow: $flow" >&2; return 2; }
    flow_dir=$(dirname -- "$flow")

    if is_fault_flow "$flow"; then
        dump_root="$(dirname -- "$flow")/build_inject_fault/dump"
    else
        dump_root="$(dirname -- "$flow")/build/dump"
    fi
    mkdir -p "$dump_root"

    if rg -q '^[[:space:]]*(prove|cover|formal_matrix)[[:space:]]+' "$flow"; then
        formal=1
    fi

    local -a dumps=(
        dump_elb_design
        dump_eir_unopt
        dump_vir_unopt
        dump_eir_opt
        dump_vir_opt
        dump_mir
        dump_btor2_low_unopt
        dump_btor2_low_opt
        'dump_asm backend=emul'
    )
    if (( formal )); then
        dumps+=(dump_btor2_high_unopt dump_btor2_high_opt)
    fi

    dump_script=$(mktemp "$flow_dir/.hwc-dump.XXXXXX.tcl")
    sed '/^[[:space:]]*write_artifacts[[:space:]]/d' "$flow" >"$dump_script"
    if rg -q '^[[:space:]]*formal_matrix[[:space:]]+(prove|cover)[[:space:]]+' "$flow"; then
        local matrix_property
        matrix_property=$(sed -nE 's/^[[:space:]]*formal_matrix[[:space:]]+(prove|cover)[[:space:]]+([^[:space:]#]+).*$/\2/p' "$flow" | tail -n1)
        if [[ -n "$matrix_property" ]]; then
            printf '\nprove %s\n' "$matrix_property" >>"$dump_script"
        fi
    fi

    echo "[dump] ${flow#$project_root/} -> ${dump_root#$project_root/}"
    for command in "${dumps[@]}"; do
        name=${command// /_}
        name=${name//=/_}
        output="$dump_root/$name.txt"
        errors="$dump_root/$name.stderr"
        if "$hwc" -f "$dump_script" -norun "$command" >"$output" 2>"$errors"; then
            : >"$errors"
        else
            echo "[dump] unavailable: $command (${flow#$project_root/})" >&2
        fi
    done
    rm -f "$dump_script"
    return 0
}

if [[ $# -eq 1 && $1 == --all ]]; then
    while IFS= read -r -d '' flow; do
        dump_flow "$flow"
    done < <(find "$project_root/tests" -type f -name 'run*.tcl' \
        -not -path '*/build/*' -not -path '*/build_inject_fault/*' -print0 | sort -z)
    exit 0
fi

if [[ $# -ne 1 ]]; then
    echo "usage: $(basename "$0") FLOW.tcl | --all" >&2
    exit 2
fi

dump_flow "$1"
