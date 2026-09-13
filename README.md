# SYNTH-0

SYNTH-0 is a deliberately small synthetic ecosystem for testing declarative
relationships among independently evolvable systems.

Its founding distinction is:

```text
declaration != observation
```

The three synthetic systems are:

- **Pulso** — produces temporal facts.
- **Trama** — derives patterns from Pulso without owning Pulso's facts.
- **Lente** — produces projections and explicit intentions without writing
  another system's private state.

The repository is a physical monorepo only for bootstrap convenience. Public
contracts are the boundary; private implementation is not shared.

## Host assumptions

The development target is Linux with:

- GCC with C++26 support;
- CMake >= 3.30;
- Ninja;
- OpenSSL development files;
- Git;
- systemd user services;
- Node.js + npm for the web experience.

Third-party C++ libraries are fetched by CMake at pinned tags:
`nlohmann/json`, `toml++`, and `cpp-httplib`.

## Bootstrap

From the repository root:

```bash
./bin/bootstrap-dev
./bin/synth run --target dev
```

The second lifecycle execution should converge without applying changes:

```bash
./bin/synth run --target dev
```

and report `NO_OP`.

Start the factual web projection:

```bash
systemctl --user start synth0-controller.service
```

Then open:

```text
http://127.0.0.1:9180
```

## Disturbance experiment

```bash
./bin/synth disturb kill pulso
./bin/synth status
./bin/synth run --target dev
```

The expected sequence is factual degradation, repair, re-observation and a
post-convergence `KEEP`.

## Architectural documents

- `docs/SYNTH-ARCH-001-v0.1.0.md`
- `docs/SYNTH-BOOTSTRAP-001-v0.1.0.md`

## License

SYNTH-0 is licensed under the GNU General Public License v3.0
(`GPL-3.0-only`). See [LICENSE](LICENSE).
