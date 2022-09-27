# Archiver

Archiver is a multi platform tool for archiving large amounts of files.

:warning: This project may have bugs which could result in the loss of files, and should not be relyed on unless you are sure how to use it and fix problems that may arise.

It works by creating a revision entry for each file and calculating a hash of the file, if the hash is unique then the file is stored in a compressed multi-part archive, otherwise a pointer to the revision with the same hash is stored.
Each revision has an associated archive operation which can be used to dearchive specific revision of a file/directory instead of the latest version.

## Requirements

- A MySQL database, see [Database](#database). 
- zpaq must be installed and available in search path.

## Database

A MySQL database is required to run Archiver. Information for connecting to the database is stored in the configuration file, see [Configuration File](#configuration-file).

The database must have a specific structure and as such an SQL file is provided in **src/database/mysql_implementation/archvier_database.sql** which when run will create the required database.

:warning: It should be noted that only one such database can exist at a time.

## Usage

The general usage of Archiver is to stage paths, then archive them, and finally dearchive them. A path can either be a file or a directory;

### Staging paths
The first step in archiving paths is to stage them. Multiple paths can be staged at once and staging can be done multiple times.
```
Archiver stage [options] [--prefix <prefix>] [--paths] <paths>
```

For options see the [Options](#options) section.
`--prefix <string>` specifies a prifix string which is  to be removed from all `<paths>` if they start with it. If a path in `<paths>` does not start with `<prefix>` then the path is staged unaltered.

`--paths` is a optional specifier for `<paths>` and while it is recommended for clarity `<paths>` is a positional argument. `<paths>` is the list of paths which are to be staged.

### Archiving paths
After paths have been staged they can be archived. This will add them to compressed archives.
```
Archiver archive [options]
```

For options see the [Options](#options) section.

### Dearchiving paths
To get paths out of the compressed archives they have to be dearchived.
```
Archiver dearchive [options] [--number <num>] --output <out> [--paths] <paths>
```

For options see the [Options](#options) section.

`--number`, or `-n`, specifies that only paths which were archived during the archive operation `<num>` should be dearchived. If this option is not used then the latest version of each path is used. It should be noted that if two files were archived to the same directory but on different archive operations, then both files would be dearchived unless a specific archive operation is specified.

`--output`, or `-o`, specified where the paths being dearchived should be output to.

`--paths` is a optional specifier for `<paths>` and while it is recommended for clarity `<paths>` is a positional argument. `<paths>` is the list of paths which are to be dearchived.

:warning: It is highly recommended that after archiving paths, those same paths be dearchived in order to check that they were archived correctly.

### Uploading
While the command does exist it is not currently implemented, it will allow for uploading archives to a cloud service.

### Checking archives
This command checks that the files archived match what is in the database, this is done by decompressing the archives and then checking the hashes of the archived files against the hashes recorded in the database.
```
Archiver check [options]
```

For options see the [Options](#options) section.

### Options
These options are used for each command.
- `--help` : print the usage information
- `-v, --verbose` : tells the program to ouput additional information about what it is doing
- `--config <file>` : specify the configuration file to be used, see [Configuration File](#configuration-file).

### Configuration File
The configuration file is a JSON file which gives the program information about where resources are located and how many resources it can use. There is an example configuration file located at **src/config/default_config.json**.

The file contains a single JSON object which has 5 parts: general, stager, archive, database, and aws.
Each of these parts is a JSON object with its own properies.
- general
  - file\_read\_sizes : An array of numbers representing the sizes that the read buffer should try to use. These values will be tried in order until the buffer can be allocated or all values have been exhausted and the program reports an error. The buffer is used to read files during the stage and check operations.
- stager
  - stage\_directory : A string representing the directory in which staged files should be placed
- archive
  - archive\_directory : A string representing the directory in which archives parts can be found and should be placed.
  - temp\_archive\_directory : A string representating the directory in which archives parts should be combined into full archives and in which decompressed archives can be found.
  - targe\_size : A number representing the size at which an archive is considered full. An archive will likely go over this target size as the last file will be placed into the archive if the archive size is less then the target size. It should be noted that this is the decompressed archive target size.
  - single\_archive\_size : A number representing the size at which a file is considered too large to be placed in an archive and is archived by itself.
- database : Information required for connecting to the database
  - user : A string representing the user to connect using.
  - password : A string representing the password for the database user.
  - location : An object containing information about the database location.
    - host : A string representing the host of the database, ie. the IP address that the database can be found at.
    - port : A number representing the port to use when connecting to the database.
    - schema : A string representing the name of the database on the database server.
- aws : Information required for connecting to AWS, currently unused.
  - access\_key : A string representing the AWS access key.
  - secret\_key : A string representing the AWS secret key for the access key.