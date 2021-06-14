package main

import (
	"errors"
	"io/ioutil"
	log "log"
	"os/exec"

	"bytes"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	//"time"

	"github.com/udhos/equalfile"
)

type Archive struct {
	archiveID uint64
}

var nilArchive = Archive{0}

func (archive *Archive) IsSingleFileArchive() bool {
	return archive.archiveID == 1
}

func (archive *Archive) ID() uint64 {
	return archive.archiveID
}

func (archive *Archive) TempPath() string {
	return filepath.Join(config.Archive.TempArchiveDirectory, strconv.FormatUint(archive.ID(), 10))
}

func (archive *Archive) CommpressedPath() string {
	return filepath.Join(config.Archive.ArchiveDirectory, strconv.FormatUint(archive.ID(), 10)) + ".7z"
}

func (archive *Archive) CommpressedPartPath() string {
	return filepath.Join(config.Archive.ArchiveDirectory, strconv.FormatUint(archive.ID(), 10)) + "_part.7z"
}
func (archive *Archive) CompressedOldPartPath() string {
	return filepath.Join(config.Archive.ArchiveDirectory, strconv.FormatUint(archive.ID(), 10)) + "_part_old.7z"
}

func (archive *Archive) Size() (uint64, error) {
	var archiveSize uint64
	err := database.QuerySingleRow("database.archive.getSizeByID", []interface{}{archive.archiveID}, []interface{}{&archiveSize})
	if err != nil {
		return 0, fmt.Errorf("could not get the size of the archive with id %d: %w", archive.archiveID, err)
	}

	return archiveSize, nil
}

func (archive *Archive) FitsFile(file *File) (bool, error) { //fileSize uint64, archiveID uint64, archiveSize uint64) bool {
	if debug {
		fmt.Printf("\nDoes archive %d fit the file %s\n", archive.ID(), file.ToString())
	}
	if archive.IsSingleFileArchive() {
		if debug {
			fmt.Printf("\nArchive is the single file archive (always fits)\n")
		}
		return true, nil
	}

	archiveSize, err := archive.Size()
	if err != nil {
		return false, fmt.Errorf("could not check if the specified file fits in the archive with id %d: %w", archive.archiveID, err)
	}
	if debug {
		fmt.Printf("\nArchive Size: %d\n", archiveSize)
	}

	if archiveSize > config.Archive.TargetSize {
		return false, nil
	}
	if (config.Archive.TargetSize - archiveSize) >= file.Size() {
		if debug {
			fmt.Printf("\nFile fits\n")
		}
		return true, nil
	}
	if debug {
		fmt.Printf("\nFile does not fit\n")
	}
	return false, nil
}

func GetArchiveForFile(file *File) (Archive, error) {
	if debug {
		fmt.Printf("\nGetting the archive for the file %s\n", file.ToString())
	}

	var archive Archive
	var err error

	if file.Extension() == "" {
		if debug {
			fmt.Printf("\nArchive set will be <BLANK>\n")
		}
		archive, err = getArchiveForContents("<BLANK>")
		if err != nil {
			return nilArchive, fmt.Errorf("could not get archive for file \"%s\": %w", file.Path(), err)
		}
		if debug {
			fmt.Printf("\nArchive ID: %d\n", archive.ID())
		}
	}

	if file.Size() >= config.Archive.SingleArchiveSize {
		if debug {
			fmt.Printf("\nArchive set will be <SINGLE>\n")
		}
		archive, err = getArchiveForContents("<SINGLE>")
		if err != nil {
			return nilArchive, fmt.Errorf("could not get archive for file \"%s\": %w", file.Path(), err)
		}
		if debug {
			fmt.Printf("\nArchive ID: %d\n", archive.ID())
		}
		return archive, nil
	}

	archive, err = getArchiveForContents(file.Extension())
	if err != nil {
		return nilArchive, fmt.Errorf("could not get archive for file \"%s\": %w", file.Path(), err)
	}
	if debug {
		fmt.Printf("\nArchive ID: %d\n", archive.ID())
	}

	if archive == nilArchive {
		if debug {
			fmt.Printf("\nNo archive was found, need to add a new archive.\n")
		}
		archive, err = addArchiveForContents(file.Extension())
		if err != nil {
			return nilArchive, fmt.Errorf("could not get archive for file \"%s\": %w", file.Path(), err)
		}
		if debug {
			fmt.Printf("\nArchive ID: %d\n", archive.ID())
		}

		return archive, nil
	}

	fileFitsInArchive, err := archive.FitsFile(file)
	if err != nil {
		return nilArchive, fmt.Errorf("could not get archive for file \"%s\": %w", file.Path(), err)
	}
	if debug {
		fmt.Printf("\nDoes the file fit in the archive?: %t\n", fileFitsInArchive)
	}
	if !fileFitsInArchive {
		if debug {
			fmt.Printf("\nFile did not fit in archive, adding a new one\n")
		}
		archive, err = addArchiveForContents(file.Extension())
		if err != nil {
			return nilArchive, fmt.Errorf("could not get archive for file \"%s\": %w", file.Path(), err)
		}

		if debug {
			fmt.Printf("\nnew archive ID: %d\n", archive.ID())
		}
	}

	return archive, nil
}

