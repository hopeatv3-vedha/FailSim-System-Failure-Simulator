# FailSim regression tests

Run from the project root:

```bash
python -m unittest discover -s tests -v
```

Tests call the same C JSON interface used by the web server and check:
- OrderDB failure propagation and backup/redirection recovery
- Optional-dependency degradation
- Multiple simultaneous failures
- Rejection of unknown failure selections
- C semantic error reporting for unknown dependency references
- Compilation and simulation of a separate small architecture

The larger `examples/stress_120.fail` input is tested separately through the CLI.

AI advisor tests mock Gemini API responses; they do not consume API quota and do not validate a real API key or model. Test a live request manually before presenting the optional AI feature.
