DROP DATABASE IF EXISTS `archiver`;

CREATE DATABASE `archiver`;

USE `archiver`;

CREATE TABLE `directory`
(
    `id`   BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(1024)   NOT NULL,
    PRIMARY KEY (`id`)
);

CREATE TABLE `directory_parent`
(
    `parent_id` BIGINT UNSIGNED NOT NULL,
    `child_id`  BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`parent_id`, `child_id`),
    FOREIGN KEY (`parent_id`) REFERENCES `directory` (`id`),
    FOREIGN KEY (`child_id`) REFERENCES `directory` (`id`)
);

INSERT INTO `directory` (`name`)
VALUES ("/");

CREATE TABLE `file`
(
    `id`   BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(1024)   NOT NULL,
    PRIMARY KEY (`id`)
);

CREATE TABLE `file_parent`
(
    `directory_id` BIGINT UNSIGNED NOT NULL,
    `file_id`      BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`directory_id`, `file_id`),
    FOREIGN KEY (`directory_id`) REFERENCES `directory` (`id`),
    FOREIGN KEY (`file_id`) REFERENCES `file` (`id`)
);

CREATE TABLE `archive`
(
    `id`               BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `next_part_number` BIGINT UNSIGNED NOT NULL DEFAULT 1,
    `contents`         VARCHAR(255)    NOT NULL,
    PRIMARY KEY (`id`)
);

INSERT INTO `archive` (`contents`)
VALUES ("<SINGLE>");

CREATE TABLE `file_revision`
(
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `hash`         TEXT(256),
    `size`         BIGINT UNSIGNED,
    `archive_time` DATETIME(2)     NOT NULL,
    PRIMARY KEY (`id`)
);

CREATE TABLE `file_revision_parent`
(
    `revision_id` BIGINT UNSIGNED NOT NULL,
    `file_id`     BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`revision_id`, `file_id`),
    FOREIGN KEY (`revision_id`) REFERENCES `file_revision` (`id`),
    FOREIGN KEY (`file_id`) REFERENCES `file` (`id`)
);

CREATE TABLE `file_revision_archive`
(
    `revision_id` BIGINT UNSIGNED NOT NULL,
    `archive_id`  BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`revision_id`, `archive_id`),
    FOREIGN KEY (`revision_id`) REFERENCES `file_revision` (`id`),
    FOREIGN KEY (`archive_id`) REFERENCES `archive` (`id`)
);

CREATE TABLE `file_revision_duplicate`
(
    `revision_id`          BIGINT UNSIGNED NOT NULL,
    `original_revision_id` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`revision_id`, `original_revision_id`),
    FOREIGN KEY (`revision_id`) REFERENCES `file_revision` (`id`),
    FOREIGN KEY (`original_revision_id`) REFERENCES `file_revision` (`id`)
);

CREATE TABLE `staged_directory`
(
    `id`   BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(1024)   NOT NULL,
    PRIMARY KEY (`id`)
);

CREATE TABLE `staged_directory_parent`
(
    `parent_id` BIGINT UNSIGNED NOT NULL,
    `child_id`  BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`parent_id`, `child_id`),
    FOREIGN KEY (`parent_id`) REFERENCES `staged_directory` (`id`),
    FOREIGN KEY (`child_id`) REFERENCES `staged_directory` (`id`)
);

CREATE TABLE `staged_file`
(
    `id`   BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(1024)   NOT NULL,
    `hash` TEXT(256)       NOT NULL,
    `size` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`id`)
);

CREATE TABLE `staged_file_parent`
(
    `directory_id` BIGINT UNSIGNED NOT NULL,
    `file_id`      BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`directory_id`, `file_id`),
    FOREIGN KEY (`directory_id`) REFERENCES `staged_directory` (`id`),
    FOREIGN KEY (`file_id`) REFERENCES `staged_file` (`id`)
);