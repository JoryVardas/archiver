package main

import (
	sql "database/sql"
	"errors"
	"fmt"
	"io"
	log "log"
	"os"
	"path/filepath"
	"strconv"
	"time"

	"github.com/udhos/equalfile"
	blake2b "golang.org/x/crypto/blake2b"
	sha3 "golang.org/x/crypto/sha3"
)

var fileRevisionTimestampFormatString = "2006-01-02-15:04:05.000"

var archivesUpdated = make(map[uint64]Archive)
var singleFileArchivesUpdated = make(map[uint64]string)

type File struct {
	fileID           uint64
	parentID         uint64
	size             uint64
	filePath         string
	absoluteFilePath string
}

func NewFile(id uint64, parentID uint64, size uint64, path string, absoluteFilePath string) File {
	return File{id, parentID, size, path, absoluteFilePath}
}

var nilFile = File{0, 0, 0, "", ""}

type FileHash struct {
	blake2b []byte
	sha3    []byte
}

var nilFileHash = FileHash{nil, nil}

type FileRevision struct {
	parentID  uint64
	timeStamp time.Time
	hashs     *FileHash
	archive   Archive
}

func NewFileRevision(parentID uint64, timeStamp time.Time, hashs *FileHash) FileRevision {
	return FileRevision{parentID, timeStamp, hashs, nilArchive}
}

func (file *File) Size() uint64 {
	return file.size
}
func (file *File) Extension() string {
	extension := filepath.Ext(file.filePath)
	if len(extension) > 0 {
		return extension[1:]
	}
	return extension
}
func (file *File) Path() string {
	return file.filePath
}
func (file *File) AbsolutePath() string {
	return file.absoluteFilePath
}
func (file *File) Name() string {
	return filepath.Base(file.filePath)
}
func (file *File) Parent() uint64 {
	return file.parentID
}
func (file *File) ToString() string {
	return fmt.Sprintf("{id: %d, parent: %d, path: %s}", file.fileID, file.parentID, file.Path())
}
func (file *File) ID() uint64 {
	return file.fileID
}

func (revision *FileRevision) Parent() uint64 {
	return revision.parentID
}
func (revision *FileRevision) TimeStamp() time.Time {
	return revision.timeStamp
}
func (revision *FileRevision) Hashs() *FileHash {
	return revision.hashs
}
func (revision *FileRevision) Archive() Archive {
	return revision.archive
}
func (revision *FileRevision) GetRevisionName() string {
	return strconv.FormatUint(revision.Parent(), 10) + "_" + revision.TimeStamp().Format(fileRevisionTimestampFormatString)
}

func (revision *FileRevision) ToString() string {
	return fmt.Sprintf("{fileID: %d, revisionTimeStamp: %s}", revision.parentID, revision.timeStamp.Format(fileRevisionTimestampFormatString))
}

func copyFileToArchive(file *os.File, fileID uint64, archiveID uint64) error {
	var revisionArchiveTime time.Time
	revisionArchiveTimeErr := database.QuerySingleRow("database.file.getFileRevisionTimestampByFileID", []interface{}{fileID}, []interface{}{&revisionArchiveTime})
	if revisionArchiveTimeErr != nil {
		log.Println(revisionArchiveTimeErr)
		return errors.New("There was an error getting the time of archival for the file just added file")
	}

	archivePath := filepath.Join(config.Archive.TempArchiveDirectory, strconv.FormatUint(archiveID, 10))
	err := os.MkdirAll(archivePath, os.ModePerm)
	if err != nil {
		return fmt.Errorf("there was an error making the directory for the archive \"%s\": %w", archivePath, err)
	}

	newFilePath := filepath.Join(archivePath, strconv.FormatUint(fileID, 10)+"_"+revisionArchiveTime.Format(fileRevisionTimestampFormatString))

	newFile, err := os.OpenFile(newFilePath, os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		log.Println(err)
		return errors.New("there was an error creating the file in the archive directory")
	}
	defer newFile.Close()

	_, err = file.Seek(0, io.SeekStart)
	if err != nil {
		log.Println(err)
		return errors.New("there was an error moving to the begining of the file")
	}

	buffer := make([]byte, config.General.FileReadSize)
	_, err = io.CopyBuffer(newFile, file, buffer)
	if err != nil {
		log.Println(err)
		return errors.New("there was an error copying the file to the archive directory")
	}

	archivesUpdated[archiveID] = Archive{archiveID}
	if archiveID == 1 {
		singleFileArchivesUpdated[fileID] = strconv.FormatUint(fileID, 10) + "_" + revisionArchiveTime.Format(fileRevisionTimestampFormatString)
	}

	return nil
}

func GetFileFromPath(relativePath string, absolutePath string) (File, error) {
	var ret File
	var err error
	ret.filePath = relativePath
	ret.absoluteFilePath = absolutePath
	parentDirectory, err := GetDirectoryFromPath(filepath.Dir(relativePath))
	if err != nil {
		return nilFile, fmt.Errorf("an error occured while trying to get the file info from the path \"%s\": %w", relativePath, err)
	}
	ret.parentID = parentDirectory.ID()

	err = ret.getFileIDIfExists()
	if err != nil {
		return nilFile, fmt.Errorf("an error occured while trying to get the file info from the path \"%s\": %w", relativePath, err)
	}
	return ret, nil
}

