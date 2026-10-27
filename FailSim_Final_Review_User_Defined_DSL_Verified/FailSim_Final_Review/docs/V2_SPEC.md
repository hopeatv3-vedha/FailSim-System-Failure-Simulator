# FailSim DSL V2 Specification

## Grammar

```text
program       ::= SYSTEM identifier declaration*

declaration   ::= group
                | component
                | dependency
                | optional_dependency
                | connection
                | recovery
                | scenario

group         ::= GROUP identifier

component     ::= COMPONENT identifier TYPE component_type component_stmt* END

component_stmt ::= STATE state_value
                 | CRITICALITY criticality
                 | GROUP identifier

dependency   ::= DEPENDS identifier ON identifier
optional_dependency ::= OPTIONAL identifier ON identifier
connection   ::= CONNECTS identifier TO identifier

recovery     ::= RECOVERY identifier recovery_stmt* END

recovery_stmt ::= USE identifier
                | REDIRECT identifier TO identifier

scenario     ::= SCENARIO identifier scenario_stmt* END

scenario_stmt ::= FAIL identifier
```

## Component types

CLIENT, CDN, GATEWAY, LOAD_BALANCER, AUTH, SERVICE, DATABASE, CACHE, QUEUE, WORKER, STORAGE, EXTERNAL_API, MONITOR, BACKUP

## States

RUNNING, STOPPED, STANDBY, FAILED

## Criticality

LOW, MEDIUM, HIGH, CRITICAL

## Semantics

- DEPENDS is a mandatory dependency.
- OPTIONAL is a non-critical dependency.
- CONNECTS represents communication and is not itself a failure-propagation edge.
- A failed mandatory dependency makes a dependent component affected.
- A failed optional dependency makes a dependent component DEGRADED.
- Multiple failures are analyzed in the same scenario.
- Recovery rules are deterministic and supplied by the DSL.

## User-defined systems

FailSim does not hard-code the e-commerce component names. A `.fail` file can describe another architecture using the supported grammar above. Component names and group names are user-defined identifiers, but must not collide with reserved DSL keywords (for example, do not name a component `CDN`, `SERVICE`, `STATE`, or `END`).

A system can be compiled and manually tested even if it defines no `SCENARIO`: compile the DSL, select one or more component chips in the UI, and inject failures. Predefined `SCENARIO` blocks are optional shortcuts for repeatable test cases.

The included `examples/library_system.fail` demonstrates a non-e-commerce system with a reader API, catalog database, replica, cache, worker, dependencies, recovery policy, and both single- and multiple-failure scenarios.

"Any system" means any architecture that can be expressed using this DSL's current component types, relationships, state/criticality fields, and recovery rules. It does not mean automatic discovery of arbitrary deployed infrastructure or live production monitoring. Unsupported concepts must first be represented in the DSL or added to the compiler.
