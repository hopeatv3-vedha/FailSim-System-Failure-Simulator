# FailSim — Final Review Demonstration

## Suggested 5-minute demo

1. Open `examples/ecommerce_v2.fail`. Explain `SYSTEM`, `GROUP`, `COMPONENT`, `DEPENDS`, `OPTIONAL`, `CONNECTS`, `RECOVERY`, `SCENARIO`, and `FAIL`.
2. Show compiler source: `src/lexer.c`, `src/parser.c`, `src/ast.c`, `src/semantic.c`, and `src/model.c`. Explain that `main.c` connects the stages.
3. Run `./failsim examples/ecommerce_v2.fail` on Linux/macOS, or run `failsim.exe` with `examples/ecommerce_v2.fail` on Windows. Show the lexer, parser/AST, semantic-analysis and system-model pass messages, then failure and recovery reports.
4. Start the dashboard with `run_tool.bat` on Windows or `./run_tool.sh` on Linux/macOS. Open `http://localhost:8091` if it does not open automatically.
5. Click **Compile & load**, select `OrderDB`, then click **Run simulation**. Expected: `OrderDB` is FAILED, `OrderService` is directly affected, impact is 20% for this 10-component example, and recovery activates `BackupDB` and redirects `OrderService`. The graph and inventory show post-recovery component states; the failure-analysis section retains the pre-recovery impact and paths.
6. Explain recovery truthfully: the original `OrderDB` remains failed; the backup is activated and the service is redirected. Post-recovery status is DEGRADED because the original database component is still down.
7. Upload or edit another `.fail` architecture and compile it. The component inventory and graph should reflect the new source. An invalid dependency should produce a compiler error.
8. Mention the larger test: `./failsim examples/stress_120.fail` processes the 120-component example.

## Suggested explanation

“FailSim is a compiler-based system failure simulator. A user describes an architecture using our custom DSL. The C compiler tokenizes and parses the source, builds an AST, validates component references and dependencies, and builds the system model. The C simulation engine applies selected failures, calculates mandatory propagation and optional degradation, and applies recovery rules defined in the DSL. The web dashboard is a front end to that C-backed workflow, not a hard-coded E-Commerce-only simulation.”

## Expected OrderDB scenario

- Failed component: `OrderDB`
- Direct impact: `OrderService`
- `Redis` remains healthy for this scenario.
- Recovery actions: activate `BackupDB`; redirect `OrderService` to `BackupDB`.
- The original `OrderDB` remains failed; post-recovery status is DEGRADED.
- Impact counts each affected component once, even if the same component is reached by more than one dependency type.

## Scope note

FailSim analyzes the architecture and recovery policies supplied by the user. It does not discover unknown dependencies or predict actual live outages. E-Commerce is a sample architecture, not a hard-coded limitation.


## Optional AI Recovery Advisor (Gemini)

1. Create a key in [Google AI Studio](https://aistudio.google.com/apikey). Check the current free-tier quota and model access.
2. In PowerShell, in the project folder, set `$env:GEMINI_API_KEY = "YOUR_KEY"` and optionally `$env:GEMINI_MODEL = "gemini-3.8-flash"`.
3. Start `run_tool.bat`, load `examples/ecommerce_v2.fail`, select `OrderDB`, run the simulation, then click **Suggest recovery with AI**.
4. The AI advice is separate from the deterministic recovery result and is not applied automatically. If the API is unavailable, demonstrate the existing `OrderDB` to `BackupDB` rule-based recovery.
5. Do not include secrets or sensitive production information in the DSL. Never share or commit the API key.