func (file *File) Archive() error {
	if debug {
		fmt.Println("Archiving file " + file.ToString())
	}
	rawFile, err := os.Open(file.AbsolutePath())
	if err != nil {
		return fmt.Errorf("there was an error opening \"%s\": %w", file.AbsolutePath(), err)
	}
	defer rawFile.Close()

	rawFileInfo, err := rawFile.Stat()
	if err != nil {
		return fmt.Errorf("there was an error getting the file size: %w", err)
	}
	file.size = uint64(rawFileInfo.Size())

	if debug {
		fmt.Printf("\nFile size: %d\n", file.size)
	}

	if file.ID() == 0 {
		if debug {
			fmt.Printf("\nFile needs to be added\n")
		}
		err = addFileEntry(file)
		if err != nil {
			return fmt.Errorf("there was an error adding the file \"%s\": %w", file.Path(), err)
		}

		if debug {
			fmt.Printf("\nFile ID: %d\n", file.ID())
		}
	}

	fileHashs, err := getHashesForFile(rawFile)
	if err != nil {
		return fmt.Errorf("there was an error getting the hashes for the file \"%s\": %w", file.Path(), err)
	}

	duplicateRevision, err := findDuplicateFileRevision(file, fileHashs)
	if err != nil {
		return fmt.Errorf("there was an error checking if the file \"%s\" is a duplicate of an archived revision: %w", file.Path(), err)
	}
	if duplicateRevision != nil {
		if debug {
			fmt.Printf("\nFile is a duplicate of: %s\n", duplicateRevision.ToString())
		}
		err = addDuplicateFileRevision(file, duplicateRevision)
		if err != nil {
			return fmt.Errorf("there was an error adding the file \"%s\" as a duplicate of an archived revision: %w", file.Path(), err)
		}

		return nil
	}

	archive, err := GetArchiveForFile(file)
	if err != nil {
		return fmt.Errorf("an error occured while archiving the file %s: %w", file.ToString(), err)
	}
	if debug {
		fmt.Printf("\nArchive for file is archive ID: %d\n", archive.ID())
	}

	//Not actually what I want to do here, but this will prevent any file changes while giving
	// enough info fo me to use.
	if info {
		return nil
	}

	compressedArchiveExists, err := archive.Exists()
	if err != nil {
		return fmt.Errorf("an error occured while archiving the file %s: %w", file.ToString(), err)
	}

	if compressedArchiveExists {
		err = archive.LoadIfUnloaded()
		if err != nil {
			return fmt.Errorf("an error occured while archiving the file %s: %w", file.ToString(), err)
		}
	}

	err = addNewFileRevision(file, fileHashs, &archive)
	if err != nil {
		return fmt.Errorf("an error occured while archiving the file %s: %w", file.ToString(), err)
	}

	err = copyFileToArchive(rawFile, file.ID(), archive.ID())
	if err != nil {
		return fmt.Errorf("an error occured while archiving the file %s: %w", file.ToString(), err)
	}

	if debug {
		fmt.Println("File processed : " + file.Path())
	}

	return nil
}

func (file *File) Check() error {
	if file.ID() == 0 {
		return fmt.Errorf("the file {path: \"%s\", absolute path: \"%s\"}  was not added to the database", file.Path(), file.AbsolutePath())
	}

	mostRecentRevision, err := file.getMostRecentRevisionInfo()
	if err != nil {
		return fmt.Errorf("an error occured while trying to check the file %s: %w", file.ToString(), err)
	}

	revisionArchive := mostRecentRevision.Archive()
	err = revisionArchive.LoadIfUnloaded()
	if err != nil {
		return fmt.Errorf("an error occured while trying to check the file %s: %w", file.ToString(), err)
	}

	archivedRevisionPath := filepath.Join(revisionArchive.TempPath(), mostRecentRevision.GetRevisionName())

	cmp := equalfile.New(nil, equalfile.Options{})
	equal, err := cmp.CompareFile(file.AbsolutePath(), archivedRevisionPath)

	if err != nil {
		return fmt.Errorf("there was an error comparing the file %s to its archived version %s: %w", file.ToString(), mostRecentRevision.ToString(), err)
	}

	if !equal {
		return fmt.Errorf("the file %s does not match its archived version %s: %w", file.ToString(), mostRecentRevision.ToString(), err)
	}

	return nil
}

