package main

import (
	"bufio"
	"flag"
	"fmt"
	log "log"
	"os"
	"path/filepath"
	"strings"
	"time"

	_ "github.com/go-sql-driver/mysql"
)

var config Configuration
var database Database
var inputReader *bufio.Reader
var debug bool
var info bool

func main() {
	inputReader = bufio.NewReader(os.Stdin)

	dearchiveFlag := flag.Bool("dearchive", false, "De-archive the specified files.")
	dearchiveLocation := flag.String("destination", "", "Location to de-archive the specified files to.")
	archiveFlag := flag.Bool("archive", false, "Archive the specified files.")
	configFilePath := flag.String("config", "settings.json", "Specify the configuration file.")
	rootPrefix := flag.String("prefix", "", "Specify a prefix which is to be removed from path of the files being archived.")
	debugOutput := flag.Bool("debug", false, "Turn on debug output.")
	infoFlag := flag.Bool("info", false, "Do not perform any lasting action")
	ignoreArchiveChecks := flag.Bool("ignore", false, "Ignore additional archive checks, NO NOT USE CAUSUALLY.")
	archiveHashCheck := flag.Bool("check-archives", false, "Check that the archives have not changed since the last archival operation.")
	archiveHashGen := flag.Bool("gen-archives", false, "Generate the hashes for all the archives.")

	flag.Parse()

	debug = *debugOutput
	info = *infoFlag

	if info {
		fmt.Println("Info flag was set, no lasting action will be taken.")
	}

	args := flag.Args()

	fmt.Println("Loading settings.")

	if !config.load(*configFilePath) {
		return
	}

	if debug {
		config.Print()
		fmt.Println()
	}

	fmt.Println("Connecting to database.")

	err := database.Init()
	if err != nil {
		fmt.Println(err.Error())
		return
	}
	defer database.Close()

	var startTime time.Time
	err = database.QuerySingleRow("database.general.getCurrentTime", []interface{}{}, []interface{}{&startTime})
	if err != nil {
		log.Println(err)
		fmt.Println("There was an error while trying to get the start time.")
		return
	}


	if *archiveHashGen {
		fmt.Println("Generating archive hashs.")

		var maxArchiveID uint64
		err := database.QuerySingleRow("database.archive.getMaxID", []interface{}{}, []interface{}{&maxArchiveID})
		if err != nil {
			fmt.Printf("\nThere was an error while trying to get the number of archives: %s", err.Error())
			return
		}
		var i uint64;
		for i = 1; i <= maxArchiveID; i++ {
			//archive, err := GetArchiveById(i)
			//if err != nil {
			//	fmt.Printf("\nThere was an error while trying to get the the archive with id %d: %s", i, err.Error())
			//	return
			//}
			fmt.Printf("Generating archive hashs for %d", i)

		
			archive := Archive{i}
			err = archive.UpdateArchiveHashes()
			if err != nil {
				fmt.Printf("\nThere was an error while trying to generate the hashes for archive with id %d: %s", i, err.Error())
				return
			}
		}
		
		database.Commit()
		return
	}


	if *archiveHashCheck {
		fmt.Println("Checking archive hashs.")

		var maxArchiveID uint64
		err := database.QuerySingleRow("database.archive.getMaxID", []interface{}{}, []interface{}{&maxArchiveID})
		if err != nil {
			fmt.Printf("\nThere was an error while trying to get the number of archives: %s", err.Error())
			return
		}
		var i uint64;
		for i = 1; i <= maxArchiveID; i++ {
			//archive, err := GetArchiveById(i)
			//if err != nil {
			//	fmt.Printf("\nThere was an error while trying to get the the archive with id %d: %s", i, err.Error())
			//	return
			//}
			fmt.Printf("Checking archive hashs for %d", i)

			archive := Archive{i}
			err = archive.CheckArchiveHashes()
			if err != nil {
				fmt.Printf("\nThere was an error while trying to check the hashes for archive with id %d: %s", i, err.Error())
				return
			}
		}

		fmt.Printf("\nAll archives passed the check.\n")
		return
	}

	if len(args) == 0 {
		fmt.Println("No files/directories listed, no work was done.")

		return
	}

	if *archiveFlag {
		pathPrefix := filepath.Clean(*rootPrefix)
		if debug {
			fmt.Printf("\nPath prefix: %s\n", pathPrefix)
		}
		for _, element := range args {
			fmt.Printf("\n\nArchiving %s\n\n", element)
			err = walkAndArchive(element, pathPrefix)
			if err != nil {
				fmt.Printf("There was an error while archiving \"%s\": %s\n", element, err.Error())
				err = undoChanges()
				if err != nil {
					fmt.Printf("There was an error while trying to undo changes, aborting: %s", err.Error())
					return
				}
				fmt.Println("Changes have been undone.")
				return
			}
			if info {
				continue
			}
		}

		if info {
			database.Rollback()
			return
		}

		fmt.Printf("\n\nCompressing Modified Archives\n\n")

		err = compressModifiedArchives()
		if err != nil {
			fmt.Printf("There was an error while compressing the modified archives, aborting: %s", err)
		}

		err = UpdateAllModifedArchiveHashes()
		if err != nil {
			fmt.Printf("There was an error while updating the hashes for the modified archives: %s", err.Error())
			return
		}

		for _, element := range args {
			fmt.Printf("\n\nChecking %s\n\n", element)
			err = walkAndCheck(element, pathPrefix)
			if err != nil {
				fmt.Printf("There was an error while checking \"%s\": %s\n", element, err.Error())
				err = undoChanges()
				if err != nil {
					fmt.Printf("There was an error while trying to undo changes, aborting: %s", err.Error())
					return
				}
				fmt.Println("Changes have been undone.")
				return
			}
		}

		database.Commit()

		fmt.Printf("\n\nChecking Modified Archive Parts\n\n")

		err = checkModifiedArchives(*ignoreArchiveChecks)
		if err != nil {
			fmt.Printf("There was an error while checking the modified archives, aborting: %s", err)
		}

		fmt.Printf("\n\nCleaning Up\n\n")

		removeTempFiles()
	} else if *dearchiveFlag {
		if info {
			fmt.Println("Deachive is not supported with info flag.")
			return
		}
		outputDir := filepath.Clean(*dearchiveLocation)
		for _, element := range args {
			err = walkAndDeArchive(element, outputDir)
			if err != nil {
				fmt.Printf("There was an error while dearchiving \"%s\", aborting: %s\n", element, err.Error())
				return
			}
		}
		removeTempFiles()
	}
}

