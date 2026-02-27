# Security Policy

## Supported versions

This project is pre-1.0. Security fixes are applied to `main`.

## Reporting a vulnerability

Please report vulnerabilities privately through GitHub Security Advisories:
- `Security` tab
- `Report a vulnerability`

If unavailable, open a private issue with:
- Impact summary
- Reproduction steps
- Suggested remediation (optional)

Do not post zero-day details in public issues.

## Scope

Security issues include:
- Memory safety bugs leading to crash or code execution.
- Unsafe parsing behavior for untrusted input.
- Supply-chain risks in CI/dependencies.

## Security automation

The repository runs:
- CodeQL analysis for C/C++.
- Secret scanning.
- SBOM generation and vulnerability scanning.
