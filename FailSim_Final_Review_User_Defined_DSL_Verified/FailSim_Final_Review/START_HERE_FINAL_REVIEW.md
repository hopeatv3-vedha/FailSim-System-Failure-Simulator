# FailSim — Final Review Build

This package contains the complete FailSim source project, dashboard, sample DSL architectures, test suite, and Windows/Linux launch scripts.

## Windows: start the dashboard

Requirements: Python 3 and GCC. The launcher checks for both and builds the C program before starting the dashboard.

1. Extract this ZIP into a normal folder (not inside the ZIP preview).
2. Open PowerShell in the extracted `FailSim_Final_Review` folder.
3. If you want AI advice, set the key in that same PowerShell session (never paste it into the source or chat):

   ```powershell
   $env:GEMINI_API_KEY = "YOUR_ACTUAL_API_KEY"
   $env:GEMINI_MODEL = "gemini-3.8-flash"
   ```

4. Start the app:

   ```powershell
   .\run_tool.bat
   ```

5. The browser should open at `http://localhost:8091`. If it does not, open that address manually.

If you do not configure a Gemini key, the compiler, graph, simulation, impact analysis, and DSL-defined recovery still work; only the AI advisor is unavailable.

## Final-review demo sequence

1. Use `examples/ecommerce_v2.fail` as the prepared backup DSL. Clear the editor and paste its contents if you want to demonstrate the DSL live.
2. Click **Compile & load**. If you clear the editor or edit the DSL, the green PASS badge changes to EMPTY or NOT COMPILED until the current text is successfully compiled. Empty source is rejected instead of silently loading the example.
3. Select `OrderDB` in the failure-injection chips, then click **Inject selected failure(s)** / **Run simulation** as shown in the interface.
4. Explain the mandatory `OrderService -> OrderDB` dependency, the failure path, and the pre-recovery impact (20% for the sample's 10 components).
5. Show the rule-based recovery: `BackupDB` activates and `OrderService` redirects to it. `OrderDB` remains failed, so the final system status can remain `DEGRADED`.
6. Explain risk candidates as graph-based indicators from the declared dependencies, not live production outage predictions.
7. Click **Suggest recovery with AI** to show architecture-specific Gemini recommendations. They are advisory and are not automatically applied.
8. The predefined-scenario menu includes `OrderDatabaseFailure` (one failure) and `MultiFailure` (OrderDB and PaymentDB together). You can also click multiple component chips to inject multiple failures in one run.
9. For a quick second case, select `WebApp`. In this sample, it is a standalone failure with 10% impact and no dependency-propagation path; no recovery rule is defined for it. This is an expected edge case, not the main demo.

## Checks performed on this build

- C compiler build with `-std=c11 -Wall -Wextra -pedantic`.
- 21 automated Python regression tests passed, including empty-DSL rejection and single-/multi-failure scenario checks.
- `OrderDB` failure/recovery scenario passed: 20% impact, BackupDB activation, OrderService redirect, original OrderDB remains failed.
- `WebApp` standalone failure passed: 10% impact, no direct/indirect propagation, no matching recovery rule.
- 120-component stress DSL loaded successfully.
- ZIP integrity checked.

A live Gemini API call depends on your local key, network, model access, and current quota; this package's automated tests mock the API and do not guarantee live service availability.


## Demonstrating a user-defined system (not only e-commerce)

To prove that FailSim is driven by the DSL rather than hard-coded to e-commerce:

1. In the editor, click **Load example** only if you want to restore the e-commerce sample; otherwise open `examples/library_system.fail` and paste its contents or use the file chooser.
2. Click **Compile & Load**. The system name should become `LibraryPlatform` and the component inventory should show ReaderAPI, CatalogDB, CatalogReplica, SearchCache, and IndexWorker.
3. Inject `CatalogDB` failure. The simulator should show the mandatory dependents affected and the configured recovery should activate CatalogReplica and redirect ReaderAPI.
4. Try the predefined `DatabaseAndCacheFailure` scenario, or select CatalogDB and SearchCache together manually, to demonstrate multiple simultaneous failures.
5. To test a custom DSL with no predefined scenarios, define `SYSTEM`, at least one `COMPONENT`, and any relationships; compile it, then select failure chips manually.

Important scope note: FailSim supports any architecture expressible in its documented DSL and supported component types. It does not automatically inspect live infrastructure or model features not represented in the DSL.
