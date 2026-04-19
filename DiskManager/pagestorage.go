package main

type PageStorage interface {
	ReadAt(buf []byte, offset int64) (int, error)
	WriteAt(buf []byte, offset int64) (int, error)

	Sync() error
	Close() error
	Name() string
	Size() (int64, error)
}

// DirectReader allows zero-copy reads by returning a slice
// directly into the underlying storage (e.g. mmap region).
// The returned slice is only valid until the next remap/close.
type DirectReader interface {
	ReadDirect(offset int64, length int) ([]byte, error)
}
