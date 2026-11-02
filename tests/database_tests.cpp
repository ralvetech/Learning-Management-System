
#include <gtest/gtest.h>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace {
class DatabaseTests : public ::testing::Test {
protected:
    void SetUp() override {
        connectionName =
            QStringLiteral("database_tests_%1").arg(++counter);

        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(":memory:");

        ASSERT_TRUE(db.open()) << db.lastError().text().toStdString();

        QSqlQuery q(db);

        ASSERT_TRUE(q.exec("PRAGMA foreign_keys = ON"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE departments("
            "id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE)"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE users("
            "id INTEGER PRIMARY KEY,"
            "username TEXT NOT NULL UNIQUE,"
            "role TEXT NOT NULL CHECK(role IN"
            "('admin','lecturer','hod','student')),"
            "department_id INTEGER REFERENCES departments(id))"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE courses("
            "id INTEGER PRIMARY KEY,"
            "code TEXT NOT NULL UNIQUE,"
            "department_id INTEGER NOT NULL REFERENCES departments(id),"
            "lecturer_id INTEGER REFERENCES users(id))"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE enrolments("
            "student_id INTEGER NOT NULL REFERENCES users(id),"
            "course_id INTEGER NOT NULL REFERENCES courses(id),"
            "PRIMARY KEY(student_id,course_id))"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE assignments("
            "id INTEGER PRIMARY KEY,"
            "course_id INTEGER NOT NULL REFERENCES courses(id),"
            "title TEXT NOT NULL,"
            "due_at TEXT NOT NULL,"
            "max_mark REAL NOT NULL CHECK(max_mark > 0),"
            "created_by INTEGER NOT NULL REFERENCES users(id),"
            "class_extension TEXT)"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE submissions("
            "id INTEGER PRIMARY KEY,"
            "assignment_id INTEGER NOT NULL REFERENCES assignments(id),"
            "student_id INTEGER NOT NULL REFERENCES users(id),"
            "content TEXT NOT NULL,"
            "submitted_at TEXT NOT NULL,"
            "status TEXT NOT NULL DEFAULT 'Submitted',"
            "raw_mark REAL,"
            "final_mark REAL,"
            "feedback TEXT NOT NULL DEFAULT '',"
            "marked_by INTEGER REFERENCES users(id),"
            "moderated_by INTEGER REFERENCES users(id),"
            "released_at TEXT,"
            "UNIQUE(assignment_id,student_id))"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE mark_audit("
            "id INTEGER PRIMARY KEY,"
            "submission_id INTEGER NOT NULL REFERENCES submissions(id),"
            "actor_id INTEGER NOT NULL REFERENCES users(id),"
            "changed_at TEXT NOT NULL,"
            "old_mark REAL,"
            "new_mark REAL NOT NULL,"
            "reason TEXT NOT NULL)"));

        ASSERT_TRUE(q.exec(
            "INSERT INTO departments VALUES(1,'Computing')"));

        ASSERT_TRUE(q.exec(
            "INSERT INTO users VALUES"
            "(1,'admin','admin',1),"
            "(2,'lecturer','lecturer',1),"
            "(3,'hod','hod',1),"
            "(4,'student','student',1)"));

        ASSERT_TRUE(q.exec(
            "INSERT INTO courses VALUES(1,'CS101',1,2)"));
    }

    void TearDown() override {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
    }

    QSqlDatabase db;
    QString connectionName;

    static int counter;
};

int DatabaseTests::counter = 0;
}

TEST_F(DatabaseTests, EnforcesUniqueUsernames) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.prepare(
        "INSERT INTO users(username,role,department_id)"
        " VALUES(?,?,?)"));

    q.addBindValue("student2");
    q.addBindValue("student");
    q.addBindValue(1);

    ASSERT_TRUE(q.exec());

    QSqlQuery duplicate(db);

    ASSERT_TRUE(duplicate.prepare(
        "INSERT INTO users(username,role,department_id)"
        " VALUES(?,?,?)"));

    duplicate.addBindValue("student2");
    duplicate.addBindValue("student");
    duplicate.addBindValue(1);

    EXPECT_FALSE(duplicate.exec());
}

TEST_F(DatabaseTests, RejectsUnknownRole) {
    QSqlQuery q(db);

    EXPECT_FALSE(q.exec(
        "INSERT INTO users(username,role)"
        " VALUES('bad_role','teacher')"));
}

TEST_F(DatabaseTests, EnforcesForeignKeysForCourseDepartment) {
    QSqlQuery q(db);

    EXPECT_FALSE(q.exec(
        "INSERT INTO courses(code,department_id)"
        " VALUES('BAD101',999)"));
}

TEST_F(DatabaseTests, PreventsDuplicateStudentCourseEnrolment) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.exec(
        "INSERT INTO enrolments(student_id,course_id)"
        " VALUES(4,1)"));

    EXPECT_FALSE(q.exec(
        "INSERT INTO enrolments(student_id,course_id)"
        " VALUES(4,1)"));
}

TEST_F(DatabaseTests, RejectsNonPositiveAssignmentMaximumMark) {
    QSqlQuery q(db);

    EXPECT_FALSE(q.exec(
        "INSERT INTO assignments"
        "(course_id,title,due_at,max_mark,created_by)"
        " VALUES(1,'Invalid','2026-11-01T10:00:00',0,2)"));
}

TEST_F(DatabaseTests, UsesBoundParametersForStoredText) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.prepare(
        "INSERT INTO users(username,role,department_id)"
        " VALUES(?,?,?)"));

    q.addBindValue("learner'); DROP TABLE users;--");
    q.addBindValue("student");
    q.addBindValue(1);

    ASSERT_TRUE(q.exec());

    QSqlQuery count(db);

    ASSERT_TRUE(count.exec("SELECT COUNT(*) FROM users"));
    ASSERT_TRUE(count.next());

    EXPECT_EQ(count.value(0).toInt(), 5);
}
