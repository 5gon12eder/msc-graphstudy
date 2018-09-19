#! /bin/bash -eu
#! -*- coding:utf-8; mode:shell-script; -*-

# Copyright (C) 2018 Karlsruhe Institute of Technology
# Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
#
# Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
# provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

set -eu

helptext="usage: $0 [--verbose] [FILE...]

Runs some stylistic checks over files staged for commit.  Exit status is 0 if
and only if all files are conforming.  Create a symlink '.git/hooks/pre-commit'
to enable this hook.  If FILEs are given, check them instead of staged files.

Options:

 -v, --verbose   more verbose output for builtin checks (can be repeated)
     --help      show usage information and exit
     --version   show version information and exit
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

find_symlinks_cmd=(
    find .
    -regextype posix-extended
    -regex '^[.]/(\.git|build|build-[[:alnum:]._-]+|my-[[:alnum:]._-]+)/?$' -prune
    -or
    -type l
    -print
)

filelist=()
cmakefiles=()
bibtexfiles=()

declare -a files_from_cmd
declare -a files_from_git

pathrex='[A-Za-z0-9/._-]+'  # regular expression for matching "sane" file names
verbose=0

function handle_cmd_args {
    local arg dd
    dd=0
    for arg in "$@"
    do
        if [ ${dd} -gt 0 ]
        then
            files_from_cmd+=("${arg}")
            continue
        fi
        case "${arg}" in
            --help)
                echo "${helptext}"
                exit
                ;;
            --version)
                basename "$0"
                exit
                ;;
            --verbose|-v)
                verbose+=1
                ;;
            --)
                dd=1
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
    printf_checking "Checking symbolic links"
    check_symlinks || status=1
    printf_checking "Checking cross-references in the README document"
    check_readme_crossrefs README.md || status=1
    printf_checking "Checking copyright notices in %d files" ${#filelist[@]}
    [ ${#filelist[@]} -eq 0 ] || ./maintainer/copyright "${chkflags_copyright[@]}" -- "${filelist[@]}" || status=1
    printf_checking "Checking %d files for white-space issues" ${#filelist[@]}
    [ ${#filelist[@]} -eq 0 ] || ./maintainer/whitespace "${chkflags_whitespace[@]}" -- "${filelist[@]}" || status=1
    printf_checking "Checking %d CMake files for style issues" ${#cmakefiles[@]}
    [ ${#cmakefiles[@]} -eq 0 ] || ./maintainer/cmake-style "${chkflags_cmake[@]}" -- "${cmakefiles[@]}" || status=1
    printf_checking "Checking %d BibTeX files for style issues" ${#bibtexfiles[@]}
    [ ${#bibtexfiles[@]} -eq 0 ] || check_bibtex_files "${bibtexfiles[@]}" || status=1
    return ${status}
}

function check_authors_file {
    [ $# -eq 1 ] || fatal "check_authors_file: Wrong number of arguments (this is a bug)"
    local user_name user_email expected
    user_name="$(git config user.name)" || notgood "Cannot query Git for user.name setting"
    user_email="$(git config user.email)" || notgood "Cannot query Git for user.email setting"
    expected="$(printf '"%s" <%s>' "${user_name}" "${user_email}")"
    [ -r "$1" ] || notgood "$1: File is not readable"
    sed -e 's;#.*;;g' "$1" | grep -q -F "${expected}" || notgood "$1: Please add yourself: ${expected}"
}

function check_symlinks {
    local status symlinks file target
    [ $# -eq 0 ] || fatal "check_symlinks: Wrong number of arguments (this is a bug)"
    status=0
    is_version_controlled '.msc-graphstudy' || fatal "Git reports questionable information"
    mapfile -t symlinks < <("${find_symlinks_cmd[@]}")
    for file in "${symlinks[@]}"
    do
        is_version_controlled "${file}" || continue
        target="$(readlink -e "${file}")"                                                                               \
            || notgood "${file}: Broken symbolic link"                                                                  \
            || { status=1; continue; }
        is_version_controlled "${target}"                                                                               \
            || notgood "${file}: Target of symlink not under version control: ${target}"                                \
            || { status=1; continue; }
    done
    return ${status}
}

function check_bibtex_files {
    local status file fixed
    status=0
    for file in "$@"
    do
        info "${file}: Checking formatting of BibTeX file ..."
        fixed="${file%.bib}_bibertool.bib"
        if "${BIBER:-biber}" --noconf --nolog --onlylog "${biber_tool_options[@]}" --tool "${file}"                     \
                && sed -e 's;^\s*%.*;;g' "${file}" | diff --ignore-blank-lines -q - "${fixed}" >/dev/null
        then
            rm -f "${fixed}"
        elif [ -f "${fixed}" ]
        then
            notgood "${file}: Please format like ${fixed}" || status=1
        else
            notgood "${file}: Biber cannot process this file (syntax error?)" || status=1
        fi
    done
    return ${status}
}

function check_readme_crossrefs {
    local status                                                                                                        \
          file lines otherlines i line ref_file ref_line ref_line_0 actual expected                                     \
          xrefs_decl xrefs_used xref location
    status=0
    for file in "$@"
    do
        if mapfile -t lines < "${file}"
        then
            info "${file}: Checking cross-references in README document ..."
        else
            notgood "${file}: Cannot map README file" || status=1
            continue
        fi
        declare -A xrefs_decl
        declare -A xrefs_used
        i=0
        for line in "${lines[@]}"
        do
            i=$((i + 1))
            if [[ "${line}" =~ '('(./${pathrex})'#L'([[:digit:]]+)')' ]]
            then
                xrefs_used["${BASH_REMATCH[1]}:${BASH_REMATCH[2]}"]="${file}:${i}"
            elif [[ "${line}" =~ '('(./${pathrex})')' ]]
            then
                xrefs_used["${BASH_REMATCH[1]}"]="${file}:${i}"
            fi
            if [[ "${line}" =~ '<!--'[[:space:]]*'Cross-Reference:'[[:space:]]*(./${pathrex})':'[[:space:]]*([[:alnum:]]+)[[:space:]]*'-->' ]] #
            then
                ref_file="${BASH_REMATCH[1]}"
                xrefs_decl["${ref_file}"]="${file}:${i}"
                case "${BASH_REMATCH[2]}"
                in
                    FILE)
                        [ -f "${BASH_REMATCH[1]}" ] && info "${file}:${i}: Confirmed cross-reference to ${ref_file}"    \
                                || notgood "${file}:${i}: Referenced file does not exist: ${ref_file}" || status=1
                        ;;
                    DIRECTORY)
                        [ -d "${BASH_REMATCH[1]}" ] && info "${file}:${i}: Confirmed cross-reference to ${ref_file}"    \
                                || notgood "${file}:${i}: Referenced directory does not exist: ${ref_file}" || status=1
                        ;;
                    *)
                        notgood "${file}:${i}: Unknown reference type: ${BASH_REMATCH[2]}" || status=1
                        ;;
                esac
            elif [[ "${line}" =~ '<!--'[[:space:]]*'Cross-Reference:'[[:space:]]*(./${pathrex})':'([[:digit:]]+)':'[[:space:]]*'"'(.*)'"'[[:space:]]*'-->' ]] #
            then
                ref_file="${BASH_REMATCH[1]}"
                ref_line="${BASH_REMATCH[2]}"
                expected="${BASH_REMATCH[3]}"
                xrefs_decl["${ref_file}:${ref_line}"]="${file}:${i}"
                if [ -f "${ref_file}" ] && [ -r "${ref_file}" ]
                then
                    mapfile -t otherlines < "${ref_file}" || notgood "${ref_file}: Cannot map file"
                    ref_line_0=$((ref_line - 1))  # adjust for zero-based indexing
                    actual="${otherlines[${ref_line_0}]-}"
                    if [ "${expected}" = "${actual}" ]
                    then
                        info "${file}:${i}: Confirmed cross-reference to ${ref_file} (line ${ref_line})"
                    else
                        status=1
                        notgood "${file}:${i}: Broken cross-reference to ${ref_file} (line ${ref_line})" || :
                        notgood "${ref_file}:${ref_line}: Expected text: ${expected}" || :
                        notgood "${ref_file}:${ref_line}: Actual text:   ${actual}"   || :
                    fi
                else
                    notgood "${file}:${i}: Referenced file does not exist: ${ref_file}" || status=1
                fi
            elif [[ "${line}" =~ '<!--'.*'-->' ]] && [[ "${line}" =~ [Cc]ross-[Rr]eference[:]? ]]
            then
                notgood "${file}:${i}: Cannot parse cross-reference marker: ${line}" || status=1
            fi
        done
        for xref in "${!xrefs_used[@]}"
        do
            location="${xrefs_used[${xref}]}"
            [ "x${xrefs_decl[${xref}]+set}" = xset ]                                                                    \
                || notgood "${location}: Cross-reference to $(parse_location ${xref}) used but never declared"          \
                || status=1
        done
        for xref in "${!xrefs_decl[@]}"
        do
            location="${xrefs_decl[${xref}]}"
            [ "x${xrefs_used[${xref}]+set}" = xset ]                                                                    \
                || info "${location}: Cross-reference to $(parse_location ${xref}) declared but never used"
        done
    done
    return ${status}
}

function parse_location {
    [ $# -eq 1 ] || fatal "parse_location: Wrong number of arguments (this is a bug)"
    if [[ "$1" =~ (${pathrex})':'([[:digit:]]+) ]]
    then
        echo "${BASH_REMATCH[1]} (line ${BASH_REMATCH[2]})"
    elif [[ "$1" =~ (${pathrex}) ]]
    then
        echo "${BASH_REMATCH[1]}"
    else
        echo '???'
        return 1
    fi
}

function fatal {
    echo "pre-commit.sh: error: $*" >&2
    exit 1
}

function notgood {
    echo "pre-commit.sh: $*" >&2
    return 1
}

function info {
    [ ${verbose} -gt 0 ] || return 0
    echo "pre-commit.sh: $*" >&2
}

function printf_checking {
    local formatstring="$1"
    shift
    printf "$(tput bold) ==> ${formatstring} ...$(tput sgr0)\n" "$@" >&2
}

function is_regular_file {
    [ -f "$1" ] && [ ! -L "$1" ]
}

function is_version_controlled {
    git ls-files --error-unmatch -- "$@" 1>/dev/null 2>/dev/null
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
