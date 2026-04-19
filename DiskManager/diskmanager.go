package main

import (
	"context"
	"encoding/binary"
	"fmt"
	"io"
)

const PageSize = 4096
const PAGE_DIRTY_BIT uint8 = 0

type DiskManager struct {
	storage      PageStorage
	allocatedMap *Bitmap
	dirtyPages   []*Page
}

type Page struct {
	PageId uint64
	Data   []byte
	Flags  byte
}

func (disk *DiskManager) NewPage(data []byte) *Page {
	ctx := context.Background()
	disk.allocatedMap.getFirstUnsetBit(ctx)
	freePageInfo := <-disk.allocatedMap.NumChannel
	disk.allocatedMap.SetBit(freePageInfo)
	page := Page{
		PageId: freePageInfo.resolvedBitNumber,
		Flags:  0,
		Data:   data,
	}
	page.SetFlag(PAGE_DIRTY_BIT)

	return &page
}

// Need to edit.
func NewDiskManager(fileName string) (*DiskManager, error) {
	storage, err := NewFileStorage(fileName)
	if err != nil {
		return nil, err
	}
	return NewDiskManagerWithStorage(storage), nil
}

func NewDiskManagerWithStorage(storage PageStorage) *DiskManager {
	return &DiskManager{
		storage:      storage,
		allocatedMap: NewBitMap(),
		dirtyPages:   make([]*Page, 0),
	}
}

func (disk *DiskManager) Close() error {
	return disk.storage.Close()
}

func (disk *DiskManager) Sync() error {
	err := disk.storage.Sync()
	if err != nil {
		return err
	}
	for _, page := range disk.dirtyPages {
		page.UnSetFlag(PAGE_DIRTY_BIT)
	}
	disk.dirtyPages = disk.dirtyPages[:0]
	return nil
}

func (p *Page) IsSet(flag uint8) bool {
	afterOp := p.Flags & (1 << flag)
	return afterOp != 0
}

func (p *Page) SetFlag(flag uint8) {
	p.Flags = p.Flags | (1 << flag)
}
func (p *Page) UnSetFlag(flag uint8) {
	p.Flags = p.Flags & ^(1 << flag)
}

func EncodeData(unEncodedData []byte) []byte {
	buf := make([]byte, PageSize)

	binary.LittleEndian.PutUint32(buf[0:4], uint32(len(unEncodedData)))
	copy(buf[4:], unEncodedData)

	return buf
}

func DecodeData(data []byte) []byte {

	len := binary.LittleEndian.Uint32(data[0:4])
	fmt.Println(len)
	return data[4 : 4+len]
}

func (disk *DiskManager) ReadPage(pageId uint64) (*Page, error) {
	size, err := disk.storage.Size()
	if err != nil {
		return nil, fmt.Errorf("Couldn't get file size: %w", err)
	}

	offSet := pageId * PageSize
	if offSet >= uint64(size) {
		return nil, fmt.Errorf("PageID overflow: offset %d >= size %d", offSet, size)
	}

	// Zero-copy path: if storage supports direct reads (mmap),
	// return a slice into the mapped region — no alloc, no copy.
	if dr, ok := disk.storage.(DirectReader); ok {
		data, err := dr.ReadDirect(int64(offSet), PageSize)
		if err != nil {
			return nil, fmt.Errorf("Could not direct-read at offset %d: %w", offSet, err)
		}
		return &Page{
			PageId: pageId,
			Data:   data,
		}, nil
	}

	// Fallback: allocate buffer and copy (FileStorage path).
	readBuff := make([]byte, PageSize)

	_, err = disk.storage.ReadAt(readBuff, int64(offSet))
	if err != nil && err != io.EOF {
		return nil, fmt.Errorf("Could not read at offset %d: %w", offSet, err)
	}

	return &Page{
		PageId: pageId,
		Data:   readBuff,
	}, nil
}

func (disk *DiskManager) DeletePage(pageId int64) error {
	nthByte := pageId / 8
	mthBit := pageId % 8
	bit := Bit{
		nthByte:   uint64(nthByte),
		bitNumber: uint8(mthBit),
	}

	return disk.allocatedMap.UnSetBit(bit)
}
func (disk *DiskManager) WritePage(page *Page) (int, error) {
	// 0 filling
	if len(page.Data) < PageSize {
		for i := len(page.Data); i < PageSize; i++ {
			page.Data[i] = 0
		}
	}
	offSet := page.PageId * PageSize

	n, err := disk.storage.WriteAt(page.Data, int64(offSet))
	if err != nil {
		return 0, fmt.Errorf("Could not write page %d: %w", page.PageId, err)
	}

	// disk.Sync() -> test_result_page_sync.txt
	return n, nil
}
