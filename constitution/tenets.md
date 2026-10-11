# Tenets

These are the rules the referee enforces and no agent, app or committee vote can override.

1. **No purpose, no operating system.**  Nothing exists in the system unless it traces to a stated purpose.  A machine can have several purposes; each is written down.
2. **One door.**  The only way anything enters the system is an approved petition: apps, libraries, drivers, services, agents, tests, telemetry, the core itself.
3. **Source only.**  Nothing enters as pre-compiled code.  Everything is built from reviewed source, reproducibly, down to a bootstrappable toolchain.
4. **Reviewed before entry.**  Every component is reviewed against this machine's purpose and invariants before it is admitted.  Review is cumulative and collective, never a claim of "proven safe."
5. **No steady-state dependency on anyone else.**  Once source is inside, we own it.  Upstream is a supplier of suggestions that come through the same door, never an authority we wait on for a fix.
6. **Mechanism and policy stay apart.**  The OS in the centre provides levers.  It never decides when to pull them.
7. **Intelligence and authority never share a body.**  Agents propose; the referee decides; the referee has no intelligence to be fooled.
8. **A test is written to fail.**  A failed test is the most valuable signal in the system.  A sensor or test that cannot go red is decoration.
9. **Nothing is self-reported.**  No agent's certainty counts.  Confidence is earned by track record and judged by a committee with competing priorities.
10. **Everything expires.**  Petitions carry a sunset.  Unused grants are revoked and the component stripped.
