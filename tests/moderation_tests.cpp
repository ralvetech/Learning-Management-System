
#include <gtest/gtest.h>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace {
class ModerationTests : public ::testing::Test {
protected:
    void SetUp() override {
        connectionName =
            QStringLiteral("moderation_tests_%1").arg(++counter);

        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(":memory:");

        ASSERT_TRUE(db.open());

        QSqlQuery q(db);

        ASSERT_TRUE(q.exec("PRAGMA foreign_keys=ON"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE users("
            "id INTEGER PRIMARY KEY,"
            "role TEXT NOT NULL,"
            "department_id INTEGER NOT NULL)"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE courses("
            "id INTEGER PRIMARY KEY,"
            "department_id INTEGER NOT NULL)"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE assignments("
            "id INTEGER PRIMARY KEY,"
            "course_id INTEGER NOT NULL)"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE submissions("
            "id INTEGER PRIMARY KEY,"
            "assignment_id INTEGER NOT NULL,"
            "student_id INTEGER NOT NULL,"
            "status TEXT NOT NULL,"
            "raw_mark REAL,"
            "final_mark REAL,"
            "feedback TEXT NOT NULL DEFAULT '',"
            "marked_by INTEGER,"
            "moderated_by INTEGER,"
            "released_at TEXT)"));

        ASSERT_TRUE(q.exec(
            "CREATE TABLE mark_audit("
            "id INTEGER PRIMARY KEY,"
            "submission_id INTEGER NOT NULL,"
            "actor_id INTEGER NOT NULL,"
            "changed_at TEXT NOT NULL,"
            "old_mark REAL,"
            "new_mark REAL NOT NULL,"
            "reason TEXT NOT NULL)"));

        ASSERT_TRUE(q.exec(
            "INSERT INTO users VALUES"
            "(1,'lecturer',7),"
            "(2,'hod',7),"
            "(3,'student',7),"
            "(4,'hod',8)"));

        ASSERT_TRUE(q.exec(
            "INSERT INTO courses VALUES(11,7),(12,8)"));

        ASSERT_TRUE(q.exec(
            "INSERT INTO assignments VALUES(21,11),(22,12)"));

        ASSERT_TRUE(q.exec(
            "INSERT INTO submissions"
            "(id,assignment_id,student_id,status,raw_mark,final_mark,feedback,marked_by)"
            " VALUES"
            "(31,21,3,'Marked',80,70,'Feedback',1),"
            "(32,22,3,'Marked',90,90,'Other department',1)"));
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

int ModerationTests::counter = 0;
}

TEST_F(ModerationTests, HoDQueueContainsOnlyMarksFromOwnDepartment) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.prepare(
        "SELECT s.id FROM submissions s"
        " JOIN assignments a ON a.id=s.assignment_id"
        " JOIN courses c ON c.id=a.course_id"
        " WHERE s.status='Marked'"
        " AND c.department_id=? ORDER BY s.id"));

    q.addBindValue(7);

    ASSERT_TRUE(q.exec());

    ASSERT_TRUE(q.next());

    EXPECT_EQ(q.value(0).toInt(), 31);

    EXPECT_FALSE(q.next());
}

TEST_F(ModerationTests, HoDCanApproveMarkAndAuditTheDecision) {
    ASSERT_TRUE(db.transaction());

    QSqlQuery update(db);

    ASSERT_TRUE(update.prepare(
        "UPDATE submissions SET"
        " status='Moderated', moderated_by=?"
        " WHERE id=? AND status='Marked'"));

    update.addBindValue(2);
    update.addBindValue(31);

    ASSERT_TRUE(update.exec());
    ASSERT_EQ(update.numRowsAffected(), 1);

    QSqlQuery audit(db);

    ASSERT_TRUE(audit.prepare(
        "INSERT INTO mark_audit"
        "(submission_id,actor_id,changed_at,old_mark,new_mark,reason)"
        " VALUES(?,?,?,?,?,?)"));

    audit.addBindValue(31);
    audit.addBindValue(2);
    audit.addBindValue("2026-11-02T10:00:00");
    audit.addBindValue(70.0);
    audit.addBindValue(70.0);
    audit.addBindValue("Approved after review");

    ASSERT_TRUE(audit.exec());
    ASSERT_TRUE(db.commit());

    QSqlQuery verify(db);

    ASSERT_TRUE(verify.exec(
        "SELECT status,moderated_by"
        " FROM submissions WHERE id=31"));

    ASSERT_TRUE(verify.next());

    EXPECT_EQ(verify.value(0).toString(), "Moderated");
    EXPECT_EQ(verify.value(1).toInt(), 2);
}

TEST_F(ModerationTests, CannotReleaseMarkBeforeModeration) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.exec(
        "UPDATE submissions SET"
        " status='Released',"
        " released_at='2026-11-02T10:00:00'"
        " WHERE id=31 AND status='Moderated'"));

    EXPECT_EQ(q.numRowsAffected(), 0);
}

TEST_F(ModerationTests, ReleasesOnlyModeratedMark) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.exec(
        "UPDATE submissions SET"
        " status='Moderated',moderated_by=2"
        " WHERE id=31"));

    ASSERT_TRUE(q.exec(
        "UPDATE submissions SET"
        " status='Released',"
        " released_at='2026-11-02T11:00:00'"
        " WHERE id=31 AND status='Moderated'"));

    EXPECT_EQ(q.numRowsAffected(), 1);

    QSqlQuery read(db);

    ASSERT_TRUE(read.exec(
        "SELECT status,released_at"
        " FROM submissions WHERE id=31"));

    ASSERT_TRUE(read.next());

    EXPECT_EQ(read.value(0).toString(), "Released");
    EXPECT_FALSE(read.value(1).toString().isEmpty());
}

