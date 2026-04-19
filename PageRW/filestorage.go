package main

import (
	"fmt"
	"os"
)

type FileStorage struct{
	file *os.File
}

func NewFileStorage(fileName string) (*FileStorage, error) {
	file, err := os.OpenFile(fileName, os.O_CREATE|os.O_RDWR, 0644)
	if err != nil {
		return nil, fmt.Errorf("could not open file %s: %w", fileName, err)
	}
	return &FileStorage{file: file}, nil
}

func (fs *FileStorage) ReadAt(buf []byte, offset int64) (int, error) {
	return fs.file.ReadAt(buf, offset)
}

func (fs *FileStorage) WriteAt(buf []byte, offset int64) (int, error) {
	return fs.file.WriteAt(buf, offset)
}

func (fs *FileStorage) Sync() error {
	return fs.file.Sync()
}

func (fs *FileStorage) Close() error {
	return fs.file.Close()
}

func (fs *FileStorage) Name() string {
	return fs.file.Name()
}

func (fs *FileStorage) Size() (int64, error) {
	stats, err := fs.file.Stat()
	if err != nil {
		return 0, err
	}
	return stats.Size(), nil
}