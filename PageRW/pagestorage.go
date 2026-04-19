package main

type PageStorage interface {
	ReadAt(buf []byte, offset int64) (int, error)
	WriteAt(buf []byte, offset int64) (int, error)
	
	Sync() error
	Close() error
	Name() string
	Size() (int64, error)
}
