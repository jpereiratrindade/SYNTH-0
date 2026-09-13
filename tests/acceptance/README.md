# Manual acceptance gates

After `./bin/bootstrap-dev`:

```bash
tests/integration/lifecycle.sh
tests/resilience/kill-recovery.sh
tests/resource/queue-saturation.sh
```

Contract-version mismatch is represented by the planner state `BLOCKED`; the first root patch includes the evidence fixture under `tests/contract/fixtures/`. Runtime mismatch injection is intentionally deferred until it can be introduced without teaching a domain system to read controller-private test state.