func (archive *Archive) IsFull() (bool, error) {
	contents, err := archive.Contents()
	if err != nil {
		return false, fmt.Errorf("could not check if the archive with id %d is full: %w", archive.archiveID, err)
	}

	currentArchive, err := getArchiveForContents(contents)
	if err != nil {
		return false, fmt.Errorf("could not check if the archive with id %d is full: %w", archive.archiveID, err)
	}

	return archive.archiveID != currentArchive.archiveID, nil
}

func (archive *Archive) Contents() (string, error) {
	var archiveContents string
	err := database.QuerySingleRow("database.archive.getContentsByID", []interface{}{archive.archiveID}, []interface{}{&archiveContents})
	if err != nil {
		return "", fmt.Errorf("could not get the cointents of the archive with id %d: %w", archive.archiveID, err)
	}

	return archiveContents, nil
}

func compressArchive(compressedArchiveName string, compressedArchiveLocation string, archiveLocation string) error {
	cmd := exec.Command("7z", "a", "-m0=LZMA2", "-mx9", "-myx=0", "-ms=on", "-mhc=on", compressedArchiveName, archiveLocation)

	var outStream bytes.Buffer
	var errStream bytes.Buffer
	cmd.Stdout = &outStream
	cmd.Stderr = &errStream

	// Create the compressed archive location if it doesn't already exist.
	if _, err := os.Stat(compressedArchiveLocation); os.IsNotExist(err) {
		os.Mkdir(compressedArchiveLocation, os.ModePerm)
	}

	cmd.Dir = compressedArchiveLocation

	err := cmd.Run()

	fmt.Println(outStream.String())

	if err != nil {
		fmt.Println(errStream.String())
		log.Println(err)
		return errors.New("There was an error compressing the archive")
	}

	return nil
}

func compressModifiedArchives() error {
	compressedArchiveLocation := config.Archive.ArchiveDirectory

	for archiveID, archive := range archivesUpdated {
		if archiveID == 1 {
			fmt.Println("Compressing single file archives")

			for _, singleArchiveFileName := range singleFileArchivesUpdated {
				fmt.Printf("Compressing %s", singleArchiveFileName)
				err := compressArchive(singleArchiveFileName+".7z", filepath.Join(compressedArchiveLocation, "1"), filepath.Join(config.Archive.TempArchiveDirectory, "1", singleArchiveFileName))
				if err != nil {
					return err
				}
			}
		} else {
			fmt.Printf("Compressing archive %d", archiveID)

			archiveExists, err := archive.Exists()
			if err != nil {
				return fmt.Errorf("an error occured while compressing the modified archive with id %d: %w", archive.ID(), err)
			}
			if archiveExists {
				os.Rename(archive.CommpressedPartPath(), archive.CompressedOldPartPath())
			}

			archiveFull, err := archive.IsFull()
			if err != nil {
				return err
			}
			if archiveFull {
				err = compressArchive(filepath.Base(archive.CommpressedPath()), filepath.Dir(archive.CommpressedPath()), filepath.Join(archive.TempPath(), "*"))
				if err != nil {
					return err
				}
			} else {
				err = compressArchive(filepath.Base(archive.CommpressedPartPath()), filepath.Dir(archive.CommpressedPartPath()), filepath.Join(archive.TempPath(), "*"))
				if err != nil {
					return err
				}
			}
		}
	}

	fmt.Println("All modified archives were compressed successfully.")
	return nil
}

