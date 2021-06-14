DROP DATABASE IF EXISTS `archiver`;

CREATE DATABASE `archiver`;

USE `archiver`;

CREATE TABLE `directory` (
	`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
	`name` VARCHAR(1024) NOT NULL,
	`parent_id` BIGINT UNSIGNED NOT NULL,
	PRIMARY KEY (`id`),
	FOREIGN KEY (`parent_id`) REFERENCES `directory`(`id`)
);

INSERT INTO `directory` (`name`, `parent_id`) VALUES ("/", 1);

CREATE TABLE `file` (
	`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
	`name` VARCHAR(1024) NOT NULL,
	`parent_id` BIGINT UNSIGNED NOT NULL,
	PRIMARY KEY (`id`),
	FOREIGN KEY (`parent_id`) REFERENCES `directory`(`id`)
);

CREATE TABLE `archive` (
	`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
	`contents` VARCHAR(255) NOT NULL,
	PRIMARY KEY (`id`)
);

INSERT INTO `archive` (`contents`) VALUES ("<SINGLE>");

CREATE TABLE `file_revision` (
	`file_id` BIGINT UNSIGNED NOT NULL,
	`blake2b` BINARY(64) NOT NULL,
	`sha3` BINARY(64) NOT NULL,
	`size` BIGINT UNSIGNED NOT NULL,
	`archive_time` DATETIME(2) NOT NULL,
	`archive` BIGINT UNSIGNED NOT NULL,
	PRIMARY KEY (`file_id`, `archive_time`),
	FOREIGN KEY (`file_id`) REFERENCES `file`(`id`),
	FOREIGN KEY (`archive`) REFERENCES `archive`(`id`)
);

CREATE TABLE `file_duplicate` (
	`file_id` BIGINT UNSIGNED NOT NULL,
	`archive_time` DATETIME(2) NOT NULL,
	`duplicate_file_id` BIGINT UNSIGNED NOT NULL,
	`duplicate_file_archive_time` DATETIME(2) NOT NULL,
	PRIMARY KEY (`file_id`, `archive_time`),
	FOREIGN KEY (`file_id`) REFERENCES `file`(`id`),
	FOREIGN KEY (`duplicate_file_id`, `duplicate_file_archive_time`) REFERENCES `file_revision`(`file_id`, `archive_time`)
);
