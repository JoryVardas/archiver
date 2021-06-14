package main

import (
	"database/sql"
	"fmt"
	"strings"
)

type Database struct {
	databaseConnection  *sql.DB
	databaseTransaction *sql.Tx

	preparedStatements map[string]*sql.Stmt
}

type databaseStatementInitializationStruct struct {
	stmtLocation string
	stmt         string
}

func (database *Database) Init() error {
	var err error
	database.databaseConnection, err = sql.Open("mysql", config.Database.User+":"+config.Database.Password+"@"+config.Database.Location+"?"+strings.Join(config.Database.Options, "&"))
	if err != nil {
		return fmt.Errorf("could not initialize database connection: %w", err)
	}

	err = database.newTransaction()
	if err != nil {
		return fmt.Errorf("could not initialize database connection: %w", err)
	}

	database.preparedStatements = make(map[string]*sql.Stmt)
	return database.initStatements([]databaseStatementInitializationStruct{
		{"database.general.getCurrentTime", "SELECT CURRENT_TIMESTAMP(2) FROM dual"},

		{"database.archive.getCurrentIDForContents", "SELECT IFNULL(MAX(id), 0) AS id FROM archive WHERE contents=?"},
		{"database.archive.getSizeByID", "SELECT SUM(size) AS size FROM file_revision WHERE file_revision.archive = ?"},
		{"database.archive.insert", "INSERT INTO archive (contents) VALUES (?)"},
		{"database.archive.getContentsByID", "SELECT contents FROM archive WHERE id=?"},
		{"database.archive.getMaxID", "SELECT MAX(id) FROM archive"},
		{"database.archive.getHashsByID", "SELECT blake2b, sha3 FROM archive WHERE id=?"},
		{"database.archive.updateHashsByID", "UPDATE archive SET blake2b=?, sha3=? WHERE id=?"},
		{"database.archive.replaceSingleHash", "REPLACE INTO single_archive_hash(file_id, archive_time, blake2b, sha3) VALUES (?, ?, ?, ?)"},
		{"database.archive.getSingleHashs", "SELECT file_id, archive_time, blake2b, sha3 FROM single_archive_hash"},

		{"database.directory.insert", "INSERT INTO directory (name, parent_id) VALUES (?, ?)"},
		{"database.directory.getByNameAndParent", "SELECT id FROM directory WHERE name = ? AND parent_id = ?"},
		{"database.directory.getChildDirectories", "SELECT id, name FROM directory WHERE parent_id = ? AND name <> \"/\""},
		{"database.directory.getChildFiles", "SELECT id, name FROM file WHERE parent_id = ?"},

		{"database.file.getRevisionsNewerThanTime", "SELECT file_id, archive, archive_time FROM file_revision WHERE archive_time > ?"},
		{"database.file.insert", "INSERT INTO file (name, parent_id) VALUES (?, ?)"},
		{"database.file.getByNameAndParent", "SELECT id FROM file WHERE name = ? AND parent_id = ?"},
		{"database.file.getRevisionBySizeAndHashs", "SELECT file_id, archive_time FROM file_revision WHERE size = ? AND blake2b = ? AND sha3 = ?"},
		{"database.file.insertNewRevision", "INSERT INTO file_revision (file_id, size, archive, blake2b, sha3, archive_time) VALUES (?, ?, ?, ?, ?, CURRENT_TIMESTAMP(2))"},
		{"database.file.insertDuplicateRevision", "INSERT INTO file_duplicate (file_id, duplicate_file_id, duplicate_file_archive_time, archive_time) VALUES (?, ?, ?, CURRENT_TIMESTAMP(2))"},
		{"database.file.getMostRecentRevisionInfoByFileID", "SELECT file_id, revision_archive_time, archive FROM (SELECT * FROM ((SELECT revision.file_id, duplicate.archive_time, revision.archive_time as revision_archive_time, revision.archive FROM (SELECT * FROM file_duplicate WHERE file_id = ? ORDER BY archive_time DESC LIMIT 1) AS duplicate LEFT JOIN (SELECT file_id, archive_time, archive FROM file_revision) AS revision ON duplicate.duplicate_file_id = revision.file_id AND duplicate.duplicate_file_archive_time = revision.archive_time) UNION (SELECT file_id, archive_time, archive_time AS revision_archive_time, archive FROM file_revision WHERE file_id = ? ORDER BY archive_time DESC LIMIT 1)) AS recent_revisions ORDER BY archive_time DESC LIMIT 1) AS revision_info"},
		{"database.file.getFileRevisionTimestampByFileID", "SELECT archive_time AS revision_archive_time FROM file_revision WHERE file_id = ? ORDER BY revision_archive_time DESC LIMIT 1"},
		{"database.file.getAllSingleArchiveFileRevisons", "SELECT file_id, archive_time FROM file_revision WHERE archive = 1"},
	})
}

