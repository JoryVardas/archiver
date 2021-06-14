package main

import (
	sql "database/sql"
	"errors"
	"fmt"
	"os"
	"path/filepath"
)

type Directory struct {
	directoryID uint64
	path        string
}

var nilDirectory = Directory{0, ""}
var rootDirectory = Directory{1, "/"}

func NewDirectory(id uint64, path string) Directory {
	return Directory{id, path}
}

func (directory *Directory) Path() string {
	return directory.path
}
func (directory *Directory) ID() uint64 {
	return directory.directoryID
}
func (directory *Directory) ToString() string {
	return fmt.Sprintf("{id: %d, path: %s}", directory.ID(), directory.Path())
}

func GetDirectoryFromPath(path string) (Directory, error) {
	var ret Directory

	path = filepath.Clean(path)
	ret.path = path

	if len(path) == 0 || path[len(path)-1:] == "/" {
		return rootDirectory, nil
	}

	parentDirectory, err := GetDirectoryFromPath(filepath.Dir(path))
	if err != nil {
		return nilDirectory, fmt.Errorf("an error occured while trying to get the directory info for \"%s\": %w", path, err)
	}

	err = database.QuerySingleRow("database.directory.getByNameAndParent", []interface{}{filepath.Base(path), parentDirectory.ID()}, []interface{}{&ret.directoryID})
	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			return ret, nil
		}
		return nilDirectory, fmt.Errorf("an error occured while trying to get the directory info for \"%s\": %w", path, err)
	}

	return ret, nil
}

func (directory *Directory) Archive() error {
	var err error
	if directory.ID() == 0 {
		err = directory.addDirectory()
		if err != nil {
			return fmt.Errorf("an error occured while archiving the directory \"%s\": %w", directory.Path(), err)
		}
	}
	return nil
}

func (directory *Directory) Check() error {
	if directory.ID() == 0 {
		return fmt.Errorf("the directory \"%s\" was not added to the database", directory.Path())
	}
	return nil
}

func (directory *Directory) Dearchive(outputPath string) error {
	if debug {
		fmt.Println("INFO: dearchiving \"" + directory.Path() + "\" to \"" + outputPath + "\"")
	} else {
		fmt.Println("Dearchiving \"" + directory.Path() + "\"")
	}

	directoryRows, err := database.Query("database.directory.getChildDirectories", []interface{}{directory.ID()})
	if err != nil {
		return fmt.Errorf("an error occured while trying to find the directories under \"%s\" to dearchive: %w", directory.ToString(), err)
	}
	defer directoryRows.Close()
	var directories []Directory
	for directoryRows.Next() {
		var id uint64
		var name string
		err = directoryRows.Scan(&id, &name)
		if err != nil {
			return fmt.Errorf("an error occured while trying to find the directories under \"%s\" to dearchive: %w", directory.ToString(), err)
		}
		newDirectory := Directory{id, filepath.Join(directory.Path(), name)}

		if debug {
			fmt.Println("DIRECTORY : " + newDirectory.ToString())
		}

		directories = append(directories, newDirectory)
	}

	fileRows, err := database.Query("database.directory.getChildFiles", []interface{}{directory.ID()})
	if err != nil {
		return fmt.Errorf("an error occured while trying to find the files under \"%s\" to dearchive: %w", directory.ToString(), err)
	}
	defer fileRows.Close()
	var files []File
	for fileRows.Next() {
		var id uint64
		var name string
		err = fileRows.Scan(&id, &name)
		if err != nil {
			return fmt.Errorf("an error occured while trying to find the directories under \"%s\" to dearchive: %w", directory.ToString(), err)
		}

		newFile := NewFile(id, directory.ID(), 0, filepath.Join(directory.Path(), name), "")

		if debug {
			fmt.Println("FILE : " + newFile.ToString())
		}

		files = append(files, newFile)
	}

	outputDir := filepath.Clean(filepath.Join(outputPath, directory.Path()))
	err = os.MkdirAll(outputDir, os.ModePerm)
	if err != nil {
		return fmt.Errorf("an error occured while trying to dearchive \"%s\": %w", directory.Path(), err)
	}

	for _, cur := range files {
		err = cur.Dearchive(outputDir)
		if err != nil {
			return fmt.Errorf("an error occured while trying to dearchive \"%s\": %w", cur.Path(), err)
		}
	}
	for _, cur := range directories {
		err = cur.Dearchive(outputPath)
		if err != nil {
			return fmt.Errorf("an error occured while trying to dearchive \"%s\": %w", cur.Path(), err)
		}
	}

	return nil
}

func (directory *Directory) addDirectory() error {
	if len(directory.Path()) == 0 || directory.Path() == "/" {
		directory.directoryID = 1
		directory.path = "/"
		return nil
	}

	parentDirectory, err := GetDirectoryFromPath(filepath.Dir(directory.Path()))
	if err != nil {
		return fmt.Errorf("an error occured while trying to add the directory \"%s\": %w", directory.Path(), err)
	}

	if parentDirectory.ID() == 0 {
		err = parentDirectory.addDirectory()
		if err != nil {
			return fmt.Errorf("an error occured while trying to add the directory \"%s\": %w", directory.Path(), err)
		}
	}

	directory.directoryID, err = database.Insert("database.directory.insert", []interface{}{filepath.Base(directory.Path()), parentDirectory.ID()})
	if err != nil {
		return fmt.Errorf("an error occured while trying to add the directory \"%s\": %w", directory.Path(), err)
	}

	return nil
}
