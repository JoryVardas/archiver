DROP
    DATABASE IF EXISTS `archiver`;

CREATE
    DATABASE `archiver`;

USE
    `archiver`;

CREATE TABLE `directory`
(
    `id`        BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`      VARCHAR(1024)   NOT NULL,
    `parent_id` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`id`),
    FOREIGN KEY (`parent_id`) REFERENCES `directory` (`id`)
);

INSERT INTO `directory` (`name`, `parent_id`)
VALUES ("/", 1);

CREATE TABLE `file`
(
    `id`        BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`      VARCHAR(1024)   NOT NULL,
    `parent_id` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`id`),
    FOREIGN KEY (`parent_id`) REFERENCES `directory` (`id`)
);

CREATE TABLE `archive`
(
    `id`       BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `contents` VARCHAR(255)    NOT NULL,
    PRIMARY KEY (`id`)
);

INSERT INTO `archive` (`contents`)
VALUES ("<SINGLE>");

CREATE TABLE `file_revision`
(
    `revision_id`  BIGINT UNSIGNED NOT NULL AUTOINCREMENT,
    `file_id`      BIGINT UNSIGNED NOT NULL,
    `blake2b`      CHAR(128)       NOT NULL,
    `sha3`         CHAR(128)       NOT NULL,
    `size`         BIGINT UNSIGNED NOT NULL,
    `archive_time` DATETIME(2)     NOT NULL,
    `archive`      BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`revision_id`),
    FOREIGN KEY (`file_id`) REFERENCES `file` (`id`),
    FOREIGN KEY (`archive`) REFERENCES `archive` (`id`)
);

CREATE TABLE `file_duplicate`
(
    `id`                    BIGINT UNSIGNED NOT NULL AUTOINCREMENT,
    `file_id`               BIGINT UNSIGNED NOT NULL,
    `archive_time`          DATETIME(2)     NOT NULL,
    `duplicate_revision_id` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`id`),
    FOREIGN KEY (`file_id`) REFERENCES `file` (`id`),
    FOREIGN KEY (`duplicate_revision_id`) REFERENCES `file_revision` (`revision_id`)
);

CREATE TABLE `staged_directory`
(
    `id`        BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`      VARCHAR(1024)   NOT NULL,
    `parent_id` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`id`),
    FOREIGN KEY (`parent_id`) REFERENCES `staged_directory` (`id`)
);

CREATE TABLE `staged_file`
(
    `id`        BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`      VARCHAR(1024)   NOT NULL,
    `parent_id` BIGINT UNSIGNED NOT NULL,
    `blake2b`   CHAR(128)       NOT NULL,
    `sha3`      CHAR(128)       NOT NULL,
    `size`      BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`id`),
    FOREIGN KEY (`parent_id`) REFERENCES `directory` (`id`)
);