func checkModifiedArchives(ignoreChecks bool) error {
	if debug {
		fmt.Printf("\nChecking modified archives.\n")
	}
	for _, archive := range archivesUpdated {
		if archive.ID() == 1 {

			if !ignoreChecks {
				for _, revisionName := range singleFileArchivesUpdated{
					err = archive.decompressSingle(filepath.Join(config.Archive.TempArchiveDirectory, "new"), revisionName)
					if err != nil {
						return fmt.Errorf("an error occured while checking the modified archive with id %d and name \"%s\": %w", archive.ID(), revisionName, err)
					}
				}
	
				newArchivePath := filepath.Join(config.Archive.TempArchiveDirectory, "new", "1")
				archivePath := archive.TempPath()
	
				files, err := ioutil.ReadDir(archivePath)
				if err != nil {
					return fmt.Errorf("an error occured while checking the modified archive with id %d: %w", archive.ID(), err)
				}
	
				for _, file := range files {
					if debug {
						fmt.Printf("\nChecking file: %s\n", file.Name())
					}
					cmp := equalfile.New(nil, equalfile.Options{})
					equal, err := cmp.CompareFile(filepath.Join(newArchivePath, file.Name()), filepath.Join(archivePath, file.Name()))
	
					if err != nil {
						return fmt.Errorf("there was an error comparing the file \"%s\" from the archive with id %d with its conterpart from the compressed archive: %w", file.Name(), archive.ID(), err)
					}
					if !equal {
						return fmt.Errorf("the file \"%s\" from the archive with id %d did not match its conterpart from the compressed archive: %w", file.Name(), archive.ID(), err)
					}
				}
			}
			

			continue
		}
		if debug {
			fmt.Printf("\nChecking modified archive %d.\n", archive.ID())
		}

		//check that the archive was compressed without error
		if !ignoreChecks {
			err = archive.decompress(filepath.Join(config.Archive.TempArchiveDirectory, "new"))
			if err != nil {
				return fmt.Errorf("an error occured while checking the modified archive with id %d: %w", archive.ID(), err)
			}

			newArchivePath := filepath.Join(config.Archive.TempArchiveDirectory, "new", strconv.FormatUint(archive.ID(), 10))
			archivePath := archive.TempPath()

			files, err := ioutil.ReadDir(archivePath)
			if err != nil {
				return fmt.Errorf("an error occured while checking the modified archive with id %d: %w", archive.ID(), err)
			}

			for _, file := range files {
				if debug {
					fmt.Printf("\nChecking file: %s\n", file.Name())
				}
				cmp := equalfile.New(nil, equalfile.Options{})
				equal, err := cmp.CompareFile(filepath.Join(newArchivePath, file.Name()), filepath.Join(archivePath, file.Name()))

				if err != nil {
					return fmt.Errorf("there was an error comparing the file \"%s\" from the archive with id %d with its conterpart from the compressed archive: %w", file.Name(), archive.ID(), err)
				}
				if !equal {
					return fmt.Errorf("the file \"%s\" from the archive with id %d did not match its conterpart from the compressed archive: %w", file.Name(), archive.ID(), err)
				}
			}
		}


		//Check the old archive part
		archiveExists, err := doesPathExist(archive.CompressedOldPartPath())
		if debug {
			fmt.Printf("\nDoes the archive have a compressed old part? %t\n", archiveExists)
		}
		if err != nil {
			return fmt.Errorf("an error occured while checking the modified archive with id %d: %w", archive.ID(), err)
		}
		if archiveExists {
			if !ignoreChecks {
				if debug {
					fmt.Printf("\nArchive has a compressed old part, so decompress it to check if it can be safely removed.\n")
				}
				err = archive.decompressOld(filepath.Join(config.Archive.TempArchiveDirectory, "old"))
				if err != nil {
					return fmt.Errorf("an error occured while checking the modified archive with id %d: %w", archive.ID(), err)
				}

				oldArchivePath := filepath.Join(config.Archive.TempArchiveDirectory, "old", strconv.FormatUint(archive.ID(), 10))
				archivePath := archive.TempPath()

				files, err := ioutil.ReadDir(oldArchivePath)
				if err != nil {
					return fmt.Errorf("an error occured while checking the modified archive with id %d: %w", archive.ID(), err)
				}

				if debug {
					fmt.Printf("\nCheck the old archive against the new one.\nOld archive location: %s\nNew archive location: %s\n", oldArchivePath, archivePath)
				}

				for _, file := range files {
					if debug {
						fmt.Printf("\nChecking file: %s\n", file.Name())
					}
					cmp := equalfile.New(nil, equalfile.Options{})
					equal, err := cmp.CompareFile(filepath.Join(oldArchivePath, file.Name()), filepath.Join(archivePath, file.Name()))

					if err != nil {
						return fmt.Errorf("there was an error comparing the file \"%s\" from the archive with id %d with its conterpart from the previous archive part: %w", file.Name(), archive.ID(), err)
					}
					if !equal {
						return fmt.Errorf("the file \"%s\" from the archive with id %d did not match its conterpart from the previous archive part: %w", file.Name(), archive.ID(), err)
					}
				}
			}
			if ignoreChecks && debug {
				fmt.Printf("\nIgnoring partial archive checks!\n")
			}
			if debug {
				fmt.Printf("\nRemoving old archive part at: %s\n", archive.CompressedOldPartPath())
			}
			err = os.Remove(archive.CompressedOldPartPath())
			if err != nil {
				return fmt.Errorf("there was an error while trying to remove the old archive part \"%s\" after checking that it was no longer needed: %w", archive.CompressedOldPartPath(), err)
			}
		}
	}

	fmt.Println("All modified archives were compressed successfully.")
	return nil
}

