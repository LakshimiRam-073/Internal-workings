package main

import (
	"fmt"
	"os"

	"golang.org/x/sys/unix"
)

const initialMmapSize = PageSize * 32 // start with space for 32 pages

type MmapStorage struct {
	file     *os.File
	data     []byte
	fileSize int64
}

func NewMmapStorage(fileName string) (*MmapStorage, error) {

	file, err := os.OpenFile(fileName, os.O_CREATE|os.O_RDWR, 0644)
	if err != nil {
		return nil, fmt.Errorf("Couldn't able to open file %s with error %w", fileName, err)
	}

	//check size
	stat, err := file.Stat()
	if err != nil {
		if os.IsNotExist(err) {
			return nil, fmt.Errorf("File not exists %s\n", fileName)
		}
		return nil, fmt.Errorf("Error while opening file %s\n", fileName)
	}
	fileSize := stat.Size()

	//resize
	if initialMmapSize > fileSize {
		if err := file.Truncate(initialMmapSize); err != nil {
			return nil, fmt.Errorf("Couldn't able to truncate file %s\n", fileName)
		}
	}

	data, err := unix.Mmap(int(file.Fd()), 0, initialMmapSize, unix.PROT_WRITE|unix.PROT_READ, unix.MAP_SHARED)
	if err != nil {
		return nil, fmt.Errorf("Couldn't able to map file %s with error %w\n", fileName, err)
	}

	return &MmapStorage{
		file:     file,
		data:     data,
		fileSize: initialMmapSize,
	}, nil

}

// remap grows the file and remaps the mmap region.
func (ms *MmapStorage) remap(newSize int64) error {
	// Unmap current region
	if err := unix.Munmap(ms.data); err != nil {
		return fmt.Errorf("munmap failed: %w", err)
	}

	// Grow the file
	if err := ms.file.Truncate(newSize); err != nil {
		return fmt.Errorf("truncate failed: %w", err)
	}

	// Remap with new size
	data, err := unix.Mmap(int(ms.file.Fd()), 0, int(newSize),
		unix.PROT_READ|unix.PROT_WRITE, unix.MAP_SHARED)
	if err != nil {
		return fmt.Errorf("mmap remap failed: %w", err)
	}

	ms.data = data
	ms.fileSize = newSize
	return nil
}

func (ms *MmapStorage) ReadAt(buf []byte, offset int64) (int, error) {
	end := offset + int64(len(buf))
	if end > ms.fileSize {
		return 0, fmt.Errorf("read beyond mapped region: offset %d, len %d, mapped %d", offset, len(buf), ms.fileSize)
	}
	n := copy(buf, ms.data[offset:end])
	return n, nil
}

func (ms *MmapStorage) WriteAt(buf []byte, offset int64) (int, error) {
	end := offset + int64(len(buf))

	// Grow if needed (double the mapping until it fits)
	if end > ms.fileSize {
		newSize := ms.fileSize
		for newSize < end {
			newSize *= 2
		}
		if err := ms.remap(newSize); err != nil {
			return 0, err
		}
	}

	n := copy(ms.data[offset:end], buf)
	return n, nil
}

// ReadDirect returns a slice directly into the mmap region — zero allocation, zero copy.
// The returned slice is only valid until the next remap or Close.
func (ms *MmapStorage) ReadDirect(offset int64, length int) ([]byte, error) {
	end := offset + int64(length)
	if end > ms.fileSize {
		return nil, fmt.Errorf("read beyond mapped region: offset %d, len %d, mapped %d", offset, length, ms.fileSize)
	}
	return ms.data[offset:end], nil
}

func (ms *MmapStorage) Sync() error {
	return unix.Msync(ms.data, unix.MS_SYNC)
}

func (ms *MmapStorage) Close() error {
	if err := unix.Munmap(ms.data); err != nil {
		ms.file.Close()
		return fmt.Errorf("munmap failed: %w", err)
	}
	return ms.file.Close()
}

func (ms *MmapStorage) Name() string {
	return ms.file.Name()
}

func (ms *MmapStorage) Size() (int64, error) {
	return ms.fileSize, nil
}
