.PHONY: all
all:
	go build -o bin/archiver src/app/*.go
	cp src/config/settings.json bin/settings.json
	cp src/database/make.sql bin/make.sql
