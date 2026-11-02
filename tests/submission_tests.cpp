
#include <gtest/gtest.h>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace {
class SubmissionTests : public ::testing::Test {
protected:
    void SetUp() override {
        connectionName =
            QStringLiteral("submission_tests_%1").arg(++counter);

        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(":memory:");

        ASSERT_TRUE(db.open());
        QSqlQuery pragma(db);

        ASSERT_TRUE(pragma.exec("PRAGMA foreign_keys = ON"));
        
        ASSERT_TRUE(pragma.exec("PRAGMA foreign_keys"));
        ASSERT_TRUE(pragma.next());
        
        EXPECT_EQ(pragma.value(0).toInt(), 1);

        QSqlQuery q(db);

        ASSERT_TRUE(q.exec("PRAGMA foreign_keys=ON"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE users("
            "id INTEGER PRIMARY KEY, role TEXT NOT NULL)"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE assignments("
            "id INTEGER PRIMARY KEY,"
            "due_at TEXT NOT NULL,"
            "class_extension TEXT,"
            "max_mark REAL NOT NULL CHECK(max_mark>0))"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE individual_extensions("
            "assignment_id INTEGER NOT NULL,"
            "student_id INTEGER NOT NULL,"
            "due_at TEXT NOT NULL,"
            "reason TEXT NOT NULL,"
            "PRIMARY KEY(assignment_id,student_id))"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE submissions("
            "id INTEGER PRIMARY KEY,"
            "assignment_id INTEGER NOT NULL,"
            "student_id INTEGER NOT NULL,"
            "content TEXT NOT NULL,"
            "submitted_at TEXT NOT NULL,"
            "status TEXT NOT NULL DEFAULT 'Submitted',"
            "raw_mark REAL,"
            "final_mark REAL,"
            "feedback TEXT NOT NULL DEFAULT '',"
            "marked_by INTEGER,"
            "moderated_by INTEGER,"
            "released_at TEXT,"
            "UNIQUE(assignment_id,student_id))"));

        ASSERT_TRUE(q.exec(
            "INSERT INTO users VALUES"
            "(1,'lecturer'),(2,'student'),(3,'hod')"));

        ASSERT_TRUE(q.exec(
            "INSERT INTO assignments(id,due_at,max_mark)"
            " VALUES(10,'2026-11-01T12:00:00',100)"));
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

int SubmissionTests::counter = 0;
}

TEST_F(SubmissionTests, SavesSubmissionWithTimestampAndContent) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.prepare(
        "INSERT INTO submissions"
        "(assignment_id,student_id,content,submitted_at)"
        " VALUES(?,?,?,?)"));

    q.addBindValue(10);
    q.addBindValue(2);
    q.addBindValue("My assignment");
    q.addBindValue("2026-11-01T10:00:00");

    ASSERT_TRUE(q.exec());

    QSqlQuery read(db);

    ASSERT_TRUE(read.exec(
        "SELECT content,status FROM submissions"
        " WHERE assignment_id=10 AND student_id=2"));

    ASSERT_TRUE(read.next());

    EXPECT_EQ(read.value(0).toString(), "My assignment");
    EXPECT_EQ(read.value(1).toString(), "Submitted");
}

TEST_F(SubmissionTests, PreventsDuplicateSubmissionForSameAssignmentAndStudent) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.exec(
        "INSERT INTO submissions"
        "(assignment_id,student_id,content,submitted_at)"
        " VALUES(10,2,'Version 1','2026-11-01T10:00:00')"));

    EXPECT_FALSE(q.exec(
        "INSERT INTO submissions"
        "(assignment_id,student_id,content,submitted_at)"
        " VALUES(10,2,'Version 2','2026-11-01T11:00:00')"));
}

TEST_F(SubmissionTests, UsesIndividualExtensionBeforeClassOrOriginalDeadline) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.exec(
        "UPDATE assignments SET"
        " class_extension='2026-11-03T12:00:00'"
        " WHERE id=10"));

    ASSERT_TRUE(q.exec(
        "INSERT INTO individual_extensions VALUES"
        "(10,2,'2026-11-04T12:00:00','Approved extension')"));

    QSqlQuery effective(db);

    ASSERT_TRUE(effective.prepare(
        "SELECT COALESCE(i.due_at,a.class_extension,a.due_at)"
        " FROM assignments a"
        " LEFT JOIN individual_extensions i"
        " ON i.assignment_id=a.id AND i.student_id=?"
        " WHERE a.id=?"));

    effective.addBindValue(2);
    effective.addBindValue(10);

    ASSERT_TRUE(effective.exec());
    ASSERT_TRUE(effective.next());

    EXPECT_EQ(
        effective.value(0).toString(),
        "2026-11-04T12:00:00");
}

TEST_F(SubmissionTests, UsesClassExtensionWhenNoIndividualExtensionExists) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.exec(
        "UPDATE assignments SET"
        " class_extension='2026-11-03T12:00:00'"
        " WHERE id=10"));

    QSqlQuery effective(db);

    ASSERT_TRUE(effective.prepare(
        "SELECT COALESCE(i.due_at,a.class_extension,a.due_at)"
        " FROM assignments a"
        " LEFT JOIN individual_extensions i"
        " ON i.assignment_id=a.id AND i.student_id=?"
        " WHERE a.id=?"));

    effective.addBindValue(2);
    effective.addBindValue(10);

    ASSERT_TRUE(effective.exec());
    ASSERT_TRUE(effective.next());

    EXPECT_EQ(
        effective.value(0).toString(),
        "2026-11-03T12:00:00");
}

TEST_F(SubmissionTests, StoresLateStatusAndBothRawAndFinalMarks) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.prepare(
        "INSERT INTO submissions"
        "(assignment_id,student_id,content,submitted_at,"
        "status,raw_mark,final_mark,feedback,marked_by)"
        " VALUES(?,?,?,?,?,?,?,?,?)"));

    q.addBindValue(10);
    q.addBindValue(2);
    q.addBindValue("Late work");
    q.addBindValue("2026-11-02T12:00:00");
    q.addBindValue("Late");
    q.addBindValue(80.0);
    q.addBindValue(70.0);
    q.addBindValue("Good effort");
    q.addBindValue(1);

    ASSERT_TRUE(q.exec());

    QSqlQuery read(db);

    ASSERT_TRUE(read.exec(
        "SELECT raw_mark,final_mark,status"
        " FROM submissions WHERE student_id=2"));

    ASSERT_TRUE(read.next());

    EXPECT_DOUBLE_EQ(read.value(0).toDouble(), 80.0);
    EXPECT_DOUBLE_EQ(read.value(1).toDouble(), 70.0);
    EXPECT_EQ(read.value(2).toString(), "Late");
}

TEST_F(SubmissionTests, RejectsSubmissionForUnknownAssignmentThroughForeignKey) {
    QSqlQuery q(db);

    EXPECT_FALSE(q.exec(
        "INSERT INTO submissions"
        "(assignment_id,student_id,content,submitted_at)"
        " VALUES(999,2,'work','2026-11-01T10:00:00')"));
}