func addArchiveForContents(contents string) (Archive, error) {
	archiveID, err := database.Insert("database.archive.insert", []interface{}{contents})
	if err != nil {
		return nilArchive, fmt.Errorf("there was an error adding an the archive for contents \"%s\": %w", contents, err)
	}
	return Archive{archiveID}, nil
}

func getArchiveForContents(contents string) (Archive, error) {
	if debug {
		fmt.Printf("\nGetting archive for contents: %s\n", contents)
	}
	var archiveID uint64
	err := database.QuerySingleRow("database.archive.getCurrentIDForContents", []interface{}{contents}, []interface{}{&archiveID})
	if err != nil {
		return nilArchive, fmt.Errorf("there was an error getting the current archive for contents \"%s\": %w", contents, err)
	}
	if debug {
		fmt.Printf("\nArchive found ID: %d\n", archiveID)
	}
	return Archive{archiveID}, nil
}

func archiveExistsForContents(contents string) (bool, error) {
	if debug {
		fmt.Printf("\nDoes an archive exist for contents: %s\n", contents)
	}
	archive, err := getArchiveForContents(contents)
	if err != nil {
		return false, fmt.Errorf("there was an error checking if an archive exists for contents \"%s\": %w", contents, err)
	}

	if debug {
		fmt.Printf("\nArchive exists: %t\n", archive != nilArchive)
	}

	return archive != nilArchive, nil
}

