# Observability Plan

ShortHand release-level branches should make runtime and evidence behavior easy to inspect.

## Minimum pilot signals

- compiler command
- input file path
- selected execution mode
- backend selection result
- fallback reason when fallback is used
- evidence report path
- validation result
- error message and exit code

## Future metrics

- compile duration
- runtime duration
- inference duration
- token count where applicable
- memory peak
- energy source status
- report generation duration

## Current status

The current repository already records fallback and evidence output in validation flows. Structured runtime metrics remain future work.