TEST_F(ModerationTests, StudentQueryReturnsOnlyReleasedMarks) {
    QSqlQuery q(db);

    ASSERT_TRUE(q.exec(
        "UPDATE submissions SET"
        " status='Moderated',moderated_by=2"
        " WHERE id=31"));

    QSqlQuery visible(db);

    ASSERT_TRUE(visible.prepare(
        "SELECT id,final_mark FROM submissions"
        " WHERE student_id=? AND status='Released'"));

    visible.addBindValue(3);

    ASSERT_TRUE(visible.exec());

    EXPECT_FALSE(visible.next());

    ASSERT_TRUE(q.exec(
        "UPDATE submissions SET"
        " status='Released',"
        " released_at='2026-11-02T12:00:00'"
        " WHERE id=31 AND status='Moderated'"));

    QSqlQuery visibleAfter(db);

    ASSERT_TRUE(visibleAfter.prepare(
        "SELECT id,final_mark FROM submissions"
        " WHERE student_id=? AND status='Released'"));

    visibleAfter.addBindValue(3);

    ASSERT_TRUE(visibleAfter.exec());
    ASSERT_TRUE(visibleAfter.next());

    EXPECT_EQ(visibleAfter.value(0).toInt(), 31);
    EXPECT_DOUBLE_EQ(visibleAfter.value(1).toDouble(), 70.0);
}

TEST_F(ModerationTests, MarkChangesCanBeAuditedWithOldNewValueAndReason) {
    ASSERT_TRUE(db.transaction());

    QSqlQuery update(db);

    ASSERT_TRUE(update.prepare(
        "UPDATE submissions SET raw_mark=?,final_mark=? WHERE id=?"));

    update.addBindValue(85.0);
    update.addBindValue(75.0);
    update.addBindValue(31);

    ASSERT_TRUE(update.exec());

    QSqlQuery audit(db);

    ASSERT_TRUE(audit.prepare(
        "INSERT INTO mark_audit"
        "(submission_id,actor_id,changed_at,old_mark,new_mark,reason)"
        " VALUES(?,?,?,?,?,?)"));

    audit.addBindValue(31);
    audit.addBindValue(1);
    audit.addBindValue("2026-11-02T13:00:00");
    audit.addBindValue(80.0);
    audit.addBindValue(85.0);
    audit.addBindValue("Correction after recheck");

    ASSERT_TRUE(audit.exec());
    ASSERT_TRUE(db.commit());

    QSqlQuery read(db);

    ASSERT_TRUE(read.exec(
        "SELECT old_mark,new_mark,reason"
        " FROM mark_audit WHERE submission_id=31"));

    ASSERT_TRUE(read.next());

    EXPECT_DOUBLE_EQ(read.value(0).toDouble(), 80.0);
    EXPECT_DOUBLE_EQ(read.value(1).toDouble(), 85.0);
    EXPECT_EQ(read.value(2).toString(), "Correction after recheck");
}