func (database *Database) Commit() error {
	err := database.databaseTransaction.Commit()
	if err != nil {
		return fmt.Errorf("could not commit the database transaction: %w", err)
	}

	return database.newTransaction()
}

func (database *Database) Rollback() error {
	err := database.databaseTransaction.Rollback()
	if err != nil {
		return fmt.Errorf("could not rollback the database transaction: %w", err)
	}

	return database.newTransaction()
}

func (database *Database) Close() {
	database.databaseTransaction.Rollback()
	database.databaseConnection.Close()
}

func (database *Database) Query(stmtLocation string, parameters []interface{}) (*sql.Rows, error) {
	if _, contains := database.preparedStatements[stmtLocation]; !contains {
		return nil, fmt.Errorf("the prepared statement %s doesn't exist", stmtLocation)
	}
	return database.getTransactionableStmt(stmtLocation).Query(parameters...)
}

func (database *Database) QuerySingleRow(stmtLocation string, parameters []interface{}, returnValues []interface{}) error {
	if _, contains := database.preparedStatements[stmtLocation]; !contains {
		return fmt.Errorf("the prepared statement %s doesn't exist", stmtLocation)
	}
	return database.getTransactionableStmt(stmtLocation).QueryRow(parameters...).Scan(returnValues...)
}

func (database *Database) Insert(stmtLocation string, parameters []interface{}) (uint64, error) {
	if _, contains := database.preparedStatements[stmtLocation]; !contains {
		return 0, fmt.Errorf("the prepared statement %s doesn't exist", stmtLocation)
	}

	insertResult, err := database.getTransactionableStmt(stmtLocation).Exec(parameters...)
	if err != nil {
		return 0, fmt.Errorf("there was an error executing the prepared statement %s: %w", stmtLocation, err)
	}
	insertID, err := insertResult.LastInsertId()
	if err != nil {
		return 0, fmt.Errorf("there was an error getting the insertion id after executing the prepared statement %s: %w", stmtLocation, err)
	}
	return uint64(insertID), nil
}
func (database *Database) Update(stmtLocation string, parameters []interface{}) (uint64, error) {
	if _, contains := database.preparedStatements[stmtLocation]; !contains {
		return 0, fmt.Errorf("the prepared statement %s doesn't exist", stmtLocation)
	}

	updateResult, err := database.getTransactionableStmt(stmtLocation).Exec(parameters...)
	if err != nil {
		return 0, fmt.Errorf("there was an error executing the prepared statement %s: %w", stmtLocation, err)
	}
	rowsAffected, err := updateResult.RowsAffected()
	if err != nil {
		return 0, fmt.Errorf("there was an error getting the number of rows affected after executing the prepared statement %s: %w", stmtLocation, err)
	}
	return uint64(rowsAffected), nil
}

func (database *Database) newTransaction() error {
	var err error
	database.databaseTransaction, err = database.databaseConnection.Begin()
	if err != nil {
		return fmt.Errorf("could not open a new database transaction: %w", err)
	}

	return nil
}

func (database *Database) initStatements(statementList []databaseStatementInitializationStruct) error {
	var err error
	for _, cur := range statementList {
		if _, contains := database.preparedStatements[cur.stmtLocation]; contains {
			return fmt.Errorf("the prepared statement %s has already been created: %w", cur.stmtLocation, err)
		}
		database.preparedStatements[cur.stmtLocation], err = database.databaseConnection.Prepare(cur.stmt)
		if err != nil {
			return fmt.Errorf("could not create the prepared statement for %s: %w", cur.stmtLocation, err)
		}
	}
	return nil
}

func (database *Database) getTransactionableStmt(stmtLocation string) *sql.Stmt {
	return database.databaseTransaction.Stmt(database.preparedStatements[stmtLocation])
}
