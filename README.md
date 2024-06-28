# Miscellaneous Utils Collection

This repository contains a collection of miscellaneous utilities I developed for an embedded project that couldn't leverage the open-source Linux utilities. Each utility is designed for a specific purpose and is self-contained, including the corresponding test code and test data used for validation.

Feel free to use and enhance these utilities as you see fit. If you would like to contribute enhancements or bug fixes, I welcome your contributions and will gladly credit you for them.

https://www.linkedin.com/in/praveen-yaramada-a651003/

## c_unescape.c

This utility unescapes C-style strings with escape characters. For reference on escape sequences, see [C++ Escape Sequences](https://en.cppreference.com/w/cpp/language/escape).

### How It Works

The parser marks each character with a state that is suitable for processing the next character. It handles most C-style escape sequences, but it does not support the Unicode escape sequences introduced in C++11.

If the parser encounters an incomplete or arbitrarily terminated escape sequence, it unescapes the preceding characters and writes them to the output buffer.

## shell_token.c

This utility extracts ash-compatible tokens from a shell input command string. It processes commands with parameters, I/O redirection, and control operators.

Form of input string comprises of 

    <command> [ params][redirecton][control operators]

### Features

**Token Extraction:** Splits a simple form of shell input command string into commands, parameters, I/O redirection, and control operators.

**Token Separators:** Handles blanks (spaces and tabs), control operators (&, |, &&, ||, ;), and redirection operators (>, >>, <, <>).

**Quoting Handling:** Supports token separators within matching double or single quotes.

## simplify_path.c

This utility reduces a POSIX absolute path by simplifying it in place. The input must be a valid null-terminated string that begins with the root directory (/).

### Features

1. Simplifies an absolute POSIX path, handling redundant and unnecessary elements such as . (current directory) and .. (parent directory).
2. In-Place Operation: Modifies the input path directly without requiring additional memory allocation.

### Requirement

The input must be a valid null-terminated string starting with the root directory (/).

## b64.c

This file provides utilities for Base64 encoding and decoding as specified in [RFC 4648](https://datatracker.ietf.org/doc/html/rfc4648)

### Features

**Base64 Encoding:** Encodes an input buffer to an output string buffer and null-terminates the output string.

**Base64 Decoding:** Decodes an input string buffer to an output buffer.

## rpm2tar.sh

This convenient shell script converts an RPM package into a tar.gz archive, similar to the rpm2archive utility.

    ./rpm2tar.gz.sh <rpmfile>

## parenth.c

This utility checks for matching braces in a string and removes unmatched braces to make the string valid. It was implemented as a programming challenge for LeetCode.