func (archive *Archive) LoadIfUnloaded() error {
	loaded, err := archive.isLoaded()
	if err != nil {
		return fmt.Errorf("an error occured while checking if the archive with id %d was loaded: %w", archive.ID(), err)
	}
	if loaded {
		return nil
	}

	err = archive.decompress(config.Archive.TempArchiveDirectory)
	if err != nil {
		return fmt.Errorf("an error occured while trying to load the unloaded archive with id %d: %w", archive.ID(), err)
	}

	err = archive.CheckArchiveHashes()
	if err != nil {
		return fmt.Errorf("an error occured while trying to load the unloaded archive with id %d: %w", archive.ID(), err)
	}

	return nil
}

func (archive *Archive) LoadSingleIfUnloaded(revisionName string) error {
	loaded, err := archive.isSingleLoaded(revisionName)
	if err != nil {
		return fmt.Errorf("an error occured while checking if the single archive with revision name %s was loaded: %w", revisionName, err)
	}
	if loaded {
		return nil
	}

	err = archive.decompressSingle(config.Archive.TempArchiveDirectory, revisionName)
	if err != nil {
		return fmt.Errorf("an error occured while trying to load the unloaded single archive with revision name %s: %w", revisionName, err)
	}
	return nil
}

func (archive *Archive) isLoaded() (bool, error) {
	exists, err := doesPathExist(archive.TempPath())
	if err != nil {
		return false, fmt.Errorf("an error occured while checking if the archive with id %d is loaded: %w", archive.ID(), err)
	}
	return exists, err
}
func (archive *Archive) isSingleLoaded(revisionName string) (bool, error) {
	exists, err := doesPathExist(filepath.Join(archive.TempPath(), revisionName))
	if err != nil {
		return false, fmt.Errorf("an error occured while checking if the single archive with revision name %s is loaded: %w", revisionName, err)
	}
	return exists, err
}
func (archive *Archive) Exists() (bool, error) {
	fullExists, err := doesPathExist(archive.CommpressedPath())
	if err != nil {
		return false, fmt.Errorf("an error occured while checking if the archive with id %d has a compressed archive: %w", archive.ID(), err)
	}
	partExists, err := doesPathExist(archive.CommpressedPartPath())
	if err != nil {
		return false, fmt.Errorf("an error occured while checking if the archive with id %d has a compressed part archive: %w", archive.ID(), err)
	}

	return fullExists || partExists, nil
}

func (archive *Archive) decompress(containingPath string) error {
	var cmd *exec.Cmd

	partExists, err := doesPathExist(archive.CommpressedPartPath())
	if err != nil {
		return fmt.Errorf("an error occured while checking if the archive with id %d is an archive part: %w", archive.ID(), err)
	}
	if partExists {
		cmd = exec.Command("7z", "e", "-o"+strconv.FormatUint(archive.ID(), 10), archive.CommpressedPartPath())
	} else {
		cmd = exec.Command("7z", "e", "-o"+strconv.FormatUint(archive.ID(), 10), archive.CommpressedPath())
	}

	var outStream bytes.Buffer
	var errStream bytes.Buffer
	cmd.Stdout = &outStream
	cmd.Stderr = &errStream

	if _, err := os.Stat(containingPath); os.IsNotExist(err) {
		os.Mkdir(containingPath, os.ModePerm)
	}

	cmd.Dir = containingPath

	err = cmd.Run()

	fmt.Println(outStream.String())

	if err != nil {
		fmt.Println(errStream.String())
		return fmt.Errorf("there was an error decompressing the archive with id %d to \"%s\": %w", archive.ID(), containingPath, err)
	}

	return nil
}
func (archive *Archive) decompressOld(containingPath string) error {
	var cmd *exec.Cmd

	cmd = exec.Command("7z", "e", "-o"+strconv.FormatUint(archive.ID(), 10), archive.CommpressedOldPartPath())

	var outStream bytes.Buffer
	var errStream bytes.Buffer
	cmd.Stdout = &outStream
	cmd.Stderr = &errStream

	if _, err := os.Stat(containingPath); os.IsNotExist(err) {
		os.Mkdir(containingPath, os.ModePerm)
	}

	cmd.Dir = containingPath

	err = cmd.Run()

	fmt.Println(outStream.String())

	if err != nil {
		fmt.Println(errStream.String())
		return fmt.Errorf("there was an error decompressing the archive with id %d to \"%s\": %w", archive.ID(), containingPath, err)
	}

	return nil
}

