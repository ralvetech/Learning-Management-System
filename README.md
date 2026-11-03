# Moderated LMS — C++17 Desktop Application
Presentation video link: https://www.youtube.com/watch?v=SWZ1qbrDP9U
A CS4433 assignment-aligned desktop LMS prototype using Qt 6 Widgets, SQLite and OpenSSL scrypt. This project is intended as a development baseline, not a claim of a fully verified or production-ready release.

## Build on Kali/Ubuntu
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev libssl-dev
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/moderated_lms
```
The `sudo` commands require a normal host terminal with administrative privileges. If your terminal reports `no new privileges`, resolve the environment/permission issue before installing packages.

## Demo accounts
All seeded accounts use `12345467890 OR ChangeMe123!`
- `admin` — Administrator
- `lecturer` — Lecturer
- `hod` — Head of Department
- `student` — Student

These are demonstration-only credentials. Do not use them with real learner data.

## Included workflows
- Role-specific login and GUI tabs.
- Admin: create users/departments/courses, assign lecturers, enrol students.
- Lecturer: create assignments, grant individual/class-wide extensions, mark submissions and enter feedback.
- HoD: moderate and release marks.
- Student: view assignments/deadlines, submit/resubmit text work, view released marks/feedback.
- SQLite persistence, salted scrypt password hashes, mark audit table.
- CR1 class-wide extensions appear in the student's assignment list.
- Late penalty calculation updated to deduct 10% of the maximum mark per late day, up to three days, and zero after three days. Confirm this interpretation and partial-day handling with the school.

## Documentation
- `docs/PROJECT_PLAN.md` — feasibility, WBS, seven-week schedule and risk register.
- `docs/REQUIREMENTS.md` — stakeholder conflict resolution, SRS and three use cases.
- `docs/DESIGN.md` — architecture, UML source, database/data dictionary, traceability.
- `docs/TEST_PLAN.md` and `docs/ACCEPTANCE_TESTS.md` — V-Model test plan and acceptance checklist.
- `docs/CONFIGURATION_AND_QA.md` — Git strategy, CI, static analysis and review checklist.
- `docs/DEPLOYMENT_AND_MANUAL.md` — deployment diagram, installation and role manuals.
- `docs/PROFESSIONAL_ISSUES.md` — privacy, security, fairness and accessibility.
- `.github/workflows/ci.yml` — build and CTest workflow.
- `tests/policy_tests.cpp` — starter boundary tests for late penalty.

## Important limitations before submission
The assignment requires a detailed design and implementation that agree. This prototype still needs work before claiming full compliance:
1. Refactor business logic into services/repositories and implement the required abstract `User` class, role subclasses/polymorphism, and Strategy/Repository patterns in actual code.
2. Upgrade the test harness to GoogleTest or Catch2; add real unit, integration, system and acceptance tests; measure and report actual >=70% business-logic line coverage.
3. Enforce role and department authorization in workflow/service functions, including department-scoped audit access.
4. Make mark update and audit insert atomic in a transaction; strengthen audit immutability.
5. Add submission file upload or explicitly agree that text-only submissions satisfy the school’s interpretation.
6. Run a clean build, CI, static analysis, performance test with 800 student records, backup/restore test, accessibility checks and security review. Record actual logs/results.
7. Complete a 6,000–8,000 word final report, including real references in a consistent style and the evidence appendices.
8. Create and publish a Git repository; do not invent a repository URL or claim CI passed without evidence.

The assignment brief warns that artefacts must be consistent and that the student must explain the code in a viva. Review, run, understand and adapt the project yourself before submission.


## Final hardening changes in this package
- Late-penalty logic is encapsulated in `lms::StandardLatePenalty` implementing `LatePenaltyStrategy`; the marking workflow calls this strategy.
- `include/lms/domain.hpp` defines the abstract `User` contract, role subclasses, a `UserFactory`, and explicit capability policy. These domain types need to be fully wired into all application services and used as the authoritative authorization boundary before production deployment.
- Mark updates and audit inserts are now wrapped in a SQLite transaction; an audit-write failure rolls back the mark update.
- The HoD audit view is filtered to the HoD's department.
- A GoogleTest suite is included and the CI workflow installs `libgtest-dev`. If GoogleTest is unavailable locally, CMake falls back to the standalone policy test. Run CI in the hosted repository to obtain actual test evidence.


## Administrator dashboard enhancements
- Filterable data views: all users, students, lecturers, HoDs, administrators, departments, courses, enrolments and assignments.
- Search box filters the currently selected view.
- Edit selected supports user account details/role/department/active status/optional password reset, department names, course code/title/department/lecturer, and assignment title/description/deadline/maximum mark.
- Enrolment correction is supported by removing a selected enrolment and using Enrol student to add the correct relationship.
- Export table to CSV exports the currently selected view and only visible (filtered) rows; open the file in LibreOffice Calc or Excel.
- Explicit black text, light table backgrounds, readable headers and dark text on selected rows replace the problematic white-on-white table styling.

## Build verification note
I attempted a clean CMake configure in the packaging environment. It could not configure because Qt 6 development files are not installed in that environment. Therefore the modified Qt GUI has not been compiled or runtime-tested here. On your Kali machine, run:
```bash
cmake -S . -B build
cmake --build build -j2
```
Resolve any compiler diagnostics before treating this as a final submission. The admin dashboard changes are in `src/main.cpp`.


## Modular source architecture
The GUI has been split into small translation units so each role can be debugged independently:
- `src/main.cpp` — application startup only.
- `src/database.cpp` and `include/lms/common.hpp` — SQLite schema/initialisation, password hashing and shared utilities.
- `src/login_dialog.cpp` and `include/lms/ui/login_dialog.hpp` — login dialog and credential verification.
- `src/main_window.cpp` and `include/lms/ui/main_window.hpp` — main window shell, shared page/table helpers and submission-table utilities.
- `src/ui/admin_dashboard.cpp` — administrator dashboard, editing, filters/search and CSV export.
- `src/ui/lecturer_dashboard.cpp` — lecturer workflows.
- `src/ui/hod_dashboard.cpp` — HoD moderation/release workflows.
- `src/ui/student_dashboard.cpp` — student learning/submission workflows.
- `include/lms/domain.hpp` — domain roles, factory and late-penalty Strategy.
- `tests/` — automated test sources.

Build from the project root:
```bash
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```
