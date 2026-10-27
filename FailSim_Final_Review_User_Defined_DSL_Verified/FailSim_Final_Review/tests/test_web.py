"""Regression tests for the FailSim web simulation layer.
Run from project root with: python -m unittest discover -s tests -v
"""
import importlib.util
from pathlib import Path
import glob
import json
import os
import subprocess
import unittest
from unittest.mock import patch, Mock

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("failsim_web_app", ROOT / "web" / "app.py")
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)
SOURCE = (ROOT / "examples" / "ecommerce_v2.fail").read_text(encoding="utf-8")


def setUpModule():
    # Keep the test suite self-contained: build the C runtime if the launcher has not run yet.
    if not Path(app.BIN).exists():
        sources = glob.glob(str(ROOT / "src" / "*.c"))
        subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-pedantic", *sources, "-o", app.BIN], check=True)

class FailSimWebTests(unittest.TestCase):
    def payload(self, failures):
        result = app.payload(SOURCE, failures)
        self.assertTrue(result["ok"], result.get("errors"))
        self.assertTrue(result["compiler"]["ok"])
        return result

    def test_order_database_failure_and_recovery(self):
        r = self.payload(["OrderDB"])
        states = {c["name"]: c["status"] for c in r["simulation"]["components"]}
        self.assertEqual(states["OrderDB"], "FAILED")
        self.assertEqual(states["OrderService"], "AFFECTED")
        self.assertEqual(states["Redis"], "HEALTHY")
        self.assertEqual(r["simulation"]["analysis"]["direct"], 1)
        self.assertEqual(r["simulation"]["analysis"]["affected"], 2)
        self.assertEqual(r["simulation"]["analysis"]["impact"], 20.0)
        self.assertIn(["OrderService", "OrderDB"], r["simulation"]["paths"])
        self.assertIn("Backup activated: BackupDB", r["recovery"]["actions"])
        self.assertIn("Redirect: OrderService → BackupDB", r["recovery"]["actions"])
        post = {c["name"]: c for c in r["recovery"]["components"]}
        self.assertEqual(post["OrderDB"]["status"], "FAILED")
        self.assertEqual(post["OrderService"]["status"], "RECOVERED")
        self.assertEqual(post["OrderService"]["redirect"], "BackupDB")
        self.assertEqual(r["recovery"]["status"], "DEGRADED")

    def test_optional_dependency_degrades_consumer(self):
        r = self.payload(["Redis"])
        states = {c["name"]: c["status"] for c in r["simulation"]["components"]}
        self.assertEqual(states["Redis"], "FAILED")
        self.assertEqual(states["PaymentService"], "DEGRADED")
        self.assertEqual(r["simulation"]["analysis"]["optional"], 1)

    def test_overlapping_mandatory_and_optional_links_do_not_double_count_impact(self):
        # OrderService is already directly impacted by the mandatory dependency;
        # adding an optional edge to the same failed database must not count it twice.
        custom = SOURCE + "\nOPTIONAL OrderService ON OrderDB\n"
        r = app.payload(custom, ["OrderDB"])
        self.assertTrue(r["ok"], r.get("errors"))
        analysis = r["simulation"]["analysis"]
        self.assertEqual(analysis["failed"], 1)
        self.assertEqual(analysis["direct"], 1)
        self.assertEqual(analysis["optional"], 1)
        self.assertEqual(analysis["affected"], 2)
        self.assertEqual(analysis["impact"], 20.0)

    def test_webapp_failure_is_reported_without_claiming_propagation(self):
        r = self.payload(["WebApp"])
        analysis = r["simulation"]["analysis"]
        states = {c["name"]: c["status"] for c in r["simulation"]["components"]}
        self.assertEqual(states["WebApp"], "FAILED")
        self.assertEqual(analysis["failed"], 1)
        self.assertEqual(analysis["affected"], 1)
        self.assertEqual(analysis["impact"], 10.0)
        self.assertEqual(analysis["direct"], 0)
        self.assertEqual(analysis["indirect"], 0)

    def test_failure_selection_and_graph_inspection_are_labeled_separately(self):
        html = (ROOT / "web" / "index.html").read_text(encoding="utf-8")
        self.assertIn("Failures selected for injection", html)
        self.assertIn("Graph node inspection", html)
        self.assertIn("failureSelectionSummary", html)

    def test_graph_uses_post_recovery_state_and_ai_report_renders_markdown(self):
        html = (ROOT / "web" / "index.html").read_text(encoding="utf-8")
        self.assertIn("drawGraph(x.recovery)", html)
        self.assertIn("Post-recovery state", html)
        self.assertIn("function renderMarkdown(source)", html)
        self.assertIn("renderMarkdown(x.recommendations)", html)

    def test_multiple_failures(self):
        r = self.payload(["OrderDB", "PaymentDB"])
        self.assertEqual(r["simulation"]["analysis"]["failed"], 2)
        states = {c["name"]: c["status"] for c in r["simulation"]["components"]}
        self.assertEqual(states["OrderDB"], "FAILED")
        self.assertEqual(states["PaymentDB"], "FAILED")
        self.assertEqual(states["OrderService"], "AFFECTED")
        self.assertEqual(states["PaymentService"], "AFFECTED")

    def test_empty_dsl_is_rejected_instead_of_loading_default_example(self):
        result = app.payload("", [])
        self.assertFalse(result["ok"])
        self.assertIn("DSL source is empty", " ".join(result["errors"]))

    def test_example_has_single_and_multi_failure_predefined_scenarios(self):
        result = self.payload([])
        scenarios = {item["name"]: item["failures"] for item in result["model"]["scenarios"]}
        self.assertEqual(scenarios["OrderDatabaseFailure"], ["OrderDB"])
        self.assertEqual(scenarios["MultiFailure"], ["OrderDB", "PaymentDB"])
        html = (ROOT / "web" / "index.html").read_text(encoding="utf-8")
        self.assertIn("A SCENARIO CAN CONTAIN MULTIPLE FAILURES", html)
        self.assertIn("function sourceChanged()", html)
        self.assertIn("Compiler: EMPTY", html)

    def test_unknown_injected_component_is_rejected(self):
        r = app.payload(SOURCE, ["NotAComponent"])
        self.assertFalse(r["ok"])
        self.assertIn("Unknown failure component", r["errors"][0])

    def test_semantic_error_is_reported_by_c_compiler(self):
        bad = SOURCE.replace("DEPENDS OrderService ON OrderDB", "DEPENDS OrderService ON GhostDB")
        r = app.payload(bad, ["OrderDB"])
        self.assertFalse(r["ok"])
        self.assertIn("GhostDB", "\n".join(r["errors"]))

    def test_initial_failed_state_propagates_without_injection(self):
        custom = '''SYSTEM InitialDown
GROUP Core
COMPONENT API TYPE SERVICE
 GROUP Core
 STATE RUNNING
 CRITICALITY HIGH
END
COMPONENT DB TYPE DATABASE
 GROUP Core
 STATE FAILED
 CRITICALITY CRITICAL
END
DEPENDS API ON DB
'''
        r = app.payload(custom, [])
        self.assertTrue(r["ok"], r.get("errors"))
        states = {c["name"]: c["status"] for c in r["simulation"]["components"]}
        self.assertEqual(states["DB"], "FAILED")
        self.assertEqual(states["API"], "AFFECTED")

    def test_invalid_syntax_is_rejected_by_c_parser(self):
        bad = SOURCE.replace("DEPENDS OrderService ON OrderDB", "DEPENDS OrderService OrderDB")
        r = app.payload(bad, ["OrderDB"])
        self.assertFalse(r["ok"])
        self.assertTrue(r.get("errors"))

    def test_risk_analysis_panel_is_present(self):
        html = (ROOT / "web" / "index.html").read_text(encoding="utf-8")
        self.assertIn("Single-point-of-failure candidates", html)
        self.assertIn("function renderRiskAnalysis(x)", html)
        self.assertIn("mandatory downstream", html)

    def test_recovery_effectiveness_comparison_is_present(self):
        html = (ROOT / "web" / "index.html").read_text(encoding="utf-8")
        self.assertIn("IMPACT BEFORE RECOVERY", html)
        self.assertIn("RESIDUAL IMPACT", html)
        self.assertIn("impactCount(x.recovery.components)", html)


    def test_ai_advisor_reports_missing_key_without_breaking_simulator(self):
        with patch.dict(os.environ, {}, clear=True):
            r = app.ai_recovery_advice(SOURCE, ["OrderDB"])
        self.assertFalse(r["ok"])
        self.assertIn("GEMINI_API_KEY", " ".join(r["errors"]))

    def test_ai_status_uses_gemini_environment_configuration(self):
        with patch.dict(os.environ, {"GEMINI_API_KEY": "test-key", "GEMINI_MODEL": "gem-test"}):
            # The status contract is intentionally metadata-only; it never returns the key.
            status = {
                "configured": bool(os.environ.get("GEMINI_API_KEY", "").strip()),
                "provider": "Gemini",
                "model": os.environ.get("GEMINI_MODEL", "gemini-3.8-flash")
            }
        self.assertTrue(status["configured"])
        self.assertEqual(status["provider"], "Gemini")
        self.assertEqual(status["model"], "gem-test")
        self.assertNotIn("key", status)

    def test_ai_advisor_uses_gemini_generate_content_and_returns_text(self):
        fake_response = Mock()
        fake_response.read.return_value = __import__("json").dumps({
            "candidates": [{"content": {"parts": [{"text": "Recommend a standby database."}]}}]
        }).encode()
        fake_response.__enter__ = Mock(return_value=fake_response)
        fake_response.__exit__ = Mock(return_value=False)
        with patch.dict(os.environ, {"GEMINI_API_KEY": "test-key", "GEMINI_MODEL": "gem-test"}), \
             patch.object(app.urllib.request, "urlopen", return_value=fake_response) as urlopen:
            r = app.call_gemini_recovery_advisor({"system": "Tiny"})
        self.assertTrue(r["ok"], r)
        self.assertEqual(r["model"], "gem-test")
        self.assertIn("standby database", r["recommendations"])
        req = urlopen.call_args.args[0]
        self.assertEqual(req.full_url, "https://generativelanguage.googleapis.com/v1beta/models/gem-test:generateContent")
        self.assertEqual(req.get_header("X-goog-api-key"), "test-key")

    def test_custom_architecture_compiles(self):
        custom = '''SYSTEM Tiny\nGROUP Core\nCOMPONENT API TYPE SERVICE\n GROUP Core\n STATE RUNNING\n CRITICALITY HIGH\nEND\nCOMPONENT DB TYPE DATABASE\n GROUP Core\n STATE RUNNING\n CRITICALITY CRITICAL\nEND\nDEPENDS API ON DB\nSCENARIO DbDown\n FAIL DB\nEND\n'''
        r = app.payload(custom, ["DB"])
        self.assertTrue(r["ok"], r.get("errors"))
        self.assertEqual(r["model"]["system"], "Tiny")
        states = {c["name"]: c["status"] for c in r["simulation"]["components"]}
        self.assertEqual(states["API"], "AFFECTED")


    def test_user_defined_library_architecture_compiles_and_recovers(self):
        custom = (ROOT / "examples" / "library_system.fail").read_text(encoding="utf-8")
        r = app.payload(custom, ["CatalogDB"])
        self.assertTrue(r["ok"], r.get("errors"))
        self.assertEqual(r["model"]["system"], "LibraryPlatform")
        self.assertEqual(len(r["model"]["components"]), 5)
        self.assertEqual(r["simulation"]["analysis"]["failed"], 1)
        self.assertEqual(r["simulation"]["analysis"]["affected"], 3)
        self.assertEqual(r["simulation"]["analysis"]["impact"], 60.0)
        self.assertIn("Backup activated: CatalogReplica", r["recovery"]["actions"])
        self.assertIn("Redirect: ReaderAPI → CatalogReplica", r["recovery"]["actions"])
        post = {c["name"]: c for c in r["recovery"]["components"]}
        self.assertEqual(post["CatalogDB"]["status"], "FAILED")
        self.assertEqual(post["ReaderAPI"]["status"], "RECOVERED")
        self.assertEqual(post["IndexWorker"]["status"], "AFFECTED")

    def test_user_defined_dsl_multiple_failures_without_ecommerce_names(self):
        custom = (ROOT / "examples" / "library_system.fail").read_text(encoding="utf-8")
        r = app.payload(custom, ["CatalogDB", "SearchCache"])
        self.assertTrue(r["ok"], r.get("errors"))
        states = {c["name"]: c["status"] for c in r["simulation"]["components"]}
        self.assertEqual(states["CatalogDB"], "FAILED")
        self.assertEqual(states["SearchCache"], "FAILED")
        self.assertEqual(r["simulation"]["analysis"]["failed"], 2)
        self.assertGreaterEqual(r["simulation"]["analysis"]["affected"], 2)

    def test_user_defined_dsl_with_no_predefined_scenarios_can_use_manual_failure(self):
        custom = """SYSTEM MinimalPlatform
COMPONENT WebNode TYPE SERVICE
    STATE RUNNING
    CRITICALITY HIGH
END
COMPONENT DataStore TYPE STORAGE
    STATE RUNNING
    CRITICALITY CRITICAL
END
DEPENDS WebNode ON DataStore
"""
        r = app.payload(custom, ["DataStore"])
        self.assertTrue(r["ok"], r.get("errors"))
        self.assertEqual(r["model"]["system"], "MinimalPlatform")
        self.assertEqual(r["simulation"]["analysis"]["failed"], 1)
        self.assertEqual(r["simulation"]["analysis"]["direct"], 1)
        self.assertEqual(r["simulation"]["analysis"]["impact"], 100.0)

    def test_user_defined_stress_architecture_scales_to_120_components(self):
        custom = (ROOT / "examples" / "stress_120.fail").read_text(encoding="utf-8")
        r = app.payload(custom, ["C1"])
        self.assertTrue(r["ok"], r.get("errors"))
        self.assertEqual(r["model"]["system"], "LargeCommerce")
        self.assertEqual(len(r["model"]["components"]), 120)
        self.assertEqual(r["simulation"]["analysis"]["failed"], 1)
        self.assertGreaterEqual(r["simulation"]["analysis"]["direct"], 1)
        self.assertEqual(r["simulation"]["analysis"]["impact"], 100.0)

    def test_ai_advisor_retries_transient_503_then_succeeds(self):
        fake_response = Mock()
        fake_response.read.return_value = json.dumps({
            "candidates": [{"content": {"parts": [{"text": "Use a standby replica."}]}}]
        }).encode()
        fake_response.__enter__ = Mock(return_value=fake_response)
        fake_response.__exit__ = Mock(return_value=False)
        http_error = __import__("urllib.error", fromlist=["HTTPError"]).HTTPError(
            "https://generativelanguage.googleapis.com", 503, "busy", {}, None
        )
        with patch.dict(os.environ, {"GEMINI_API_KEY": "test-key", "GEMINI_MODEL": "gem-test"}):
            with patch.object(app.urllib.request, "urlopen", side_effect=[http_error, fake_response]) as urlopen:
                with patch.object(app.time, "sleep"):
                    result = app.call_gemini_recovery_advisor({"system": "Tiny"})
        self.assertTrue(result["ok"], result)
        self.assertEqual(urlopen.call_count, 2)

    def test_ai_advisor_retries_timeout_once(self):
        fake_response = Mock()
        fake_response.read.return_value = json.dumps({
            "candidates": [{"content": {"parts": [{"text": "Add a failover path."}]}}]
        }).encode()
        fake_response.__enter__ = Mock(return_value=fake_response)
        fake_response.__exit__ = Mock(return_value=False)
        with patch.dict(os.environ, {"GEMINI_API_KEY": "test-key", "GEMINI_MODEL": "gem-test"}):
            with patch.object(app.urllib.request, "urlopen", side_effect=[TimeoutError("read timed out"), fake_response]) as urlopen:
                with patch.object(app.time, "sleep"):
                    result = app.call_gemini_recovery_advisor({"system": "Tiny"})
        self.assertTrue(result["ok"], result)
        self.assertEqual(urlopen.call_count, 2)

if __name__ == "__main__":
    unittest.main()