func (archive *Archive) decompressSingle(containingPath string, revisionName string) error {
	var cmd *exec.Cmd

	cmd = exec.Command("7z", "e", "-o1", filepath.Join(config.Archive.ArchiveDirectory, "1", revisionName+".7z"))

	var outStream bytes.Buffer
	var errStream bytes.Buffer
	cmd.Stdout = &outStream
	cmd.Stderr = &errStream

	if _, err := os.Stat(containingPath); os.IsNotExist(err) {
		os.Mkdir(containingPath, os.ModePerm)
	}

	cmd.Dir = containingPath

	err := cmd.Run()

	fmt.Println(outStream.String())

	if err != nil {
		fmt.Println(errStream.String())
		return fmt.Errorf("there was an error decompressing the single archive with revision name %s to \"%s\": %w", revisionName, containingPath, err)
	}

	return nil
}

func (archive *Archive) GetHashes() (*FileHash, error) {
	fmt.Printf("\nGenerating hashes for archive with id %d \n", archive.ID())

	exists, err := doesPathExist(archive.CommpressedPath())
	if err != nil {
		return nil, fmt.Errorf("there was an error while checking if the archive with id %d exists: %w", archive.ID(), err)
	}

	var rawFile *os.File
	if exists {
		rawFile, err = os.Open(archive.CommpressedPath())
		if err != nil {
			return nil, fmt.Errorf("there was an error opening \"%s\": %w", archive.CommpressedPath(), err)
		}
	} else {
		rawFile, err = os.Open(archive.CommpressedPartPath())
		if err != nil {
			return nil, fmt.Errorf("there was an error opening \"%s\": %w", archive.CommpressedPartPath(), err)
		}
	}
	defer rawFile.Close()

	fileHashs, err := getHashesForFile(rawFile)
	if err != nil {
		return nil, fmt.Errorf("there was an error getting the hashes for the archive with id %d: %w", archive.ID(), err)
	}

	return fileHashs, nil
}

