#! /usr/bin/env python3

# This script produces a code quality report in Code Climate JSON format
# suitable for use in Gitlab CI/CD pipeline. See the links for more details:
# https://docs.gitlab.com/ee/user/project/merge_requests/code_quality.html#implementing-a-custom-tool
# https://github.com/codeclimate/platform/blob/master/spec/analyzers/SPEC.md

# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring
# pylint: disable=missing-module-docstring
# pylint: disable=no-else-return

import dataclasses
import enum
import hashlib
import itertools
import json
import re
import sys
import typing


PATH_PREFIX = sys.argv[1] if len(sys.argv) > 1 else ""

PATTERN_WARN = re.compile(fr"^{PATH_PREFIX}(?P<path>.+):(?P<line>\d+):(?P<column>\d+): warning: (?P<text>.+ \[(?P<check_name>.+)\])$")  # noqa: E501, pylint: disable=line-too-long
PATTERN_WARN_START = re.compile(fr"^{PATH_PREFIX}(?P<path>.+):(?P<line>\d+):(?P<column>\d+): warning: (?P<text>.+)$")  # noqa: E501, pylint: disable=line-too-long
PATTERN_WARN_END = re.compile(r"^(?P<text>.+ \[(?P<check_name>.+)\])$")
PATTERN_NOTE = re.compile(r"^(?P<path>.+):(?P<line>\d+):\d+: note:.*$")


class Severity(enum.Enum):
    BLOCKER     = 5, "blocker"  # noqa: E221
    CRITICAL    = 4, "critical"  # noqa: E221
    MAJOR       = 3, "major"  # noqa: E221
    MINOR       = 2, "minor"  # noqa: E221
    INFO        = 1, "info"  # noqa: E221


class Category(enum.Enum):
    BUG_RISK        = "Bug Risk"  # noqa: E221
    CLARITY         = "Clarity"  # noqa: E221
    COMPATIBILITY   = "Compatibility"  # noqa: E221
    COMPLEXITY      = "Complexity"  # noqa: E221
    DUPLICATION     = "Duplication"  # noqa: E221
    PERFORMANCE     = "Performance"  # noqa: E221
    SECURITY        = "Security"  # noqa: E221
    STYLE           = "Style"  # noqa: E221


SEVERITY_MAPPING = (
    # Explicitly defined severities for some checks. These must go before
    # the catch-all patterns to take precedence during check name matching.
    ("cppcoreguidelines-init-variables",                Severity.CRITICAL),
    ("cppcoreguidelines-interfaces-global-init",        Severity.CRITICAL),
    ("cppcoreguidelines-owning-memory",                 Severity.CRITICAL),
    ("cppcoreguidelines-pro-type-member-init",          Severity.CRITICAL),
    ("cppcoreguidelines-pro-type-static-cast-downcast", Severity.CRITICAL),
    ("cppcoreguidelines-slicing",                       Severity.CRITICAL),
    # Common catch-all patterns.
    ("bugprone-",           Severity.MAJOR),
    ("clang-analyzer-",     Severity.BLOCKER),
    ("clang-diagnostic-",   Severity.INFO),
    ("concurrency-",        Severity.BLOCKER),
    ("cppcoreguidelines-",  Severity.MAJOR),
    ("misc-",               Severity.MINOR),
    ("modernize-",          Severity.MINOR),
    ("performance-",        Severity.MAJOR),
    ("portability-",        Severity.MAJOR),
    ("readability-",        Severity.MINOR),)


CATEGORY_MAPPING = (
    ("bugprone-",           Category.BUG_RISK),
    ("clang-analyzer-",     Category.SECURITY),
    ("clang-diagnostic-",   Category.SECURITY),
    ("concurrency-",        Category.SECURITY),
    ("cppcoreguidelines-",  Category.BUG_RISK),
    ("misc-",               Category.BUG_RISK),
    ("modernize-",          Category.STYLE),
    ("performance-",        Category.PERFORMANCE),
    ("portability-",        Category.COMPATIBILITY),
    ("readability-",        Category.CLARITY),)


@dataclasses.dataclass(frozen=True)
class ParserState:
    path: str = ""
    line: str = ""
    column: str = ""
    text: str = ""
    check_name: str = ""
    code: str = ""


