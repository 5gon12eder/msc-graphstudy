#! /bin/bash -eu
#! -*- coding:utf-8; mode:shell-script; -*-

# Copyright (C) 2018 Karlsruhe Institute of Technology
# Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
#
# Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
# provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

set -eu

helptext="usage: $0 [FILE...]

Runs some stylistic checks over files staged for commit.
Exit status is 0 if and only if all files are conforming.
Create a symlink '.git/hooks/pre-commit' to enable this hook.
If FILEs are given, check them instead of staged files.
"

chkflags="$(git config hooks.chkflags || echo '--color=auto')"
chkflags_copyright=('--strict' '--brief' ${chkflags})
chkflags_whitespace=(${chkflags})
chkflags_cmake=(${chkflags})
unset chkflags

biber_tool_options=(
    --fixinits
    --input-encoding='utf-8'
    --isbn-normalise
    --isbn13
    --output-encoding='ascii'
    --output-fieldcase=lower
    --output-format=bibtex
    --strip-comments
)

filelist=()
cmakefiles=()
bibtexfiles=()

declare -a files_from_cmd
declare -a files_from_git

function handle_cmd_args {
    local arg
    for arg in "$@"
    do
        case "${arg}" in
            --help)
                echo "${helptext}"
                exit
                ;;
            --version)
                basename "$0"
                exit
                ;;
            -*)
                fatal "Unknown option: ${arg}"
                ;;
            *)
                files_from_cmd+=("${arg}")
                ;;
        esac
    done
}

function select_files {
    local filter file basename
    filter=0
    if [ $# -gt 0 ] && [ "$1" = '--filter' ]
    then
        filter=1
        shift
    fi
    for file in "$@"
    do
        [ ${filter} -eq 0 ] || git diff --quiet -- "${file}" || fatal "${file}: File has unstaged local changes"
        [ ${filter} -eq 0 ] || is_regular_file "${file}" || continue
        basename="${file##*/}"
        case "${basename}" in
            *.bz2|*.gz|*.jpg|*.pdf|*.png|*.tar|*.zip)
                continue
                ;;
        esac
        filelist+=("${file}")
        case "${basename}" in
            CMakeLists.txt|*.cmake)
                cmakefiles+=("${file}")
                ;;
            *.bib)
                bibtexfiles+=("${file}")
                ;;
        esac
    done
}

function perform_all_checks {
    local status
    status=0
    printf_checking "Checking whether you're mentioned in the AUTHORS file"
    check_authors_file AUTHORS || status=1
    printf_checking "Checking copyright notices in %d files" ${#filelist[@]}
    [ ${#filelist[@]} -eq 0 ] || ./maintainer/copyright "${chkflags_copyright[@]}" -- "${filelist[@]}" || status=1
    printf_checking "Checking %d files for white-space issues" ${#filelist[@]}
    [ ${#filelist[@]} -eq 0 ] || ./maintainer/whitespace "${chkflags_whitespace[@]}" -- "${filelist[@]}" || status=1
    printf_checking "Checking %d CMake files for style issues" ${#cmakefiles[@]}
    [ ${#cmakefiles[@]} -eq 0 ] || ./maintainer/cmake-style "${chkflags_cmake[@]}" -- "${cmakefiles[@]}" || status=1
    printf_checking "Checking %d BibTeX files for style issues" ${#bibtexfiles[@]}
    [ ${#bibtexfiles[@]} -eq 0 ] || check_bibtex_file "${bibtexfiles[@]}" || status=1
    return ${status}
}

function check_authors_file {
    local user_name user_email expected
    user_name="$(git config user.name)" || notgood "Cannot query Git for user.name setting"
    user_email="$(git config user.email)" || notgood "Cannot query Git for user.email setting"
    expected="$(printf '"%s" <%s>' "${user_name}" "${user_email}")"
    [ -r "$1" ] || notgood "$1: File is not readable"
    sed -e 's;#.*;;g' "$1" | grep -q -F "${expected}" || notgood "$1: Please add yourself: ${expected}"
}

function check_bibtex_file {
    local fixed
    fixed="${1%.bib}_bibertool.bib"
    "${BIBER:-biber}" --noconf --nolog --onlylog "${biber_tool_options[@]}" --tool "$1"                                 \
        && sed -e 's;^\s*%.*;;g' "$1" | diff --ignore-blank-lines -q - "${fixed}" >/dev/null                            \
        && rm -f "${fixed}"                                                                                             \
            || {
            if [ -f "${fixed}" ]
            then
                notgood "$1: Please format like ${fixed}"
            else
                notgood "$1: Biber cannot process this file (syntax error?)"
            fi
        }
}

function fatal {
    echo "pre-commit.sh: error: $*" >&2
    exit 1
}

function notgood {
    echo "pre-commit.sh: $*" >&2
    return 1
}

function printf_checking {
    local formatstring="$1"
    shift
    printf "$(tput bold) ==> ${formatstring} ...$(tput sgr0)\n" "$@" >&2
}

function is_regular_file {
    [ -f "$1" ] && [ ! -L "$1" ]
}

handle_cmd_args "$@"
[ -f .msc-graphstudy ] || fatal "This script must be run from the top-level source directory"
if [ "x${files_from_cmd+set}" = xset ]
then
    select_files "${files_from_cmd[@]}"
else
    mapfile -t files_from_git < <(git diff --cached --name-only --no-color)
    select_files --filter "${files_from_git[@]}"
fi
perform_all_checks || fatal "Please fix the detected issues and then try to commit again"