func (archive *Archive) UpdateArchiveHashes() error {
	if archive.ID() == 1 {
		return updateSingleArchiveHashes()
	}
	fileHashs, err := archive.GetHashes()
	if err != nil {
		return fmt.Errorf("there was an error getting the archive hashs: %w", err)
	}

	rowsAffected, err := database.Update("database.archive.updateHashsByID", []interface{}{fileHashs.blake2b, fileHashs.sha3, archive.ID()})
	if err != nil || rowsAffected != 1 {
		return fmt.Errorf("there was an error updating the hashes for the archive with id %d: %w", archive.ID(), err)
	}
	return nil
}
func (archive *Archive) UpdateModifiedArchiveHashes() error {
	if archive.ID() == 1 {
		return updateModifiedSingleArchiveHashes()
	}
	fileHashs, err := archive.GetHashes()
	if err != nil {
		return fmt.Errorf("there was an error getting the archive hashs: %w", err)
	}

	rowsAffected, err := database.Update("database.archive.updateHashsByID", []interface{}{fileHashs.blake2b, fileHashs.sha3, archive.ID()})
	if err != nil || rowsAffected != 1 {
		return fmt.Errorf("there was an error updating the hashes for the archive with id %d: %w", archive.ID(), err)
	}
	return nil
}
func UpdateAllModifedArchiveHashes() error {
	for _, archive := range archivesUpdated {
		err := archive.UpdateModifiedArchiveHashes
		if err != nil {
			return err
		}
	}

	return nil
}
func updateSingleArchiveHashes() error {
	singleArchives, err := database.Query("database.file.getAllSingleArchiveFileRevisons", []interface{}{})
	if err != nil {
		return fmt.Errorf("there was an error getting the list of single archives: %w", err)
	}
	//defer singleArchives.Close()

	//var fileID uint64
	//var archiveTime time.Time

	var fileRevisions []FileRevision

	for singleArchives.Next() {
		var revision FileRevision
		err = singleArchives.Scan(&revision.parentID, &revision.timeStamp)
		if err != nil {
			return fmt.Errorf("there was an error while getting the id and archive time for a single file archive: %w", err)
		}
		fileRevisions = append(fileRevisions, revision)
	}

	singleArchives.Close()

	for _, revision := range fileRevisions{

	//for singleArchives.Next() {
	//	err = singleArchives.Scan(&fileID, &archiveTime)
	//	if err != nil {
	//		return fmt.Errorf("there was an error while getting the file id and archive time for a single file archive: %w", err)
	//	}

		archiveLocation := filepath.Join(config.Archive.ArchiveDirectory, "1", revision.GetRevisionName()) + ".7z"

		fmt.Printf("\n\tProcessing single file archive %s", revision.GetRevisionName() + ".7z")

		rawFile, err := os.Open(archiveLocation)
		if err != nil {
			return fmt.Errorf("there was an error opening \"%s\": %w", archiveLocation, err)
		}

		fmt.Printf("\n\t\tGenerating hashes")
	
		fileHashs, err := getHashesForFile(rawFile)
		if err != nil {
			rawFile.Close()
			return fmt.Errorf("there was an error getting the hashes for the single file archive with file id %d and timestamp %s: %w", revision.Parent(), revision.TimeStamp().Format(fileRevisionTimestampFormatString), err)
		}
		rawFile.Close()

		fmt.Printf("\n\t\tUpdating database")
		_, err = database.Insert("database.archive.replaceSingleHash", []interface{}{revision.Parent(), revision.TimeStamp(), fileHashs.blake2b, fileHashs.sha3})
		if err != nil {
			return fmt.Errorf("there was an error replacing the hashes for the single archive with id %d and timestamp %s: %w", revision.Parent(), revision.TimeStamp().Format(fileRevisionTimestampFormatString), err)
		}
	}

	return nil
}
func updateModifiedSingleArchiveHashes() error {
	//singleArchives, err := database.Query("database.file.getAllSingleArchiveFileRevisons", []interface{}{})
	//if err != nil {
	//	return fmt.Errorf("there was an error getting the list of single archives: %w", err)
	//}
	//defer singleArchives.Close()

	//var fileID uint64
	//var archiveTime time.Time

	var fileRevisions []FileRevision
	var err error

	for fileID, revisionName := range singleFileArchivesUpdated {
		revisionNameParts := strings.Split(singleFileArchivesUpdated[fileID], "_")


		var revision FileRevision
		revision.parentID = fileID
		fileRevision.timeStamp, err = time.Parse(revisionNameParts[1], fileRevisionTimestampFormatString)
		
		if err != nil {
			return fmt.Errorf("an error occured while trying to parse the revision name of a single file archive (%s) into a timestamp: %w", revisionNameParts[1], err)
		}

		fileRevisions = append(fileRevisions, revision)
	}

	for _, revision := range fileRevisions {

		archiveLocation := filepath.Join(config.Archive.ArchiveDirectory, "1", revision.GetRevisionName()) + ".7z"

		fmt.Printf("\n\tProcessing single file archive %s", revision.GetRevisionName() + ".7z")

		rawFile, err := os.Open(archiveLocation)
		if err != nil {
			return fmt.Errorf("there was an error opening \"%s\": %w", archiveLocation, err)
		}

		fmt.Printf("\n\t\tGenerating hashes")
	
		fileHashs, err := getHashesForFile(rawFile)
		if err != nil {
			rawFile.Close()
			return fmt.Errorf("there was an error getting the hashes for the single file archive with file id %d and timestamp %s: %w", revision.Parent(), revision.TimeStamp().Format(fileRevisionTimestampFormatString), err)
		}
		rawFile.Close()

		fmt.Printf("\n\t\tUpdating database")
		_, err = database.Insert("database.archive.replaceSingleHash", []interface{}{revision.Parent(), revision.TimeStamp(), fileHashs.blake2b, fileHashs.sha3})
		if err != nil {
			return fmt.Errorf("there was an error replacing the hashes for the single archive with id %d and timestamp %s: %w", revision.Parent(), revision.TimeStamp().Format(fileRevisionTimestampFormatString), err)
		}
	}

	return nil
}

