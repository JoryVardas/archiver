package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"os"
)

type Configuration struct {
	General struct {
		FileReadSize uint64 `json:"file_read_size"`
	} //`json:"general"
	Archive struct {
		ArchiveDirectory     string `json:"archive_directory"`
		TargetSize           uint64 `json:"target_size"`
		SingleArchiveSize    uint64 `json:"single_archive_size"`
		TempArchiveDirectory string `json:"temp_archive_directory"`
		//CompressedArchiveDirectory string `json:"compressed_archive_directory"`
	} //`json:"archive"`
	Database struct {
		User     string   `json:"user"`
		Password string   `json:"password"`
		Location string   `json:"location"`
		Options  []string `json:"options"`
	} //`json:"database"`
	AWS struct {
		AccessKey string `json:"access_key"`
		SecretKey string `json:"secret_key"`
	} //`json:"aws"`
}

func (config *Configuration) load(configFilePath string) bool {
	configFile, err := os.Open(configFilePath)
	if err != nil {
		log.Println(err)
		fmt.Println("The specified configuration file could not be opened. Please make sure it exists as it is required.")
		return false
	}

	configFileBytes, err := ioutil.ReadAll(configFile)
	if err != nil {
		log.Println(err)
		fmt.Println("The specified configuration file could not be read.")
		return false
	}

	if json.Unmarshal([]byte(configFileBytes), &config) != nil {
		log.Println(err)
		fmt.Println("There was an error parsing the specified configuration file, please check the syntax.")
		return false
	}

	configFile.Close()
	return true
}

func (config *Configuration) Print() {
	fmt.Printf("\nLoaded Configuration {"+
		"\n\tGeneral {"+
		"\n\t\tFileReadSize: %d "+
		"\n\t}"+
		"\n\tArchive {"+
		"\n\t\tArchiveDirectory: %s "+
		"\n\t\tTargetSize: %d "+
		"\n\t\tSingleArchiveSize: %d "+
		"\n\t\tTempArchiveDirectory: %s "+
		"\n\t}"+
		"\n\tDatabase {"+
		"\n\t\t[Hidded For Security]"+
		"\n\t}"+
		"\n\tAWS {"+
		"\n\t\t[Hidded For Security]"+
		"\n\t}"+
		"\n}",
		config.General.FileReadSize,
		config.Archive.ArchiveDirectory,
		config.Archive.TargetSize,
		config.Archive.SingleArchiveSize,
		config.Archive.TempArchiveDirectory)
}
