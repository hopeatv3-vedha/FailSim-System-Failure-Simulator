# FailSim — Final Review Build

FailSim is a compiler-based system failure and recovery simulator. Users describe an architecture in a custom `.fail` domain-specific language (DSL). The C program performs lexical analysis, parsing and AST construction, semantic analysis, system-model creation, failure propagation, and recovery. The web interface lets users edit or upload a DSL file, compile/load it, select one or more failures, inspect the dependency graph, and compare simulation state with post-recovery state.

## Requirements

- GCC with C11 support; on Windows, MSYS2 UCRT64 GCC is supported.
- Python 3.9 or newer.
- A modern browser.

## Run on Windows

1. Extract the ZIP and open the project folder in VS Code.
2. Run `run_tool.bat` from File Explorer or the VS Code terminal.
3. The launcher rebuilds the C program and starts the local web server. It opens `http://localhost:8091` in your browser.
4. Keep the terminal open while using the dashboard. Press Ctrl+C to stop the server.

## Run on Linux or macOS

```bash
./run_tool.sh
```

The launcher builds the C program and opens `http://localhost:8091`.

## Optional AI Recovery Advisor (Google Gemini API)

The dashboard includes an optional AI Recovery Advisor. The C compiler and deterministic simulator remain authoritative: the server validates and simulates the submitted `.fail` architecture first, then sends a compact architecture/simulation context to the Google Gemini `generateContent` API. The AI never edits or applies the architecture automatically.

### Configure on Windows PowerShell

1. Create a Gemini API key in [Google AI Studio](https://aistudio.google.com/apikey). Check that your chosen model is available to your account and review free-tier limits. Free-tier availability and quotas can change. Do not send sensitive production details because data handling on the free tier may differ from paid services.
2. In the same PowerShell window you use to launch FailSim, set the key (replace the placeholder locally; never paste your key into source code or share it):

   ```powershell
   $env:GEMINI_API_KEY = "YOUR_GEMINI_API_KEY_HERE"
   # Optional: select a model available to your AI Studio project
   $env:GEMINI_MODEL = "gemini-3.8-flash"
   .\run_tool.bat
   ```

### Configure on Linux/macOS

```bash
export GEMINI_API_KEY="YOUR_GEMINI_API_KEY_HERE"
export GEMINI_MODEL="gemini-3.8-flash"  # optional
./run_tool.sh
```

3. In the dashboard, compile the architecture, select a failure, and click **Suggest recovery with AI**. The server calls Gemini only when you explicitly request advice.
4. If the key is missing, invalid, the model is unavailable, or the free-tier quota is exhausted, the dashboard displays a configuration/error message. The compiler, simulator and deterministic recovery still work without AI access.

The API key stays on the Python server and is not sent to browser JavaScript. Architecture source and simulation evidence are sent to Gemini when the user requests advice. Do not submit passwords, tokens, personal data, or sensitive production configuration in `.fail` files. Review recommendations with an engineer; they are suggestions, not guaranteed fixes. AI tests use mocked API responses and do not prove a live API key/model works; perform a live test before the review.

## CLI compiler demonstration

```bash
gcc -std=c11 -Wall -Wextra -pedantic src/*.c -o failsim
./failsim examples/ecommerce_v2.fail
```

On Windows, the equivalent executable is `failsim.exe`; run it with `examples/ecommerce_v2.fail` as its input.

The web interface uses the C executable's structured JSON interface for both compilation and simulation. Example:

```bash
./failsim --json examples/ecommerce_v2.fail OrderDB
```

The browser submits the current DSL and selected failure names to the C executable; the simulation is not a hard-coded OrderDB/Redis result.

## Dashboard workflow

1. Edit the FailSim DSL in the editor or upload a `.fail` file.
2. Click **Compile & load**. Invalid syntax and semantic references are rejected by the C compiler.
3. Inspect the component inventory and dependency graph.
4. Select one or more failures or choose a named scenario from the DSL.
5. Click **Run simulation** to see failed, directly affected, indirectly affected, and degraded components.
6. Inspect impact, affected groups, and failure paths.
7. Review recovery actions and post-recovery state. The original failed component remains failed; activating a backup does not falsely mark the original database healthy.
8. Upload or write another architecture and repeat.

## DSL quick reference

```text
SYSTEM ECommerce
GROUP CoreServices
GROUP DataLayer

COMPONENT OrderService TYPE SERVICE
    GROUP CoreServices
    STATE RUNNING
    CRITICALITY HIGH
END

COMPONENT OrderDB TYPE DATABASE
    GROUP DataLayer
    STATE RUNNING
    CRITICALITY CRITICAL
END

COMPONENT Redis TYPE CACHE
    GROUP DataLayer
    STATE RUNNING
    CRITICALITY MEDIUM
END

COMPONENT Gateway TYPE GATEWAY
    GROUP CoreServices
    STATE RUNNING
    CRITICALITY HIGH
END

COMPONENT BackupDB TYPE BACKUP
    GROUP DataLayer
    STATE STANDBY
    CRITICALITY HIGH
END

DEPENDS OrderService ON OrderDB
OPTIONAL OrderService ON Redis
CONNECTS Gateway TO OrderService

RECOVERY OrderDB
    USE BackupDB
    REDIRECT OrderService TO BackupDB
END

SCENARIO DatabaseFailure
    FAIL OrderDB
END
```

Component types: `CLIENT CDN GATEWAY LOAD_BALANCER AUTH SERVICE DATABASE CACHE QUEUE WORKER STORAGE EXTERNAL_API MONITOR BACKUP`.

States: `RUNNING STOPPED STANDBY FAILED`.

Criticalities: `LOW MEDIUM HIGH CRITICAL`.

- `DEPENDS A ON B`: mandatory dependency; failure of B can affect A.
- `OPTIONAL A ON B`: optional dependency; failure of B degrades A instead of failing it through this edge alone.
- `CONNECTS A TO B`: communication relationship; shown in the graph but does not itself propagate failure.
- `RECOVERY A`: policy for failure of A. `USE` activates a backup; `REDIRECT` reroutes a component.
- `SCENARIO`: named list of `FAIL` statements.

## Tests

Run from the project root:

```bash
python -m unittest discover -s tests -v
```

Regression tests exercise the C-backed web simulation, mandatory propagation, optional degradation, multi-failure behavior, recovery/redirection, semantic error reporting, and a custom architecture. `examples/stress_120.fail` is a larger 120-component CLI test.

## Architecture

```text
FailSim DSL → C Lexer → C Parser + AST → C Semantic Analysis
           → C System Model / IR → C Failure Simulator → C Recovery Engine
           → JSON result → Web dashboard
```

The Python layer serves the dashboard and presents results; the C executable is invoked for validation and simulation. FailSim analyzes the architecture supplied by the user. It does not discover hidden dependencies or predict actual production outages.


For the final-review setup and demo checklist, start with `START_HERE_FINAL_REVIEW.md`.


## Generic user-defined DSL example

Besides the e-commerce demo, `examples/library_system.fail` demonstrates that system names, component names, dependencies, recovery policies, and failure scenarios come from the `.fail` file. See `docs/V2_SPEC.md` for the supported grammar and limitations.