def make_issue(state: ParserState) -> dict:
    # We do not include line number in the fingerprint in order to
    # preserve it in case the faulty code is moved to another line.
    fingerprint = state.path + state.column + state.text + state.code
    fingerprint_hash = hashlib.md5(fingerprint.encode('utf-8')).hexdigest()
    # There can be multiple names in the check name list if aliases are used.
    check_names = state.check_name.split(",")
    return {
        'type': "issue",
        'check_name': state.check_name,
        'description': state.text,
        'content': {
            'body': f"```\n{state.code}\n```"},
        'categories': get_categories(check_names),
        'location': {
            'path': state.path,
            # We could use 'lines' instead of 'positions' because this is all
            # that Gitlab CI needs. However we need column for deduplication.
            'positions': {
                'begin': {
                    'line': int(state.line),
                    'column': int(state.column)},
                # We use the same begin and end position. clang-tidy does not
                # emit end position, but this is required by the json schema.
                'end': {
                    'line': int(state.line),
                    'column': int(state.column)}}},
        'severity': get_severity(check_names),
        'fingerprint': fingerprint_hash}


T = typing.TypeVar("T")  # pylint: disable=invalid-name


def mapping_lookup(key: str, mapping: typing.Tuple[typing.Tuple[str, T], ...]) -> T:  # noqa: E501
    return next(value for pattern, value in mapping if pattern in key)


def get_severity(check_names: typing.List[str]) -> str:
    severities = (mapping_lookup(c, SEVERITY_MAPPING) for c in check_names)
    _, severity = max(s.value for s in severities)
    return severity


def get_categories(check_names: typing.List[str]) -> typing.List[str]:
    categories = (mapping_lookup(c, CATEGORY_MAPPING) for c in check_names)
    return list(c.value for c in categories)


def state_finished(_line, _state):
    return (state_finished, None, None)


def state_ignore_lines(line, _state):
    if line is None:
        return (state_finished, None, None)
    elif match := PATTERN_WARN.match(line):
        next_state = ParserState(**match.groupdict())
        return (state_read_code, next_state, None)
    elif match := PATTERN_WARN_START.match(line):
        next_state = ParserState(**match.groupdict())
        return (state_read_text, next_state, None)
    else:
        return (state_ignore_lines, None, None)


def state_read_code(line, state):
    if line is None:
        return (state_finished, None, state)
    elif match := PATTERN_WARN.match(line):
        next_state = ParserState(**match.groupdict())
        return (state_read_code, next_state, state)
    elif match := PATTERN_WARN_START.match(line):
        next_state = ParserState(**match.groupdict())
        return (state_read_text, next_state, state)
    elif PATTERN_NOTE.match(line):
        return (state_ignore_lines, None, state)
    elif line.startswith("note: this fix will not be applied"):
        return (state_ignore_lines, None, state)
    elif line.startswith("clang-tidy"):  # New file is being processed.
        return (state_ignore_lines, None, state)
    else:
        code = state.code + line + "\n"
        next_state = dataclasses.replace(state, code=code)
        return (state_read_code, next_state, None)


def state_read_text(line, state):
    if line is None:
        return (state_finished, None, None)
    elif match := PATTERN_WARN_END.match(line):
        text = state.text + match.group("text")
        check_name = match.group("check_name")
        next_state = dataclasses.replace(state, text=text, check_name=check_name)  # noqa: E501
        return (state_read_code, next_state, None)
    else:
        text = state.text + line
        next_state = dataclasses.replace(state, text=text)
        return (state_read_text, next_state, None)


def generate(lines):
    handler = state_ignore_lines
    next_state = None
    for line in itertools.chain(lines, [None]):
        handler, next_state, old_state = handler(line, next_state)
        if old_state is not None:
            yield old_state


def issues(states):
    prev_code = None
    for state in states:
        if not state.code:
            # Workaround for cases where two or more issues are reported for
            # the same code location. Location of the second issue is omited
            # in clang-tidy output but we need that for fingerprint calculation.
            state = dataclasses.replace(state, code=prev_code)
        prev_code = state.code
        yield make_issue(state)


def unique(issues):
    seen = set()
    for issue in issues:
        # We use source code location and check name for deduplication. Using
        # fingerprint would be no good as it does not include line number.
        # Also sometimes fingerprint is different for the same issue reported
        # from checking different TUs because faulty code may not be shown for
        # one of them.
        path = issue['location']['path']
        line = issue['location']['positions']['begin']['line']
        column = issue['location']['positions']['begin']['column']
        check_name = issue['check_name']
        key = (path, line, column, check_name)
        if key not in seen:
            seen.add(key)
            yield issue


def main():
    lines = (line.rstrip("\n") for line in sys.stdin)
    entries = unique(issues(generate(lines)))
    json.dump(list(entries), sys.stdout)


if __name__ == "__main__":
    main()