func sanitizeInputPath(path string) string {
	ret := filepath.Clean(path)
	if !strings.HasPrefix(ret, "/") {
		return "/" + ret
	}
	return ret
}

type FileAction func(File) error
type DirectoryAction func(Directory) error

func walkAndPerformActions(rootPath string, rootPrefix string, fileAction FileAction, directoryAction DirectoryAction) error {
	walkError := filepath.Walk(rootPath, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		relativePath := strings.TrimPrefix(path, rootPrefix)

		if info.IsDir() {
			directory, err := GetDirectoryFromPath(relativePath)
			if err != nil {
				return err
			}
			return directoryAction(directory)
		}

		if !info.IsDir() {
			file, err := GetFileFromPath(relativePath, path)
			if err != nil {
				return err
			}
			return fileAction(file)
		}
		return nil
	})

	return walkError
}

func walkAndArchive(rootPath string, rootPrefix string) error {
	walkError := walkAndPerformActions(rootPath, rootPrefix, func(file File) error {
		return file.Archive()
	}, func(directory Directory) error {
		return directory.Archive()
	})

	if walkError != nil {
		return fmt.Errorf("an error occured while trying to walk and archive \"%s\": %w", rootPath, walkError)
	}
	return nil
}

func walkAndCheck(rootPath string, rootPrefix string) error {
	walkError := walkAndPerformActions(rootPath, rootPrefix, func(file File) error {
		return file.Check()
	}, func(directory Directory) error {
		return directory.Check()
	})

	if walkError != nil {
		return fmt.Errorf("an error occured while trying to walk and check \"%s\": %w", rootPath, walkError)
	}
	return nil
}

func undoChanges() error {
	if info {
		return nil
	}
	err := database.Rollback()
	if err != nil {
		return fmt.Errorf("an error occured while trying to undo all changes: %w", err)
	}

	fmt.Println("Would you like to remove the temporary files? (y: yes, n: no)")
	input, err := inputReader.ReadString('\n')
	if err != nil {
		fmt.Println("There was an error reading your input, assuming no.")
		return nil
	}
	switch input {
	case "n", "N":
		fmt.Println("Leaving temporary files.")
		return nil
	case "y", "Y":
		fmt.Println("Removing temporary files")
		break
	default:
		fmt.Println("Unrecognised input, assuming no.")
		return nil
	}

	err = removeTempFiles()
	if err != nil {
		return fmt.Errorf("an error occured while trying to undo all changes: %w", err)
	}

	return nil
}

func removeTempFiles() error {
	err := os.RemoveAll(config.Archive.TempArchiveDirectory)
	if err != nil {
		return fmt.Errorf("an error occured while trying to remove temporary files: %w", err)
	}
	return nil
}

func doesPathExist(path string) (bool, error) {
	_, err := os.Stat(path)
	if err == nil {
		return true, nil
	}
	if os.IsNotExist(err) {
		return false, nil
	}
	return false, fmt.Errorf("an error occured while checking if the path \"%s\" exists: %w", path, err)
}

func walkAndDeArchive(archivePath string, outputPath string) error {
	file, err := GetFileFromPath(archivePath, "")
	if err != nil {
		return fmt.Errorf("an error occured while trying to dearchive \"%s\": %w", archivePath, err)
	}

	if file.ID() != 0 {
		outputDir := filepath.Join(outputPath, filepath.Dir(archivePath))
		err = os.MkdirAll(outputDir, os.ModePerm)
		if err != nil {
			return fmt.Errorf("an error occured while trying to dearchive \"%s\": %w", archivePath, err)
		}
		err = file.Dearchive(outputDir)
		if err != nil {
			return fmt.Errorf("an error occured while trying to dearchive \"%s\": %w", archivePath, err)
		}
		return nil
	}

	directory, err := GetDirectoryFromPath(archivePath)
	if err != nil {
		return fmt.Errorf("an error occured while trying to dearchive \"%s\": %w", archivePath, err)
	}
	if directory.ID() != 0 {
		err = directory.Dearchive(outputPath)
		if err != nil {
			return fmt.Errorf("an error occured while trying to dearchive \"%s\": %w", archivePath, err)
		}
		return nil
	}

	return fmt.Errorf("an error occured while trying to dearchive \"%s\": the specified path is not in any archive", archivePath)
}