func (file *File) Dearchive(outputPath string) error {
	mostRecentRevision, err := file.getMostRecentRevisionInfo()
	if err != nil {
		return fmt.Errorf("an error occured while trying to dearchive the file %s: %w", file.ToString(), err)
	}

	revisionArchive := mostRecentRevision.Archive()
	if revisionArchive.ID() == 1 {
		err = revisionArchive.LoadSingleIfUnloaded(mostRecentRevision.GetRevisionName())
		if err != nil {
			return fmt.Errorf("an error occured while trying to dearchive the file %s: %w", file.ToString(), err)
		}
	} else {
		err = revisionArchive.LoadIfUnloaded()
		if err != nil {
			return fmt.Errorf("an error occured while trying to dearchive the file %s: %w", file.ToString(), err)
		}
	}

	archivedRevisionPath := filepath.Join(revisionArchive.TempPath(), mostRecentRevision.GetRevisionName())
	newFilePath := filepath.Join(outputPath, file.Name())

	rawFile, err := os.Open(archivedRevisionPath)
	if err != nil {
		return fmt.Errorf("there was an error opening \"%s\": %w", archivedRevisionPath, err)
	}
	defer rawFile.Close()

	newFile, err := os.OpenFile(newFilePath, os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		log.Println(err)
		return fmt.Errorf("there was an error creating the file \"%s\" in the dearchive directory while trying to dearchive \"%s\": %w", newFilePath, file.Path(), err)
	}
	defer newFile.Close()

	_, err = rawFile.Seek(0, io.SeekStart)
	if err != nil {
		log.Println(err)
		return errors.New("there was an error moving to the begining of the file")
	}

	buffer := make([]byte, config.General.FileReadSize)
	_, err = io.CopyBuffer(newFile, rawFile, buffer)
	if err != nil {
		log.Println(err)
		return errors.New("there was an error copying the file to the dearchive directory")
	}

	return nil
}

func (file *File) getMostRecentRevisionInfo() (FileRevision, error) {
	var revision FileRevision
	err := database.QuerySingleRow("database.file.getMostRecentRevisionInfoByFileID", []interface{}{file.ID(), file.ID()}, []interface{}{&revision.parentID, &revision.timeStamp, &revision.archive.archiveID})
	if err != nil {
		return revision, fmt.Errorf("an error occured while trying to get the most recent revision info for the file %s: %w", file.ToString(), err)
	}
	return revision, nil
}

func getHashesForFile(rawFile *os.File) (*FileHash, error) {
	sha512Hasher := sha3.New512()
	blake512Hasher, err := blake2b.New512(nil)
	if err != nil {
		return nil, fmt.Errorf("could not create new BLAKE2b-512 hasher: %w", err)
	}

	splitWriter := io.MultiWriter(sha512Hasher, blake512Hasher)
	buffer := make([]byte, config.General.FileReadSize)

	_, writeError := io.CopyBuffer(splitWriter, rawFile, buffer)
	if writeError != nil {
		return nil, fmt.Errorf("there was an error while generating the hashes: %w", writeError)
	}

	return &FileHash{sha512Hasher.Sum(nil), blake512Hasher.Sum(nil)}, nil
}

func addFileEntry(file *File) error {
	fileID, err := database.Insert("database.file.insert", []interface{}{file.Name(), file.Parent()})
	if err != nil {
		return fmt.Errorf("there was an error adding the entry for the file %s: %w", file.ToString(), err)
	}
	file.fileID = fileID
	return nil
}

func findDuplicateFileRevision(file *File, fileHash *FileHash) (*FileRevision, error) {
	var revisionParentID uint64
	var revisionArchiveTime time.Time

	err := database.QuerySingleRow("database.file.getRevisionBySizeAndHashs", []interface{}{file.Size(), fileHash.blake2b, fileHash.sha3}, []interface{}{&revisionParentID, &revisionArchiveTime})
	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			return nil, nil
		}
		return nil, fmt.Errorf("there was an error while checking if the file %s is a duplicate revision of an archived file: %w", file.ToString(), err)
	}

	return &FileRevision{revisionParentID, revisionArchiveTime, fileHash, nilArchive}, nil
}

func addDuplicateFileRevision(file *File, duplicateRevision *FileRevision) error {
	_, err := database.Insert("database.file.insertDuplicateRevision", []interface{}{file.ID(), duplicateRevision.Parent(), duplicateRevision.TimeStamp()})
	if err != nil {
		return fmt.Errorf("there was an error adding the file revision %s as a duplicate of %s: %w", file.ToString(), duplicateRevision.ToString(), err)
	}

	return nil
}

func addNewFileRevision(file *File, fileHashs *FileHash, archive *Archive) error {
	_, err := database.Insert("database.file.insertNewRevision", []interface{}{file.ID(), file.Size(), archive.ID(), fileHashs.blake2b, fileHashs.sha3})
	if err != nil {
		return fmt.Errorf("there was an error adding the file revision %s to the archive with id %d: %w", file.ToString(), archive.ID(), err)
	}

	return nil
}

func (file *File) getFileIDIfExists() error {
	err := database.QuerySingleRow("database.file.getByNameAndParent", []interface{}{file.Name(), file.Parent()}, []interface{}{&file.fileID})
	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			return nil
		}
		return fmt.Errorf("there was an error getting the file id for the file %s: %w", file.ToString(), err)
	}
	return nil
}