func (archive *Archive) CheckArchiveHashes() error {
	if archive.ID() == 1 {
		return checkSingleArchiveHashes()
	}
	fileHashs, err := archive.GetHashes()
	if err != nil {
		return fmt.Errorf("there was an error getting the archive hashs: %w", err)
	}

	var archiveHashs FileHash
	err = database.QuerySingleRow("database.archive.getHashsByID", []interface{}{archive.ID()}, []interface{}{&archiveHashs.blake2b, &archiveHashs.sha3})
	if err != nil {
		return fmt.Errorf("there was an error getting the hashes for the archive with id %d: %w", archive.ID(), err)
	}
	
	resultBlake := bytes.Compare(fileHashs.blake2b, archiveHashs.blake2b)
	resultSha := bytes.Compare(fileHashs.sha3, archiveHashs.sha3)

	if resultBlake != 0 || resultSha != 0 {
		return fmt.Errorf("the archive with id %d did not match the recorded hashs", archive.ID())
	}
	
	return nil
}
func checkSingleArchiveHashes() error {
	singleArchives, err := database.Query("database.archive.getSingleHashs", []interface{}{})
	if err != nil {
		return fmt.Errorf("there was an error getting the list of single archive hashs: %w", err)
	}
	//defer singleArchives.Close()

	//var fileID uint64
	//var archiveTime time.Time
	//var archiveHashs FileHash

	//var fileID uint64
	//var archiveTime time.Time

	var fileRevisions []FileRevision

	for singleArchives.Next() {
		var revision FileRevision
		var archiveHashs FileHash
		err = singleArchives.Scan(&revision.parentID, &revision.timeStamp, &archiveHashs.blake2b, &archiveHashs.sha3)
		if err != nil {
			return fmt.Errorf("there was an error while getting the id and archive time for a single file archive: %w", err)
		}
		revision.hashs = &archiveHashs
		fileRevisions = append(fileRevisions, revision)
	}

	singleArchives.Close()

	for _, revision := range fileRevisions{

	//for singleArchives.Next() {
	//	err = singleArchives.Scan(&fileID, &archiveTime, &archiveHashs.blake2b, &archiveHashs.sha3)
	//	if err != nil {
	//		return fmt.Errorf("there was an error while getting the id, archive time, and hashes for a single file archive: %w", err)
	//	}

		archiveLocation := filepath.Join(config.Archive.ArchiveDirectory, "1", revision.GetRevisionName()) + ".7z"

		fmt.Printf("\n\tProcessing single file archive %s",revision.GetRevisionName() + ".7z")

		rawFile, err := os.Open(archiveLocation)
		if err != nil {
			return fmt.Errorf("there was an error opening \"%s\": %w", archiveLocation, err)
		}

		fmt.Printf("\n\t\tGenerating hashes")
	
		fileHashs, err := getHashesForFile(rawFile)
		if err != nil {
			rawFile.Close()
			return fmt.Errorf("there was an error getting the hashes for the archive with file id %d and timestamp %s: %w", revision.Parent(), revision.TimeStamp().Format(fileRevisionTimestampFormatString), err)
		}
		rawFile.Close()


		fmt.Printf("\n\t\tComparing hashes")
		
		resultBlake := bytes.Compare(fileHashs.blake2b, revision.Hashs().blake2b)
		resultSha := bytes.Compare(fileHashs.sha3, revision.Hashs().sha3)

		if resultBlake != 0 || resultSha != 0 {
			return fmt.Errorf("the single file archive with id %d and timestamp %s did not match the recorded hashs", revision.Parent(), revision.TimeStamp().Format(fileRevisionTimestampFormatString))
		}
	}

	return nil
}