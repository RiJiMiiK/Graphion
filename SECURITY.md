# Security Policy

## Supported versions

This project is pre-1.0.
Security fixes are applied on the active main line.

## Reporting a vulnerability

Use the Discord server for private contact:

- https://discord.com/invite/mPzDQ7TYkj

When reporting, include:

- impact summary
- reproduction steps
- any suspected scope or affected area

Do not post zero-day details in public issues.

## Scope

Security issues include:

- memory safety bugs leading to crashes or code execution
- unsafe parsing behavior for untrusted input
- supply-chain risks in CI or dependencies

## Security automation

The repository currently runs:

- CodeQL analysis for C/C++
- secret scanning
- SBOM generation and vulnerability scanning
